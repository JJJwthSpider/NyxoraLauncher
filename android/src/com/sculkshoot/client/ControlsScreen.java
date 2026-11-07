package com.sculkshoot.client;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.net.Uri;
import android.text.Html;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.util.List;

/**
 * "Управление": the touch controls the game gets, shown on a phone-shaped preview. The joystick walks; the buttons
 * attack (ЛКМ), use and place (ПКМ), jump, sneak, switch the item, open the inventory, drop, change the camera (F5),
 * open the chat and the on-screen keyboard. "Отправить в лаунчер" hands the layout to PojavLauncher (or a launcher
 * built on it), which imports it.
 */
final class ControlsScreen {
    /** launchers that import PojavLauncher layouts (name, package) */
    static final String[][] POJAV_FAMILY = {
        { "PojavLauncher", "net.kdt.pojavlaunch" },
        { "Amethyst", "org.angelauramc.amethyst" },
        { "Zalith Launcher", "com.movtery.zalithlauncher" },
    };
    static final String FILE_NAME = "Lullaby_controls.json";

    private final MainActivity activity;
    private final LinearLayout root;
    private final Controls controls;
    private final Preview preview;

    ControlsScreen(MainActivity activity) {
        this.activity = activity;
        controls = Controls.load(activity);
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new MoonSkyDrawable(activity.getResources().getDisplayMetrics().density));

        LinearLayout bar = new LinearLayout(activity);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(8), dp(6), dp(8), dp(6));
        bar.setBackgroundColor(0xCC0D0E16);
        Button back = Ui.button(activity, "← Назад", false);
        back.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                ControlsScreen.this.activity.showHome();
            }
        });
        bar.addView(back);
        TextView title = Ui.label(activity, "Управление в игре", 17, Ui.ACCENT, true);
        title.setPadding(dp(12), 0, 0, 0);
        bar.addView(title, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        root.addView(bar);

        LinearLayout body = new LinearLayout(activity);
        root.addView(body, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        preview = new Preview(activity);
        LinearLayout.LayoutParams pp = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1);
        pp.setMargins(dp(12), dp(12), dp(12), dp(12));
        body.addView(preview, pp);

        ScrollView sideScroll = new ScrollView(activity);
        sideScroll.setBackgroundColor(0x990D0E16);
        LinearLayout side = new LinearLayout(activity);
        side.setOrientation(LinearLayout.VERTICAL);
        side.setPadding(dp(12), dp(10), dp(12), dp(10));
        sideScroll.addView(side);
        body.addView(sideScroll, new LinearLayout.LayoutParams(dp(250), ViewGroup.LayoutParams.MATCH_PARENT));

        side.addView(Ui.label(activity, "Джойстик слева — ходить (потяни вверх за край — бежать вперёд). "
                                        + "Справа — ЛКМ, ПКМ, прыжок, присесть и смена вещи. Сверху — пауза, чат, камера (F5), "
                                        + "инвентарь, выбросить, клавиатура и мышь.", 12, Ui.HINT, false));
        side.addView(slider("Размер кнопок", 70, 140, controls.size, new Changed() {
            public void to(int value) {
                controls.size = value;
            }
        }));
        side.addView(slider("Прозрачность", 30, 100, controls.opacity, new Changed() {
            public void to(int value) {
                controls.opacity = value;
            }
        }));
        side.addView(check("Для левши (джойстик справа)", controls.leftHanded, new Changed() {
            public void to(int value) {
                controls.leftHanded = value == 1;
            }
        }));
        side.addView(check("«Присесть» нажал — и сидишь", controls.sneakToggle, new Changed() {
            public void to(int value) {
                controls.sneakToggle = value == 1;
            }
        }));
        side.addView(check("«Бег» нажал — и бежишь", controls.sprintToggle, new Changed() {
            public void to(int value) {
                controls.sprintToggle = value == 1;
            }
        }));

        Button send = Ui.button(activity, "Отправить в лаунчер", true);
        send.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                sendToLauncher();
            }
        });
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        bp.topMargin = dp(12);
        side.addView(send, bp);
        Button save = Ui.button(activity, "Сохранить в Загрузки", false);
        save.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                saveToDownloads();
            }
        });
        side.addView(save, new LinearLayout.LayoutParams(bp));
    }

    View view() {
        return root;
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    interface Changed {
        void to(int value);
    }

    private View slider(final String name, final int min, int max, int value, final Changed changed) {
        LinearLayout box = new LinearLayout(activity);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(0, dp(10), 0, 0);
        final TextView label = Ui.label(activity, name + ": " + value + "%", 13, Ui.TEXT, false);
        box.addView(label);
        SeekBar bar = new SeekBar(activity);
        bar.setMax((max - min) / 5);
        bar.setProgress((value - min) / 5);
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                int v = min + progress * 5;
                label.setText(name + ": " + v + "%");
                changed.to(v);
                changedSettings();
            }

            public void onStartTrackingTouch(SeekBar seekBar) {}

            public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        box.addView(bar);
        return box;
    }

    private View check(String name, boolean value, final Changed changed) {
        CheckBox box = new CheckBox(activity);
        box.setText(name);
        box.setTextColor(Ui.TEXT);
        box.setTextSize(13);
        box.setChecked(value);
        box.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton button, boolean checked) {
                changed.to(checked ? 1 : 0);
                changedSettings();
            }
        });
        return box;
    }

    private void changedSettings() {
        controls.save(activity);
        preview.invalidate();
    }

    /* ------------------------------------------------------------ to the launcher */

    /** the layout file in the app's export folder (the content provider shares it) */
    File writeFile() throws Exception {
        File file = new File(PackProvider.exportDir(activity), FILE_NAME);
        FileOutputStream out = new FileOutputStream(file);
        try {
            out.write(controls.toPojavJson().getBytes("UTF-8"));
        } finally {
            out.close();
        }
        return file;
    }

    private void sendToLauncher() {
        File file;
        try {
            file = writeFile();
        } catch (Exception e) {
            Toast.makeText(activity, "Не удалось создать файл: " + e.getMessage(), Toast.LENGTH_LONG).show();
            return;
        }
        Uri uri = PackProvider.uriFor(file);
        for (String[] launcher : POJAV_FAMILY) {
            Intent open = new Intent(Intent.ACTION_VIEW);
            open.setDataAndType(uri, "application/json");
            open.setPackage(launcher[1]);
            open.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (open.resolveActivity(activity.getPackageManager()) != null) {
                activity.startActivity(open);
                Toast.makeText(activity, launcher[0] + ": нажми «Импорт», затем в настройках управления выбери «Lullaby_controls»",
                               Toast.LENGTH_LONG).show();
                return;
            }
        }
        // no launcher that imports layouts by itself: the file goes to Downloads with the steps
        saveToDownloads();
    }

    private void saveToDownloads() {
        File file;
        try {
            file = writeFile();
        } catch (Exception e) {
            Toast.makeText(activity, "Не удалось создать файл: " + e.getMessage(), Toast.LENGTH_LONG).show();
            return;
        }
        String saved = activity.saveToDownloads(file);
        String where = saved != null ? "Файл сохранён: <b>" + Util.escape(saved) + "</b>" : "Не удалось сохранить файл в Загрузки.";
        new AlertDialog.Builder(activity)
            .setTitle("Как включить управление")
            .setMessage(Html.fromHtml(where + "<br><br>В PojavLauncher (или Amethyst): открой файл через «Файлы» → «Открыть с помощью» "
                                      + "→ лаунчер и нажми «Импорт». Потом в лаунчере: <b>Настройки → Управление</b> и выбери "
                                      + "<b>Lullaby_controls</b>.<br><br>Fold Craft Launcher читает только свои схемы управления — "
                                      + "в нём кнопки настраиваются в его собственном редакторе."))
            .setPositiveButton("OK", null)
            .show();
    }

    /* ------------------------------------------------------------ the preview */

    /** a phone screen in landscape with a bit of Minecraft sky, grass and the hotbar, and the buttons on top */
    private final class Preview extends View {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF rect = new RectF();

        Preview(Context context) {
            super(context);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            // a typical phone: 800 x 370 dp; fitted into the view
            float screenW = 800;
            float screenH = 370;
            float scale = Math.min(getWidth() / screenW, getHeight() / screenH);
            float ox = (getWidth() - screenW * scale) / 2;
            float oy = (getHeight() - screenH * scale) / 2;
            rect.set(ox, oy, ox + screenW * scale, oy + screenH * scale);
            paint.setStyle(Paint.Style.FILL);
            paint.setShader(new LinearGradient(0, rect.top, 0, rect.bottom, new int[] { 0xFF78A7FF, 0xFFB5D3FF, 0xFF5E9E3A, 0xFF4C7F2E },
                                               new float[] { 0, 0.55f, 0.56f, 1 }, Shader.TileMode.CLAMP));
            canvas.drawRoundRect(rect, 14 * scale, 14 * scale, paint);
            paint.setShader(null);
            // the hotbar in the middle at the bottom
            float slot = 20 * scale;
            float hotbarLeft = rect.centerX() - slot * 4.5f;
            for (int i = 0; i < 9; i++) {
                paint.setColor(i == 0 ? 0xDDFFFFFF : 0x88303030);
                canvas.drawRect(hotbarLeft + i * slot + 1, rect.bottom - slot - 4 * scale, hotbarLeft + (i + 1) * slot - 1, rect.bottom - 4 * scale, paint);
            }
            // the crosshair
            paint.setColor(0xCCFFFFFF);
            canvas.drawRect(rect.centerX() - 6 * scale, rect.centerY() - scale, rect.centerX() + 6 * scale, rect.centerY() + scale, paint);
            canvas.drawRect(rect.centerX() - scale, rect.centerY() - 6 * scale, rect.centerX() + scale, rect.centerY() + 6 * scale, paint);

            List<Controls.Button> buttons = controls.buttons();
            int alpha = Math.round(255 * controls.opacity / 100f);
            Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
            text.setColor(0xFFFFFFFF);
            text.setTextAlign(Paint.Align.CENTER);
            for (Controls.Button b : buttons) {
                float l = rect.left + b.left(screenW) * scale;
                float t = rect.top + b.top(screenH) * scale;
                RectF r = new RectF(l, t, l + b.width * scale, t + b.height * scale);
                float radius = b.round || b.joystick ? Math.min(r.width(), r.height()) / 2 : Math.min(r.width(), r.height()) * 0.3f;
                paint.setStyle(Paint.Style.FILL);
                paint.setColor(b.joystick ? 0x33000000 : 0x66000000);
                paint.setAlpha(Math.round(paint.getAlpha() * controls.opacity / 100f));
                canvas.drawRoundRect(r, radius, radius, paint);
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(Math.max(1, 1.5f * scale));
                paint.setColor(0xFFFFFFFF);
                paint.setAlpha(alpha);
                canvas.drawRoundRect(r, radius, radius, paint);
                if (b.joystick) {
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(0x88FFFFFF);
                    paint.setAlpha(Math.round(0x88 * controls.opacity / 100f));
                    canvas.drawCircle(r.centerX(), r.centerY(), r.width() * 0.22f, paint);
                    continue;
                }
                text.setAlpha(alpha);
                text.setTextSize(Math.min(r.height() * 0.36f, r.width() * 0.2f));
                canvas.drawText(b.name, r.centerX(), r.centerY() - (text.descent() + text.ascent()) / 2, text);
            }
        }
    }
}
