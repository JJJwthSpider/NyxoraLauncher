package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;

import java.io.File;
import java.io.FileOutputStream;

/**
 * The player's skin and cape kept on the phone, so the lobby shows them at once, also without internet or when
 * Mojang's texture server is slow (before, every start fetched them again and the player stayed a shadow when that
 * failed). A cape made by the generator is kept here too; it shows in Nyxora.
 */
final class SkinCache {
    private SkinCache() {}

    private static File dir(Context context) {
        File dir = new File(context.getFilesDir(), "skins");
        dir.mkdirs();
        return dir;
    }

    private static File skinFile(Context context, Account account) {
        return new File(dir(context), (account.uuid.isEmpty() ? "player" : account.uuid) + ".png");
    }

    private static File capeFile(Context context, Account account) {
        return new File(dir(context), (account.uuid.isEmpty() ? "player" : account.uuid) + "-cape.png");
    }

    static File generatedCapeFile(Context context) {
        return new File(dir(context), "generated-cape.png");
    }

    static Bitmap skin(Context context, Account account) {
        return read(skinFile(context, account));
    }

    static void saveSkin(Context context, Account account, Bitmap skin) {
        write(skinFile(context, account), skin);
    }

    static Bitmap cape(Context context, Account account) {
        return read(capeFile(context, account));
    }

    static void saveCape(Context context, Account account, Bitmap cape) {
        if (cape == null) {
            capeFile(context, account).delete();
        } else {
            write(capeFile(context, account), cape);
        }
    }

    /** the generated cape chosen in the wardrobe (null: none) */
    static Bitmap generatedCape(Context context) {
        if (!context.getSharedPreferences("wardrobe", Context.MODE_PRIVATE).getBoolean("generatedCape", false)) {
            return null;
        }
        return read(generatedCapeFile(context));
    }

    static void useGeneratedCape(Context context, Bitmap cape) {
        if (cape != null) {
            write(generatedCapeFile(context), cape);
        }
        context.getSharedPreferences("wardrobe", Context.MODE_PRIVATE).edit().putBoolean("generatedCape", cape != null).apply();
    }

    static Bitmap read(File file) {
        if (!file.isFile()) {
            return null;
        }
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        return BitmapFactory.decodeFile(file.getPath(), options);
    }

    static void write(File file, Bitmap bitmap) {
        try {
            File temp = new File(file.getPath() + ".tmp");
            FileOutputStream out = new FileOutputStream(temp);
            try {
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
            } finally {
                out.close();
            }
            temp.renameTo(file);
        } catch (Exception e) {
            // the cache is a convenience: without it the skin is fetched again next time
        }
    }

    /** the face of a skin (with the hat layer), sharp and big, for the account chip */
    static Bitmap head(Bitmap skin) {
        if (skin == null || skin.getWidth() < 64) {
            return null;
        }
        Bitmap out = Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(out);
        Paint p = new Paint();
        p.setFilterBitmap(false);
        float s = skin.getWidth() / 64f;
        c.drawBitmap(skin, new Rect(Math.round(8 * s), Math.round(8 * s), Math.round(16 * s), Math.round(16 * s)), new Rect(0, 0, 64, 64), p);
        c.drawBitmap(skin, new Rect(Math.round(40 * s), Math.round(8 * s), Math.round(48 * s), Math.round(16 * s)), new Rect(0, 0, 64, 64), p);
        return out;
    }
}
