package net.afpro.sc;

import android.graphics.Bitmap;

/**
 * User: afpro
 * Date: 5/22/13
 * Time: 7:46 PM
 */
public class RawImageCenter {
    public final static String TAG = "RawImageCenter";

    static {
        System.loadLibrary("sc");
        setupLogTag(TAG);
    }

    public static native void setupLogTag(String tag);

    public static native boolean save(String path, Bitmap bitmap);

    public static native int map(String path);

    public static native void unmap(int ptr);

    public static native int getWidth(int ptr);

    public static native int getHeight(int ptr);

    public static native boolean draw(int ptr,
                                    int srcx, int srcy, int srcw, int srch,
                                    int dstx, int dsty, int dstw, int dsth,
                                    Bitmap buffer);
}
