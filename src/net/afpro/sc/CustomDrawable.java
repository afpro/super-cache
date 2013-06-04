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
