package net.afpro.sc;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.view.View;

public class MyActivity extends Activity {
    MainHolder holder;
    RawImageCenter imgCenter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        holder = new MainHolder(findViewById(android.R.id.content));

        final DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        imgCenter = new RawImageCenter(dm.widthPixels, dm.heightPixels, getExternalFilesDir(Environment.DIRECTORY_PICTURES));

        holder.getBtConvert().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                imgCenter.convert(MyActivity.this, holder.getNameEdit().getText().toString(), holder.getPathEdit().getText().toString());
            }
        });

        holder.getBtShow().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                holder.getImg().setImageDrawable(new CustomDrawable(imgCenter, holder.getNameEdit().getText().toString()));
            }
        });
    }
}
