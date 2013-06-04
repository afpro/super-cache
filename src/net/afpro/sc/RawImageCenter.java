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

package net.afpro.sc;

import android.content.Context;
import android.graphics.*;

import java.io.File;
import java.security.InvalidParameterException;
import java.util.Iterator;
import java.util.LinkedHashMap;

/**
 * User: afpro
 * Date: 5/22/13
 * Time: 7:46 PM
 */
public class RawImageCenter {
    public final static String TAG = "sc";
    public final int CACHE_SIZE = 10;

    static {
        System.loadLibrary("sc");
    }

    private final Bitmap buf;
    private final int dw, dh;
    private final File cacheDir;
    private final LinkedHashMap<String, Integer> mapped = new LinkedHashMap<String, Integer>(CACHE_SIZE, 0.75f, true);

    private String absPath(String name) {
        return cacheDir.getAbsolutePath() + File.separatorChar + name;
    }

    private int checkMapped(String name) {
        assert name != null;
        if (!mapped.containsKey(name)) {
            if (mapped.size() > CACHE_SIZE) {
                Iterator<LinkedHashMap.Entry<String, Integer>> iterator = mapped.entrySet().iterator();
                for (int i = mapped.size() - CACHE_SIZE; i > 0 && iterator.hasNext(); i--) {
                    LinkedHashMap.Entry<String, Integer> e = iterator.next();
                    mapped.remove(e.getKey());
                    native_unmap(e.getValue());
                }
            }
            int ptr = native_map(absPath(name));
            if (ptr != 0) {
                mapped.put(name, ptr);
            }
            return ptr;
        } else {
            return mapped.get(name);
        }
    }

    public RawImageCenter(int bufw, int bufh, File cacheDir) {
        if (cacheDir == null)
            throw new NullPointerException("can't use null cache dir");

        if (bufw <= 0 || bufh <= 0)
            throw new InvalidParameterException("bufw or bufh is negative");

        buf = Bitmap.createBitmap(bufw, bufh, Bitmap.Config.ARGB_8888);
        dw = bufw;
        dh = bufh;
        this.cacheDir = cacheDir;
    }

    public boolean save(String name, Bitmap bitmap) {
        if (name == null || bitmap == null)
            throw new NullPointerException("name or bitmap is null");

        if (bitmap.isRecycled() || bitmap.getConfig() != Bitmap.Config.ARGB_8888)
            throw new InvalidParameterException("bitmap may be recycled or not ARGB_8888");

        return native_save(absPath(name), bitmap);
    }

    public boolean save(String name, String originPath) {
        if (name == null || originPath == null)
            throw new NullPointerException("name or originPath is null");

        Bitmap bitmap = BitmapFactory.decodeFile(originPath);
        if (bitmap == null || bitmap.getConfig() != Bitmap.Config.ARGB_8888)
            throw new RuntimeException("decode bitmap failed or not ARGB_8888");

        try {
            return native_save(absPath(name), bitmap);
        } finally {
            bitmap.recycle();
        }
    }

    public void convert(Context ctx, String name, String jpegPath) {
        if(ctx == null || name == null || jpegPath == null)
            throw new NullPointerException("ctx or name or jpegPath is null");

        final String cache;
        if(ctx.getExternalCacheDir() != null) {
            cache = ctx.getExternalCacheDir().getAbsolutePath() + File.separatorChar + "img_cache";
        } else {
            cache = ctx.getCacheDir().getAbsolutePath() + File.separatorChar + "img_cache";
        }

        native_convert(cache, jpegPath, absPath(name));
    }

    public int getWidth(String name) {
        return native_getWidth(checkMapped(name));
    }

    public int getHeight(String name) {
        return native_getHeight(checkMapped(name));
    }

    public boolean draw(String name, Rect src, Rect dst, Canvas canvas, Paint paint) {
        if (name == null || canvas == null || dst == null)
            throw new NullPointerException("name or canvas or dst is null");

        int ptr = checkMapped(name);
        if(ptr == 0)
            return false;

        int sw = native_getWidth(ptr);
        int sh = native_getHeight(ptr);

        if (src == null) {
            src = new Rect(0, 0, sw, sh);
        } else {
            if (src.left < 0)
                src.left = 0;
            if (src.top < 0)
                src.top = 0;
            if (src.width() > sw)
                src.right = src.left + sw;
            if (src.height() > sh)
                src.bottom = src.top + sh;
        }

        int dstw = dst.width();
        int dsth = dst.height();
        if (dstw > dw)
            dstw = dw;
        if (dsth > dh)
            dsth = dh;
        if (native_draw(ptr,
                src.left, src.top, src.width(), src.height(),
                0, 0, dstw, dsth,
                buf)) {
            canvas.drawBitmap(buf, new Rect(0, 0, dstw, dsth), dst, paint);
            return true;
        } else {
            return false;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        for (int ptr : mapped.values())
            native_unmap(ptr);
    }

    private static native boolean native_save(String path, Bitmap bitmap);

    private static native boolean native_convert(String cache, String from, String to);

    private static native int native_map(String path);

    private static native void native_unmap(int ptr);

    private static native int native_getWidth(int ptr);

    private static native int native_getHeight(int ptr);

    private static native boolean native_draw(int ptr,
                                              int srcx, int srcy, int srcw, int srch,
                                              int dstx, int dsty, int dstw, int dsth,
                                              Bitmap buffer);
}
