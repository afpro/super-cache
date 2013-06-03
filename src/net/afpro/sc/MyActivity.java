package net.afpro.sc;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.widget.ImageView;

public class MyActivity extends Activity {
    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        /*
        TextView v = new TextView(this);
        setContentView(v);

        ActivityManager mgr = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        int memClass = mgr.getMemoryClass();
        ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
        mgr.getMemoryInfo(memInfo);

        v.setText(String.format("memClass %d\nava %d\ntotal %d\nthreshold %d\nlow %b", memClass,
                memInfo.availMem, memInfo.totalMem, memInfo.threshold, memInfo.lowMemory));
                */

        Bitmap bmp = BitmapFactory.decodeFile("/sdcard/fuck.jpg");
        RawImageCenter.save("/sdcard/fuck.jpg.raw", bmp);

        Bitmap buf = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
        int ptr = RawImageCenter.map("/sdcard/fuck.jpg.raw");
        RawImageCenter.draw(ptr, 0, 0, bmp.getWidth(), bmp.getHeight(), 0, 0, 50, 50, buf);
        RawImageCenter.unmap(ptr);

        ImageView imageView = new ImageView(this);
        imageView.setImageBitmap(buf);
        setContentView(imageView);
    }
}
