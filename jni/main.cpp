// standard c&c++
#include <stdio.h>
#include <stdlib.h>
#include <string>

// unix
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

// java wrapper
#include <jni.h>

// android
#include <android/log.h>
#include <android/bitmap.h>

// struct
struct map_t {
    int fd;
    size_t size;
    int *ptr;
};

// data
std::string tag("sc");

// utils
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, tag.c_str(), __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, tag.c_str(), __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, tag.c_str(), __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, tag.c_str(), __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, tag.c_str(), __VA_ARGS__)

// interfaces
void setupLogTag(JNIEnv *env, jclass, jstring str) {
    const char *ptr = env->GetStringUTFChars(str, 0);
    tag = ptr;
    env->ReleaseStringUTFChars(str, ptr);
}

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

jint map(JNIEnv *env, jclass, jstring jpath) {
    map_t *mpt = new map_t;

    // open
    const char *path = env->GetStringUTFChars(jpath, 0);
    mpt->fd = open(path, O_RDONLY);
    env->ReleaseStringUTFChars(jpath, path);

    if(!mpt->fd) {
        LOGE("open file failed");
        delete mpt;
        return 0;
    }

    // get status
    struct stat s;
    fstat(mpt->fd, &s);
    mpt->size = s.st_size;

    // map
    mpt->ptr = (int*)mmap(0, mpt->size, PROT_READ, MAP_FILE|MAP_SHARED, mpt->fd, 0);
    if(!mpt->ptr) {
        LOGE("mmap failed");
        delete mpt;
        return 0;
    }

    // return
    return (jint)mpt;
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
    return mpt->ptr[0];
}

jint getHeight(JNIEnv *, jclass, jint ptr) {
    map_t *mpt = (map_t*)ptr;
    if(!mpt) {
        LOGE("get width of null pointer");
        return -1;
    }
    return mpt->ptr[1];
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

    // try lock
    int *dst_addr = 0;
    AndroidBitmap_lockPixels(env, buffer, (void**)&dst_addr);
    if(!dst_addr) {
        LOGE("lock bitmap failed");
        return JNI_FALSE;
    }

    // src info
    int src_stride = mpt->ptr[0] * mpt->ptr[1];
    int *src_addr = mpt->ptr + 2;

    // dst info
    int dst_stride = info.stride >> 2;

    // draw&scale
    for(int x=0; x<dw; x++) {
        for(int y=0; y<dh; y++) {
            int src_x = x * sw / dw + sx;
            int src_y = y * sh / dh + sy;
            int dst_x = x + dx;
            int dst_y = y + dy;
            dst_addr[dst_y * dst_stride + dst_x] = src_addr[src_y * src_stride + src_x];
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
        {"setupLogTag", "(Ljava/lang/String;)V", (void*)&setupLogTag},
        {"save", "(Ljava/lang/String;Landroid/graphics/Bitmap;)Z", (void*)&save},
        {"map", "(Ljava/lang/String;)I", (void*)&map},
        {"unmap", "(I)V", (void*)&unmap},
        {"getWidth", "(I)I", (void*)&getWidth},
        {"getHeight", "(I)I", (void*)&getHeight},
        {"draw", "(IIIIIIIIILandroid/graphics/Bitmap;)Z", (void*)&draw},
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
