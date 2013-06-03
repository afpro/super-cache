package net.afpro.sc;

import android.graphics.*;
import android.graphics.drawable.Drawable;
import android.util.Log;

/**
 * User: afpro
 * Date: 5/21/13
 * Time: 4:20 PM
 */
public class CustomDrawable extends Drawable {
    private final Paint paint = new Paint();
    private final Rect bound = new Rect();

    private final RawImageCenter imgCenter;
    private final String name;

    public CustomDrawable(RawImageCenter imgCenter, String name) {
        this.imgCenter = imgCenter;
        this.name = name;
    }

    @Override
    public void draw(Canvas canvas) {
        Log.d("fuck", "bound: " + bound);
        imgCenter.draw(name, null, bound, canvas, paint);
    }

    @Override
    public void setAlpha(int i) {
        int a = paint.getAlpha();
        if (a != i) {
            paint.setAlpha(i);
            invalidateSelf();
        }
    }

    @Override
    public void setColorFilter(ColorFilter colorFilter) {
        paint.setColorFilter(colorFilter);
        invalidateSelf();
    }

    @Override
    public int getOpacity() {
        return 0;
    }

    @Override
    public void setFilterBitmap(boolean filter) {
        paint.setFilterBitmap(filter);
        invalidateSelf();
    }

    @Override
    public void setDither(boolean dither) {
        paint.setDither(dither);
        invalidateSelf();
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        bound.set(bounds);
    }

    @Override
    public int getIntrinsicWidth() {
        return imgCenter.getWidth(name);
    }

    @Override
    public int getIntrinsicHeight() {
        return imgCenter.getHeight(name);
    }
}
