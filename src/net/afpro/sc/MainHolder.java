package net.afpro.sc;

import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;

public class MainHolder {
    private EditText nameEdit;
    private EditText pathEdit;
    private Button btShow;
    private Button btConvert;
    private ImageView img;

    public MainHolder(View view) {
        this.nameEdit = (EditText) view.findViewById(R.id.name_edit);
        this.pathEdit = (EditText) view.findViewById(R.id.path_edit);
        this.btShow = (Button) view.findViewById(R.id.bt_show);
        this.btConvert = (Button) view.findViewById(R.id.bt_convert);
        this.img = (ImageView) view.findViewById(R.id.img);
    }

    public EditText getNameEdit() {
        return nameEdit;
    }

    public EditText getPathEdit() {
        return pathEdit;
    }

    public Button getBtShow() {
        return btShow;
    }

    public Button getBtConvert() {
        return btConvert;
    }

    public ImageView getImg() {
        return img;
    }
}
