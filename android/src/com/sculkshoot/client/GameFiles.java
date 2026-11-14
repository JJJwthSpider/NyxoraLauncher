package com.sculkshoot.client;

import android.app.AlertDialog;
import android.content.ClipData;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/**
 * The game files of an installation, like Millida Launcher shows them: the screenshot gallery (open, share, delete) and
 * the worlds (size, last played, a backup into Downloads/Nyxora, delete). Only in the full build, where the game runs
 * on the phone.
 */
final class GameFiles {
    private final MainActivity activity;
    private final Installation inst;
    private final File gameDir;

    GameFiles(MainActivity activity, Installation inst) {
        this.activity = activity;
        this.inst = inst;
        this.gameDir = GameEngine.get().gameDir(inst);
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    private static File[] newestFirst(File[] files) {
        if (files == null) {
            return new File[0];
        }
        Arrays.sort(files, new Comparator<File>() {
            public int compare(File a, File b) {
                return Long.compare(b.lastModified(), a.lastModified());
            }
        });
        return files;
    }

    private static String date(long millis) {
        return new SimpleDateFormat("d MMM yyyy, HH:mm", new Locale("ru")).format(new Date(millis));
    }

    /* ------------------------------------------------------------ screenshots */

    void showScreenshots() {
        File[] shots = newestFirst(new File(gameDir, "screenshots").listFiles(new java.io.FileFilter() {
            public boolean accept(File f) {
                return f.isFile() && f.getName().toLowerCase(Locale.ROOT).endsWith(".png");
            }
        }));
        if (shots.length == 0) {
            new AlertDialog.Builder(activity).setTitle("Скриншоты — " + inst.name)
                .setMessage("Скриншотов пока нет. В игре они делаются клавишей F2 (её можно добавить в «Управление → Свои кнопки»).")
                .setPositiveButton("OK", null).show();
            return;
        }
        GridLayout grid = new GridLayout(activity);
        grid.setColumnCount(3);
        grid.setPadding(dp(8), dp(8), dp(8), dp(8));
        final AlertDialog[] dialog = new AlertDialog[1];
        for (final File shot : shots) {
            ImageView image = new ImageView(activity);
            image.setScaleType(ImageView.ScaleType.CENTER_CROP);
            image.setImageBitmap(thumbnail(shot, 320));
            image.setBackground(Ui.rounded(activity, Ui.PANEL, 0, 0xFF322364));
            image.setPadding(dp(3), dp(3), dp(3), dp(3));
            GridLayout.LayoutParams p = new GridLayout.LayoutParams();
            p.width = dp(150);
            p.height = dp(90);
            p.setMargins(dp(4), dp(4), dp(4), dp(4));
            image.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    openScreenshot(shot, dialog[0]);
                }
            });
            grid.addView(image, p);
        }
        ScrollView scroll = new ScrollView(activity);
        scroll.addView(grid);
        dialog[0] = new AlertDialog.Builder(activity).setTitle("Скриншоты — " + inst.name + " (" + shots.length + ")").setView(scroll)
            .setNegativeButton("Закрыть", null).show();
    }

    private static Bitmap thumbnail(File file, int width) {
        BitmapFactory.Options bounds = new BitmapFactory.Options();
        bounds.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(file.getPath(), bounds);
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inSampleSize = Math.max(1, bounds.outWidth / Math.max(1, width));
        return BitmapFactory.decodeFile(file.getPath(), options);
    }

    private void openScreenshot(final File shot, final AlertDialog gallery) {
        ImageView image = new ImageView(activity);
        image.setAdjustViewBounds(true);
        image.setImageBitmap(thumbnail(shot, 1600));
        new AlertDialog.Builder(activity).setTitle(date(shot.lastModified())).setView(image)
            .setPositiveButton("Поделиться", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    share(shot, "image/png", "Отправить скриншот");
                }
            })
            .setNeutralButton("Удалить", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    shot.delete();
                    if (gallery != null) {
                        gallery.dismiss();
                    }
                    showScreenshots();
                }
            })
            .setNegativeButton("Закрыть", null).show();
    }

    private void share(File file, String type, String title) {
        try {
            File copy = new File(PackProvider.exportDir(activity), file.getName());
            copyFile(file, copy);
            Uri uri = PackProvider.uriFor(copy);
            Intent send = new Intent(Intent.ACTION_SEND);
            send.setType(type);
            send.putExtra(Intent.EXTRA_STREAM, uri);
            send.setClipData(ClipData.newRawUri(file.getName(), uri));
            send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            activity.startActivity(Intent.createChooser(send, title));
        } catch (Exception e) {
            Toast.makeText(activity, "Не удалось: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    /* ------------------------------------------------------------ worlds */

    void showWorlds() {
        File[] worlds = newestFirst(new File(gameDir, "saves").listFiles(new java.io.FileFilter() {
            public boolean accept(File f) {
                return f.isDirectory() && new File(f, "level.dat").isFile();
            }
        }));
        if (worlds.length == 0) {
            new AlertDialog.Builder(activity).setTitle("Миры — " + inst.name).setMessage("Миров пока нет: они появятся, когда ты создашь мир в игре.")
                .setPositiveButton("OK", null).show();
            return;
        }
        LinearLayout list = new LinearLayout(activity);
        list.setOrientation(LinearLayout.VERTICAL);
        list.setPadding(dp(12), dp(8), dp(12), dp(8));
        final AlertDialog[] dialog = new AlertDialog[1];
        for (final File world : worlds) {
            LinearLayout row = new LinearLayout(activity);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setPadding(dp(10), dp(8), dp(8), dp(10));
            row.setBackground(Ui.rounded(activity, Ui.PANEL, 0, 0xFF322364));
            ImageView icon = new ImageView(activity);
            File iconFile = new File(world, "icon.png");
            if (iconFile.isFile()) {
                icon.setImageBitmap(BitmapFactory.decodeFile(iconFile.getPath()));
            } else {
                icon.setImageResource(R.drawable.install_default);
            }
            row.addView(icon, new LinearLayout.LayoutParams(dp(44), dp(44)));
            LinearLayout texts = new LinearLayout(activity);
            texts.setOrientation(LinearLayout.VERTICAL);
            texts.setPadding(dp(10), 0, dp(8), 0);
            texts.addView(Ui.label(activity, world.getName(), 15, Ui.TEXT, true));
            texts.addView(Ui.label(activity, "играли " + date(new File(world, "level.dat").lastModified()) + " · " + size(folderSize(world)),
                                   11, Ui.HINT, false));
            row.addView(texts, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            Button backup = Ui.button(activity, "Бэкап", false);
            backup.setTextSize(12);
            backup.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    backup(world);
                }
            });
            row.addView(backup);
            Button delete = Ui.button(activity, "✕", false);
            delete.setTextColor(Ui.DANGER);
            delete.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    new AlertDialog.Builder(activity).setMessage("Удалить мир «" + world.getName() + "» навсегда? Сначала можно сделать бэкап.")
                        .setPositiveButton("Удалить", new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface d, int which) {
                                deleteTree(world);
                                if (dialog[0] != null) {
                                    dialog[0].dismiss();
                                }
                                showWorlds();
                            }
                        })
                        .setNegativeButton("Отмена", null).show();
                }
            });
            LinearLayout.LayoutParams deleteParams = new LinearLayout.LayoutParams(dp(44), ViewGroup.LayoutParams.WRAP_CONTENT);
            deleteParams.leftMargin = dp(6);
            row.addView(delete, deleteParams);
            LinearLayout.LayoutParams rp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            rp.bottomMargin = dp(6);
            list.addView(row, rp);
        }
        ScrollView scroll = new ScrollView(activity);
        scroll.addView(list);
        dialog[0] = new AlertDialog.Builder(activity).setTitle("Миры — " + inst.name).setView(scroll).setNegativeButton("Закрыть", null).show();
    }

    /** the world as a zip in Downloads/Nyxora, in the background */
    private void backup(final File world) {
        Toast.makeText(activity, "Делаю бэкап «" + world.getName() + "»...", Toast.LENGTH_SHORT).show();
        activity.background(new MainActivity.Job<String>() {
            public String run() throws Exception {
                String stamp = new SimpleDateFormat("yyyy-MM-dd_HH-mm", Locale.ROOT).format(new Date());
                File zip = new File(PackProvider.exportDir(activity), safe(world.getName()) + "_" + stamp + ".zip");
                ZipOutputStream out = new ZipOutputStream(new FileOutputStream(zip));
                try {
                    addToZip(out, world, world.getName() + "/");
                } finally {
                    out.close();
                }
                String saved = activity.saveToDownloads(zip);
                return saved != null ? saved : zip.getPath();
            }

            public void done(String where, Exception error) {
                if (error != null) {
                    Toast.makeText(activity, "Бэкап не удался: " + error.getMessage(), Toast.LENGTH_LONG).show();
                } else {
                    new AlertDialog.Builder(activity).setMessage("Бэкап мира сохранён: " + where + ".\nЧтобы вернуть мир, распакуй архив в папку saves.")
                        .setPositiveButton("OK", null).show();
                }
            }
        });
    }

    private static String safe(String name) {
        return name.replaceAll("[\\\\/:*?\"<>|]", "_");
    }

    private static void addToZip(ZipOutputStream out, File file, String path) throws IOException {
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            for (int i = 0; children != null && i < children.length; i++) {
                addToZip(out, children[i], path + children[i].getName() + (children[i].isDirectory() ? "/" : ""));
            }
            return;
        }
        if (file.getName().equals("session.lock")) {
            return;  // locked while the game runs, and not needed
        }
        out.putNextEntry(new ZipEntry(path));
        InputStream in = new FileInputStream(file);
        try {
            byte[] buffer = new byte[16384];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
        } finally {
            in.close();
        }
        out.closeEntry();
    }

    static long folderSize(File file) {
        if (file.isFile()) {
            return file.length();
        }
        long total = 0;
        File[] children = file.listFiles();
        for (int i = 0; children != null && i < children.length; i++) {
            total += folderSize(children[i]);
        }
        return total;
    }

    static String size(long bytes) {
        if (bytes >= 1024L * 1024 * 1024) {
            return String.format(Locale.ROOT, "%.1f ГБ", bytes / (1024f * 1024 * 1024));
        }
        if (bytes >= 1024L * 1024) {
            return String.format(Locale.ROOT, "%.1f МБ", bytes / (1024f * 1024));
        }
        return Math.max(1, bytes / 1024) + " КБ";
    }

    static void deleteTree(File file) {
        File[] children = file.listFiles();
        for (int i = 0; children != null && i < children.length; i++) {
            deleteTree(children[i]);
        }
        file.delete();
    }

    private static void copyFile(File from, File to) throws IOException {
        InputStream in = new FileInputStream(from);
        try {
            FileOutputStream out = new FileOutputStream(to);
            try {
                byte[] buffer = new byte[16384];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                }
            } finally {
                out.close();
            }
        } finally {
            in.close();
        }
    }
}
