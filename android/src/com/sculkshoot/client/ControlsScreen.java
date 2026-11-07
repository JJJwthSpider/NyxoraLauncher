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
    private Controls.Custom selected;   // the own button being moved or edited
    private TextView selectedLabel;
    private LinearLayout selectedPanel;

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

        // the player's own buttons: any key, put anywhere on the preview
        TextView ownTitle = Ui.label(activity, "Свои кнопки", 15, Ui.ACCENT, true);
        ownTitle.setPadding(0, dp(14), 0, dp(2));
        side.addView(ownTitle);
        side.addView(Ui.label(activity, "Добавь кнопку с любой клавишей и перетащи её на превью пальцем.", 11, Ui.HINT, false));
        Button addOwn = Ui.button(activity, "+ Добавить кнопку", true);
        addOwn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                editCustom(null);
            }
        });
        LinearLayout.LayoutParams ap = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        ap.topMargin = dp(8);
        side.addView(addOwn, ap);
        selectedPanel = new LinearLayout(activity);
        selectedPanel.setOrientation(LinearLayout.VERTICAL);
        selectedPanel.setVisibility(View.GONE);
        selectedLabel = Ui.label(activity, "", 13, Ui.TEXT, false);
        selectedLabel.setPadding(0, dp(8), 0, dp(4));
        selectedPanel.addView(selectedLabel);
        LinearLayout row = new LinearLayout(activity);
        Button edit = Ui.button(activity, "Изменить", false);
        edit.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (selected != null) {
                    editCustom(selected);
                }
            }
        });
        Button delete = Ui.button(activity, "Удалить", false);
        delete.setTextColor(Ui.DANGER);
        delete.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (selected != null) {
                    controls.custom.remove(selected);
                    select(null);
                    changedSettings();
                }
            }
        });
        LinearLayout.LayoutParams half = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1);
        half.rightMargin = dp(4);
        row.addView(edit, half);
        row.addView(delete, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        selectedPanel.addView(row);
        side.addView(selectedPanel);

        if (GameEngine.get() != null) {
            // the full build: the game starts inside Lullaby with these buttons, nothing to send
            TextView built = Ui.label(activity, "Эти кнопки сами появятся в игре, когда нажмёшь «Играть».", 13, Ui.ACCENT, false);
            built.setPadding(0, dp(12), 0, 0);
            side.addView(built);
            return;
        }
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
        return slider(name, min, max, value, changed, true);
    }

    private View slider(final String name, final int min, int max, int value, final Changed changed, final boolean live) {
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
                if (live) {
                    changedSettings();
                }
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

    private void select(Controls.Custom c) {
        selected = c;
        selectedPanel.setVisibility(c == null ? View.GONE : View.VISIBLE);
        if (c != null) {
            selectedLabel.setText("Выбрана «" + c.name + "»: " + keysText(c) + (c.toggle ? ", держится" : ""));
        }
        preview.invalidate();
    }

    static String keysText(Controls.Custom c) {
        return Controls.keyName(c.key) + (c.key2 != 0 ? " + " + Controls.keyName(c.key2) : "");
    }

    /** the dialog for a new button (c == null) or an existing one: keys, label, size, hold or toggle */
    private void editCustom(final Controls.Custom existing) {
        final Controls.Custom c = new Controls.Custom();
        if (existing != null) {
            c.name = existing.name;
            c.key = existing.key;
            c.key2 = existing.key2;
            c.size = existing.size;
            c.toggle = existing.toggle;
            c.fx = existing.fx;
            c.fy = existing.fy;
        } else {
            c.key = 'R';
            c.name = "R";
        }
        final List<Object[]> keys = Controls.keyCatalogue();
        final String[] keyNames = new String[keys.size()];
        for (int i = 0; i < keys.size(); i++) {
            keyNames[i] = (String) keys.get(i)[1];
        }

        LinearLayout box = new LinearLayout(activity);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(18), dp(8), dp(18), 0);
        final Button keyButton = Ui.button(activity, "Клавиша: " + Controls.keyName(c.key), true);
        final Button key2Button = Ui.button(activity, "Вместе с: " + (c.key2 == 0 ? "ничем" : Controls.keyName(c.key2)), false);
        final android.widget.EditText name = new android.widget.EditText(activity);
        name.setSingleLine(true);
        name.setText(c.name);
        name.setHint("Подпись на кнопке");
        name.setTextColor(Ui.TEXT);
        name.setHintTextColor(Ui.HINT);
        keyButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                new AlertDialog.Builder(activity).setTitle("Какую клавишу нажимать").setItems(keyNames, new android.content.DialogInterface.OnClickListener() {
                    public void onClick(android.content.DialogInterface d, int which) {
                        String oldName = Controls.keyName(c.key);
                        c.key = (Integer) keys.get(which)[0];
                        keyButton.setText("Клавиша: " + keyNames[which]);
                        // the label follows the key until the player writes their own
                        String label = name.getText().toString().trim();
                        if (label.isEmpty() || label.equals(oldName) || label.length() <= 2) {
                            name.setText(shortName(keyNames[which]));
                        }
                    }
                }).show();
            }
        });
        key2Button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                final String[] withNone = new String[keyNames.length + 1];
                withNone[0] = "ничем (одна клавиша)";
                System.arraycopy(keyNames, 0, withNone, 1, keyNames.length);
                new AlertDialog.Builder(activity).setTitle("Нажимать вместе с (например, F3 + G)").setItems(withNone, new android.content.DialogInterface.OnClickListener() {
                    public void onClick(android.content.DialogInterface d, int which) {
                        c.key2 = which == 0 ? 0 : (Integer) keys.get(which - 1)[0];
                        key2Button.setText("Вместе с: " + (c.key2 == 0 ? "ничем" : withNone[which]));
                    }
                }).show();
            }
        });
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        bp.topMargin = dp(6);
        box.addView(keyButton, bp);
        box.addView(key2Button, new LinearLayout.LayoutParams(bp));
        box.addView(name, new LinearLayout.LayoutParams(bp));
        box.addView(slider("Размер", 60, 160, c.size, new Changed() {
            public void to(int value) {
                c.size = value;
            }
        }, false));
        CheckBox toggle = new CheckBox(activity);
        toggle.setText("Нажал — держится, нажал ещё раз — отпустилась");
        toggle.setTextColor(Ui.TEXT);
        toggle.setChecked(c.toggle);
        toggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton button, boolean checked) {
                c.toggle = checked;
            }
        });
        box.addView(toggle);
        android.widget.ScrollView scroll = new android.widget.ScrollView(activity);
        scroll.addView(box);

        new AlertDialog.Builder(activity)
            .setTitle(existing == null ? "Новая кнопка" : "Кнопка «" + existing.name + "»")
            .setView(scroll)
            .setPositiveButton(existing == null ? "Добавить" : "Сохранить", new android.content.DialogInterface.OnClickListener() {
                public void onClick(android.content.DialogInterface d, int which) {
                    c.name = name.getText().toString().trim();
                    if (c.name.isEmpty()) {
                        c.name = shortName(Controls.keyName(c.key));
                    }
                    Controls.Custom target = existing;
                    if (target == null) {
                        target = c;
                        controls.custom.add(target);
                    } else {
                        target.name = c.name;
                        target.key = c.key;
                        target.key2 = c.key2;
                        target.size = c.size;
                        target.toggle = c.toggle;
                    }
                    select(target);
                    changedSettings();
                    if (existing == null) {
                        Toast.makeText(activity, "Кнопка в центре превью — перетащи её пальцем", Toast.LENGTH_LONG).show();
                    }
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    /** a label that fits on a button: "Стрелка вверх" → "↑" */
    static String shortName(String keyName) {
        if (keyName.startsWith("Стрелка ")) {
            return keyName.endsWith("вверх") ? "↑" : keyName.endsWith("вниз") ? "↓" : keyName.endsWith("влево") ? "←" : "→";
        }
        if (keyName.contains("мыши")) {
            return keyName.startsWith("Левая") ? "ЛКМ" : keyName.startsWith("Правая") ? "ПКМ" : "СКМ";
        }
        int space = keyName.indexOf(" (");
        return space > 0 ? keyName.substring(0, space) : keyName;
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

        private float scale = 1;
        private Controls.Custom dragging;

        Preview(Context context) {
            super(context);
        }

        /** touch an own button to select it, move the finger to put it somewhere else */
        @Override
        public boolean onTouchEvent(android.view.MotionEvent e) {
            float sx = (e.getX() - rect.left) / rect.width();
            float sy = (e.getY() - rect.top) / rect.height();
            switch (e.getActionMasked()) {
                case android.view.MotionEvent.ACTION_DOWN:
                    dragging = null;
                    List<Controls.Button> all = controls.buttons();
                    for (int i = all.size() - 1; i >= 0 && dragging == null; i--) {
                        Controls.Button b = all.get(i);
                        if (b.custom == null) {
                            continue;
                        }
                        float l = rect.left + b.left(800) * scale;
                        float t = rect.top + b.top(370) * scale;
                        if (e.getX() >= l - 8 && e.getX() <= l + b.width * scale + 8 && e.getY() >= t - 8 && e.getY() <= t + b.height * scale + 8) {
                            dragging = b.custom;
                        }
                    }
                    select(dragging);
                    return dragging != null;
                case android.view.MotionEvent.ACTION_MOVE:
                    if (dragging != null) {
                        dragging.fx = Math.max(0.03f, Math.min(0.97f, sx));
                        dragging.fy = Math.max(0.05f, Math.min(0.95f, sy));
                        invalidate();
                    }
                    return dragging != null;
                case android.view.MotionEvent.ACTION_UP:
                case android.view.MotionEvent.ACTION_CANCEL:
                    if (dragging != null) {
                        changedSettings();
                    }
                    dragging = null;
                    return true;
                default:
                    return dragging != null;
            }
        }

        @Override
        protected void onDraw(Canvas canvas) {
            // a typical phone: 800 x 370 dp; fitted into the view
            float screenW = 800;
            float screenH = 370;
            scale = Math.min(getWidth() / screenW, getHeight() / screenH);
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
                boolean picked = b.custom != null && b.custom == selected;
                paint.setStrokeWidth(Math.max(1, (picked ? 3.5f : 1.5f) * scale));
                paint.setColor(picked ? 0xFFFFE36E : 0xFFFFFFFF);
                paint.setAlpha(picked ? 255 : alpha);
                canvas.drawRoundRect(r, radius, radius, paint);
                if (b.joystick) {
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(0x88FFFFFF);
                    paint.setAlpha(Math.round(0x88 * controls.opacity / 100f));
                    canvas.drawCircle(r.centerX(), r.centerY(), r.width() * 0.22f, paint);
                    continue;
                }
                text.setAlpha(alpha);
                // short labels ("R", "F5") big, long ones ("Инвентарь") smaller, so every one fits
                text.setTextSize(Math.min(r.height() * 0.42f, r.width() * 1.7f / Math.max(2, b.name.length())));
                canvas.drawText(b.name, r.centerX(), r.centerY() - (text.descent() + text.ascent()) / 2, text);
            }
        }
    }
}
