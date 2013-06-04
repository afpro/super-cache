/*
* Copyright (c) 1998, Regents of the University of California
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the University of California, Berkeley nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS" AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// standard c&c++
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

// unix
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <setjmp.h>

// image
extern "C" {
#include <png.h>
#include <jpeglib.h>
}

// java wrapper
#include <jni.h>

// android
#include <android/log.h>
#include <android/bitmap.h>

// sc
#include "log.h"

// struct
struct map_t {
	int fd;
	size_t size;
	char *ptr;
};

union my_error_t {
	struct {
		struct jpeg_error_mgr pub;    /* "public" fields */
		jmp_buf setjmp_buffer;    /* for return to caller */
	}jpeg;
	struct {
	}png;
};

// utils
inline void copy4(char *dst, char *src) {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

// callbacks
void jpeg_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_t *myerr = (my_error_t*) cinfo->err;

	/* print message */
	char buf[JMSG_LENGTH_MAX];
	cinfo->err->format_message(cinfo, buf);
	LOGV("jpeg error: %s", buf);

	/* Return control to the setjmp point */
	longjmp(myerr->jpeg.setjmp_buffer, 1);
}

// interfaces
jboolean save(JNIEnv *env, jclass, jstring jpath, jobject bitmap) {
	FILE *fp = 0;
	const char *path = 0;
	AndroidBitmapInfo info = {0,0,0,0,0};
	char *ptr = 0;

	if(!jpath) {
		LOGE("null path");
		goto early_fail;
	}

	if(!bitmap) {
		LOGE("null bitmap");
		goto early_fail;
	}

	path = env->GetStringUTFChars(jpath, 0);
	fp = fopen(path, "w");
	env->ReleaseStringUTFChars(jpath, path);

	if(!fp) {
		LOGE("open file failed");
		goto early_fail;
	}

	AndroidBitmap_getInfo(env, bitmap, &info);
	AndroidBitmap_lockPixels(env, bitmap, (void**)&ptr);
	if(!ptr) {
		LOGE("lock bitmap failed");
		goto fail;
	}

	fwrite(&info.width, 4, 1, fp);
	fwrite(&info.height, 4, 1, fp);

	for(int i=0, line=info.width<<2; i<info.height; i++, ptr += info.stride) {
		fwrite(ptr, line, 1, fp);
	}

	AndroidBitmap_unlockPixels(env, bitmap);

success:
	fclose(fp);
	return JNI_TRUE;

fail:
	fclose(fp);

early_fail:
	return JNI_FALSE;
}

jboolean convert(JNIEnv *env, jclass, jstring cachePath, jstring srcPath, jstring dstPath) {
	// methods
	bool try_jpeg(FILE *sfp, FILE *dfp, const char *cache);
	bool try_png(FILE *sfp, FILE *dfp);

	// get path
	const char *srcCPath = env->GetStringUTFChars(srcPath, 0);
	const char *dstCPath = env->GetStringUTFChars(dstPath, 0);
	const char *cacheCPath = env->GetStringUTFChars(cachePath, 0);
	LOGV("convert %s => %s", srcCPath, dstCPath);

	// file operation
	FILE *sfp = fopen(srcCPath, "rb");
	FILE *dfp = fopen(dstCPath, "wb");
	std::string cp = cacheCPath;

	// release path
	env->ReleaseStringUTFChars(srcPath, srcCPath);
	env->ReleaseStringUTFChars(dstPath, dstCPath);
	env->ReleaseStringUTFChars(cachePath, cacheCPath);

	// common var
	jboolean success = JNI_FALSE;

	// check file open status
	if(!sfp || !dfp) {
		LOGD("sfp or dfp is null");
		goto fail;
	}

	if(try_jpeg(sfp, dfp, cp.c_str()))
		goto success;

	if (try_png(sfp, dfp))
		goto success;

	goto fail;

success:
	success = JNI_TRUE;

fail:
	LOGV("cleanup and quit");
	if(sfp) fclose(sfp);
	if(dfp) fclose(dfp);
	return success;
}

jint map(JNIEnv *env, jclass, jstring jpath) {
	const char *path;
	map_t *mpt;
	struct stat s;
	int wh[2];

	mpt = new map_t;
	memset(mpt, 0, sizeof(map_t));

	// open
	path = env->GetStringUTFChars(jpath, 0);
	mpt->fd = open(path, O_RDONLY);
	env->ReleaseStringUTFChars(jpath, path);

	if(!mpt->fd) {
		LOGE("open file failed");
		goto fail;
	}

	// get status
	fstat(mpt->fd, &s);
	if(s.st_size < 8) {
		LOGE("file too small");
		goto fail;
	}
	mpt->size = s.st_size;

	// map
	mpt->ptr = (char*)mmap(0, mpt->size, PROT_READ, MAP_FILE|MAP_SHARED, mpt->fd, 0);
	if(!mpt->ptr) {
		LOGE("mmap failed");
		goto fail;
	}

	// check
#define SIZE_TO_PIXEL(SIZE) ( ((SIZE)-8) >> 2 )
	memcpy(wh, mpt->ptr, 8);
	if(wh[0] * wh[1] != SIZE_TO_PIXEL(mpt->size)) {
		LOGE("w(%d) * h(%d) = %d != pixels(%d)", wh[0], wh[1], wh[0] * wh[1], SIZE_TO_PIXEL(mpt->size));
		goto fail;
	}

	return (jint)mpt;

fail:
	if(mpt) {
		if(mpt->ptr)
			munmap(mpt->ptr, mpt->size);
		if(mpt->fd)
			close(mpt->fd);
		delete mpt;
	}
	return 0;
}

void unmap(JNIEnv *env, jclass, jint ptr) {
	map_t *mpt = (map_t*)ptr;
	if(!mpt) {
		LOGV("try unmmap null pointer");
	} else {
		if(mpt->ptr) {
			munmap(mpt->ptr, mpt->size);
			mpt->ptr = 0;
		}
		if(mpt->fd) {
			close(mpt->fd);
			mpt->fd = 0;
		}
		mpt->size = 0;
	}
}

jint getWidth(JNIEnv *, jclass, jint ptr) {
	map_t *mpt = (map_t*)ptr;
	if(!mpt) {
		LOGE("get width of null pointer");
		return -1;
	}

	int n;
	copy4((char*)&n, mpt->ptr);
	return n;
}

jint getHeight(JNIEnv *, jclass, jint ptr) {
	map_t *mpt = (map_t*)ptr;
	if(!mpt) {
		LOGE("get width of null pointer");
		return -1;
	}

	int n;
	copy4((char*)&n, mpt->ptr + 4);
	return n;
}

jboolean draw(JNIEnv *env, jclass, jint ptr, jint sx, jint sy, jint sw, jint sh, jint dx, jint dy, jint dw, jint dh, jobject buffer) {
	// get map_t
	map_t *mpt = (map_t*)ptr;
	if(!mpt) {
		LOGE("try to draw null map_t");
		return JNI_FALSE;
	}

	// check null buffer
	if(!buffer) {
		LOGE("try to draw on null bitmap");
		return JNI_FALSE;
	}

	// get info
	AndroidBitmapInfo info = {0,0,0,0,0};
	AndroidBitmap_getInfo(env, buffer, &info);
	LOGV("dst info: [%d,%d], stride=%d", info.width, info.height, info.stride);

	// try lock
	char *dst_addr = 0;
	AndroidBitmap_lockPixels(env, buffer, (void**)&dst_addr);
	if(!dst_addr) {
		LOGE("lock bitmap failed");
		return JNI_FALSE;
	}
	LOGV("dst addr: %p", dst_addr);

	// src info
	int src_w, src_h;
	copy4((char*)&src_w, mpt->ptr);
	copy4((char*)&src_h, mpt->ptr + 4);
	int src_stride = src_w << 2;
	LOGV("src info: [%d,%d], stride=%d", src_w, src_h, src_stride);

	char *src_addr = mpt->ptr + 8;
	LOGV("src addr: %p", src_addr);

	// dst info
	int dst_stride = info.stride;

	// draw&scale
	for(jint x=0; x<dw; x++) {
		for(jint y=0; y<dh; y++) {
			int src_x = x * sw / dw + sx;
			int src_y = y * sh / dh + sy;
			int dst_x = x + dx;
			int dst_y = y + dy;
			copy4(dst_addr + dst_y * dst_stride + (dst_x << 2), src_addr + src_y * src_stride + (src_x<<2));
		}
	}

	// unlock
	AndroidBitmap_unlockPixels(env, buffer);

	// success
	return JNI_TRUE;
}

// entry point
jint JNI_OnLoad(JavaVM *vm, void*) {
	JNIEnv *env;
	if(vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
		LOGE("get env failed");
		return JNI_ERR;
	}

	jclass scz = env->FindClass("net/afpro/sc/RawImageCenter");
	if(!scz) {
		LOGE("get sc class failed");
		return JNI_ERR;
	}

	const static JNINativeMethod methods[] = {
		{"native_save", "(Ljava/lang/String;Landroid/graphics/Bitmap;)Z", (void*)&save},
		{"native_convert", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z", (void*)&convert},
		{"native_map", "(Ljava/lang/String;)I", (void*)&map},
		{"native_unmap", "(I)V", (void*)&unmap},
		{"native_getWidth", "(I)I", (void*)&getWidth},
		{"native_getHeight", "(I)I", (void*)&getHeight},
		{"native_draw", "(IIIIIIIIILandroid/graphics/Bitmap;)Z", (void*)&draw},
	};
	const static jint methodCount = sizeof(methods) / sizeof(JNINativeMethod);
	if(env->RegisterNatives(scz, methods, methodCount) != JNI_OK) {
		LOGE("register native methods failed");
		return JNI_ERR;
	}

	LOGI("sc init success");
	return JNI_VERSION_1_6;
}

// quit point
void JNI_OnUnload(JavaVM *vm, void*) {
}

// try jpeg
bool try_jpeg(FILE *sfp, FILE *dfp, const char *cache) {
	bool success = false;
	int width;
	int height;
	int stride;
	my_error_t err;
	std::vector<char> rgba_line;

	// jpg var
	struct jpeg_decompress_struct jpeg_dec;
	JSAMPARRAY jpeg_line = 0;

	rewind(sfp);
	jpeg_dec.client_data = (void*)cache;
	jpeg_dec.err = jpeg_std_error(&err.jpeg.pub);
	err.jpeg.pub.error_exit = jpeg_error_exit;

	if (setjmp(err.jpeg.setjmp_buffer)) {
		jpeg_destroy_decompress(&jpeg_dec);
		return 0;
	}

	jpeg_create_decompress(&jpeg_dec);
	LOGV("create decompress");

	jpeg_stdio_src(&jpeg_dec, sfp);
	LOGV("stdio src");

	if(jpeg_read_header(&jpeg_dec, FALSE) != JPEG_HEADER_OK) {
		LOGD("is not jpeg");
		goto fail;
	}
	LOGV("got jpeg");

	(void) jpeg_start_decompress(&jpeg_dec);
	LOGV("start decompress");

	width = jpeg_dec.output_width;
	height = jpeg_dec.output_height;
	fwrite(&width, 4, 1, dfp);
	fwrite(&height, 4, 1, dfp);
	LOGV("jpeg: [%d, %d]", width, height);

	stride = jpeg_dec.output_width * jpeg_dec.output_components;
	jpeg_line = (jpeg_dec.mem->alloc_sarray)((j_common_ptr)&jpeg_dec, JPOOL_IMAGE, stride, 1);
	LOGV("stride %d, alloc line %p -> %p", stride, jpeg_line, *jpeg_line);

	rgba_line.resize(jpeg_dec.output_width << 2);
	for(JSAMPROW row = *jpeg_line;jpeg_dec.output_scanline < jpeg_dec.output_height;) {
		jpeg_read_scanlines(&jpeg_dec, jpeg_line, 1);
		for(int x=0, p=0, rp=0; x<width; x++) {
			rgba_line[rp++] = row[p++];
			rgba_line[rp++] = row[p++];
			rgba_line[rp++] = row[p++];
			rgba_line[rp++] = 0xff;
		}
		fwrite(rgba_line.data(), rgba_line.size(), 1, dfp);
	}
	LOGV("jpeg convert finished");

success:
	success = true;

fail:
	(void) jpeg_finish_decompress(&jpeg_dec);
	jpeg_destroy_decompress(&jpeg_dec);
	return success;
}

bool try_png(FILE *sfp, FILE *dfp) {
	bool success = false;
	png_uint_32 width;
	png_uint_32 height;
	png_uint_32 stride;
	int bit_depth, color_type, interlace_type;
	my_error_t err;
	std::vector<char> rgba_line;

	// jpg var
	struct jpeg_decompress_struct jpeg_dec;
	JSAMPARRAY jpeg_line = 0;

	// png vars
	png_structp png_ptr = 0;
	png_infop png_info = 0;
	png_byte png_magic[8];
	png_bytep png_line = 0;

	rewind(sfp);
	fread(png_magic, sizeof(png_magic), 1, sfp);
	if(png_sig_cmp(png_magic, 0, 8)) {
		LOGD("is not png");
		goto fail;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr) {
		LOGD("create read struct failed");
		goto fail;
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		LOGD("png jmp point!");
		png_destroy_read_struct(&png_ptr, &png_info, 0);
		if(png_line)
			free(png_line);
		return 0;
	}

	png_info = png_create_info_struct(png_ptr);
	if(!png_info) {
		LOGD("create info struct failed");
		goto fail;
	}

	png_init_io(png_ptr, sfp);
	png_set_sig_bytes(png_ptr, sizeof(png_magic));
	png_read_info(png_ptr, png_info);
	png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, png_info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	png_read_update_info(png_ptr, png_info);
	png_get_IHDR(png_ptr, png_info, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);
	fwrite(&width, 4, 1, dfp);
	fwrite(&height, 4, 1, dfp);
	LOGV("png [%u, %u], bit_depth %d", width, height, bit_depth);

	stride = png_get_rowbytes(png_ptr, png_info);
	png_line = (png_bytep)malloc(stride);
	LOGV("stride %u, alloc line %p", stride, png_line);

	rgba_line.resize(width << 2);
	switch(stride / width) {
	case 1:
		for(int y=0; y<height; y++) {
			png_read_row(png_ptr, png_line, png_bytep_NULL);
			for(int x=0, p=0, rp=0; x<width; x++) {
				rgba_line[rp++] = png_line[p];   // g
				rgba_line[rp++] = png_line[p];   // g
				rgba_line[rp++] = png_line[p++]; // g
				rgba_line[rp++] = 0xff;          // 0xff
			}
			fwrite(rgba_line.data(), rgba_line.size(), 1, dfp);
		}
		break;
	case 2:
		for(int y=0; y<height; y++) {
			png_read_row(png_ptr, png_line, png_bytep_NULL);
			for(int x=0, p=0, rp=0; x<width; x++) {
				rgba_line[rp++] = png_line[p];   // g
				rgba_line[rp++] = png_line[p];   // g
				rgba_line[rp++] = png_line[p++]; // g
				rgba_line[rp++] = png_line[p++]; // a
			}
			fwrite(rgba_line.data(), rgba_line.size(), 1, dfp);
		}
		break;
	case 3:
		for(int y=0; y<height; y++) {
			png_read_row(png_ptr, png_line, png_bytep_NULL);
			for(int x=0, p=0, rp=0; x<width; x++) {
				rgba_line[rp++] = png_line[p++]; // r
				rgba_line[rp++] = png_line[p++]; // g
				rgba_line[rp++] = png_line[p++]; // b
				rgba_line[rp++] = 0xff;          // 0xff
			}
			fwrite(rgba_line.data(), rgba_line.size(), 1, dfp);
		}
		break;
	case 4:
		for(int y=0; y<height; y++) {
			png_read_row(png_ptr, png_line, png_bytep_NULL);
			for(int x=0, p=0, rp=0; x<width; x++) {
				rgba_line[rp++] = png_line[p++]; // r
				rgba_line[rp++] = png_line[p++]; // g
				rgba_line[rp++] = png_line[p++]; // b
				rgba_line[rp++] = png_line[p++]; // a
			}
			fwrite(rgba_line.data(), rgba_line.size(), 1, dfp);
		}
		break;
	default:
		LOGD("unknown png bit depth %d", bit_depth);
		break;
	}

success:
	success = true;

fail:
	png_destroy_read_struct(&png_ptr, &png_info, 0);
	if(png_line)
		free(png_line);
	return success;
}
