package com.sculkshoot.client;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

/**
 * The main screen, like the main window of the desktop launcher: the tiles on the left (Moony, ready packs, new
 * installation), the installations in the middle and what to do with the selected one on the right.
 */
final class HomeScreen {
    private final MainActivity activity;
    private final LinearLayout root;
    private final GridView grid;
    private final TextView empty;
    private final EditText search;
    private final LinearLayout actions;
    private final List<Installation> all = new ArrayList<Installation>();
    private final List<Installation> shown = new ArrayList<Installation>();
    private String selectedId;

    HomeScreen(MainActivity activity) {
        this.activity = activity;
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new MoonSkyDrawable(activity.getResources().getDisplayMetrics().density));

        // top bar: name, search, menu
        LinearLayout bar = new LinearLayout(activity);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(12), dp(6), dp(8), dp(6));
        bar.setBackgroundColor(0xCC0D0E16);
        ImageView logo = new ImageView(activity);
        logo.setImageResource(R.drawable.install_default);
        bar.addView(logo, new LinearLayout.LayoutParams(dp(34), dp(34)));
        TextView title = Ui.label(activity, "Lullaby Launcher", 18, Ui.ACCENT, true);
        title.setPadding(dp(10), 0, dp(16), 0);
        bar.addView(title);
        search = new EditText(activity);
        search.setHint("Найти установку...");
        search.setHintTextColor(Ui.HINT);
        search.setTextColor(Ui.TEXT);
        search.setTextSize(14);
        search.setSingleLine(true);
        search.setBackground(Ui.rounded(activity, Ui.GLASS, 18, Ui.LINE));
        search.setPadding(dp(12), dp(6), dp(12), dp(6));
        search.addTextChangedListener(new TextWatcher() {
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            public void afterTextChanged(Editable s) {
                filter();
            }
        });
        bar.addView(search, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button chats = Ui.button(activity, "Чаты", false);
        chats.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.showChatList();
            }
        });
        LinearLayout.LayoutParams barButton = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        barButton.leftMargin = dp(8);
        bar.addView(chats, barButton);
        Button more = Ui.button(activity, "⋮", false);
        more.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.showAppMenu();
            }
        });
        bar.addView(more, new LinearLayout.LayoutParams(dp(44), ViewGroup.LayoutParams.WRAP_CONTENT));
        root.addView(bar);

        LinearLayout body = new LinearLayout(activity);
        root.addView(body, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        // left: the three tiles of the desktop launcher
        LinearLayout tiles = new LinearLayout(activity);
        tiles.setOrientation(LinearLayout.VERTICAL);
        tiles.setPadding(dp(10), dp(10), dp(4), dp(10));
        tiles.addView(tile(R.drawable.moony, "ИИ-агент Лунни", "Сборка по описанию", new Runnable() {
            public void run() {
                HomeScreen.this.activity.openChat(null);
            }
        }), tileParams());
        tiles.addView(tile(R.drawable.tile_packs, "Готовые сборки", "Модпаки с Modrinth", new Runnable() {
            public void run() {
                HomeScreen.this.activity.openBrowser(null, BrowserScreen.MODPACK);
            }
        }), tileParams());
        tiles.addView(tile(R.drawable.tile_new, "Новая установка", "Версия, загрузчик и моды", new Runnable() {
            public void run() {
                newInstallation();
            }
        }), tileParams());
        body.addView(tiles, new LinearLayout.LayoutParams(dp(170), ViewGroup.LayoutParams.MATCH_PARENT));

        // middle: the installations
        LinearLayout middle = new LinearLayout(activity);
        middle.setOrientation(LinearLayout.VERTICAL);
        grid = new GridView(activity);
        grid.setNumColumns(GridView.AUTO_FIT);
        grid.setColumnWidth(dp(150));
        grid.setStretchMode(GridView.STRETCH_COLUMN_WIDTH);
        grid.setHorizontalSpacing(dp(8));
        grid.setVerticalSpacing(dp(8));
        grid.setPadding(dp(8), dp(10), dp(8), dp(10));
        grid.setClipToPadding(false);
        grid.setSelector(new android.graphics.drawable.ColorDrawable(0));
        grid.setAdapter(adapter);
        grid.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                select(shown.get(position).id);
            }
        });
        empty = Ui.label(activity, "Привет! Чтобы начать играть, нажми «Новая установка», выбери «Готовые сборки» или попроси Лунни собрать сборку.",
                         16, Ui.TEXT, true);
        empty.setGravity(Gravity.CENTER);
        empty.setPadding(dp(18), dp(14), dp(18), dp(14));
        empty.setBackground(Ui.rounded(activity, Ui.GLASS, 12, Ui.ACCENT));
        LinearLayout.LayoutParams emptyParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        emptyParams.setMargins(dp(24), dp(40), dp(24), 0);
        middle.addView(empty, emptyParams);
        middle.addView(grid, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        body.addView(middle, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));

        // right: what to do with the selected installation
        ScrollView side = new ScrollView(activity);
        actions = new LinearLayout(activity);
        actions.setOrientation(LinearLayout.VERTICAL);
        actions.setPadding(dp(10), dp(10), dp(10), dp(10));
        side.addView(actions);
        side.setBackgroundColor(0x990D0E16);
        body.addView(side, new LinearLayout.LayoutParams(dp(210), ViewGroup.LayoutParams.MATCH_PARENT));
    }

    View view() {
        return root;
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    private LinearLayout.LayoutParams tileParams() {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1);
        p.bottomMargin = dp(8);
        return p;
    }

    private View tile(int picture, String title, String subtitle, final Runnable action) {
        LinearLayout tile = new LinearLayout(activity);
        tile.setOrientation(LinearLayout.VERTICAL);
        tile.setGravity(Gravity.CENTER);
        tile.setPadding(dp(6), dp(6), dp(6), dp(6));
        tile.setBackground(Ui.rounded(activity, Ui.GLASS, 12, 0xCCFFFFFF));
        ImageView image = new ImageView(activity);
        image.setImageResource(picture);
        tile.addView(image, new LinearLayout.LayoutParams(dp(54), dp(54)));
        TextView t = Ui.label(activity, title, 14, Ui.TEXT, true);
        t.setGravity(Gravity.CENTER);
        tile.addView(t);
        TextView s = Ui.label(activity, subtitle, 11, Ui.HINT, false);
        s.setGravity(Gravity.CENTER);
        tile.addView(s);
        tile.setClickable(true);
        tile.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                action.run();
            }
        });
        return tile;
    }

    /* ------------------------------------------------------------ the list */

    void refresh() {
        all.clear();
        all.addAll(activity.store().list());
        if (selectedId == null && !all.isEmpty()) {
            selectedId = all.get(0).id;
        }
        filter();
    }

    void select(String id) {
        selectedId = id;
        adapter.notifyDataSetChanged();
        updateActions();
    }

    private Installation selected() {
        for (Installation inst : all) {
            if (inst.id.equals(selectedId)) {
                return inst;
            }
        }
        return null;
    }

    private void filter() {
        String query = search.getText().toString().trim().toLowerCase(Locale.ROOT);
        shown.clear();
        for (Installation inst : all) {
            if (query.isEmpty() || inst.name.toLowerCase(Locale.ROOT).contains(query)) {
                shown.add(inst);
            }
        }
        empty.setVisibility(all.isEmpty() ? View.VISIBLE : View.GONE);
        adapter.notifyDataSetChanged();
        updateActions();
    }

    private final BaseAdapter adapter = new BaseAdapter() {
        public int getCount() {
            return shown.size();
        }

        public Object getItem(int position) {
            return shown.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            Installation inst = shown.get(position);
            LinearLayout card = new LinearLayout(activity);
            card.setOrientation(LinearLayout.VERTICAL);
            card.setGravity(Gravity.CENTER_HORIZONTAL);
            card.setPadding(dp(8), dp(10), dp(8), dp(10));
            boolean selected = inst.id.equals(selectedId);
            card.setBackground(selected ? Ui.rainbowFrame(activity, 12) : Ui.rounded(activity, Ui.GLASS, 12, Ui.LINE));
            ImageView icon = new ImageView(activity);
            Icons.load(icon, inst.iconUrl, inst.chatId.isEmpty() ? R.drawable.install_default : R.drawable.moony);
            card.addView(icon, new LinearLayout.LayoutParams(dp(56), dp(56)));
            TextView name = Ui.label(activity, inst.name, 14, Ui.TEXT, true);
            name.setGravity(Gravity.CENTER);
            name.setMaxLines(2);
            card.addView(name);
            TextView sub = Ui.label(activity, inst.minecraftVersion + " · " + Plan.loaderName(inst.loader), 11, Ui.HINT, false);
            sub.setGravity(Gravity.CENTER);
            card.addView(sub);
            TextView count = Ui.label(activity, "модов: " + inst.count(Mod.MOD), 11, Ui.HINT, false);
            count.setGravity(Gravity.CENTER);
            card.addView(count);
            return card;
        }
    };

    /* ------------------------------------------------------------ actions on the right */

    private void updateActions() {
        actions.removeAllViews();
        final Installation inst = selected();
        if (inst == null) {
            TextView hint = Ui.label(activity, "Выбери установку, чтобы играть или менять моды.", 13, Ui.HINT, false);
            actions.addView(hint);
            return;
        }
        TextView name = Ui.label(activity, inst.name, 16, Ui.TEXT, true);
        actions.addView(name);
        TextView sub = Ui.label(activity, inst.describe() + " " + inst.loaderVersion + "\nмодов: " + inst.count(Mod.MOD) + ", ресурспаков: "
                                    + inst.count(Mod.RESOURCE_PACK) + ", шейдеров: " + inst.count(Mod.SHADER),
                                12, Ui.HINT, false);
        sub.setPadding(0, dp(2), 0, dp(8));
        actions.addView(sub);

        Button play = Ui.button(activity, "Играть", true);
        play.setTextSize(18);
        play.setMinHeight(dp(48));
        play.setMinimumHeight(dp(48));
        play.setOnClickListener(click(new Runnable() {
            public void run() {
                activity.playInstallation(inst);
            }
        }));
        actions.addView(play, actionParams());
        actions.addView(action("Моды и ресурсы", new Runnable() {
            public void run() {
                activity.openEditor(inst);
            }
        }), actionParams());
        if (!inst.chatId.isEmpty()) {
            actions.addView(action("Чат с Лунни", new Runnable() {
                public void run() {
                    activity.openChat(inst.chatId);
                }
            }), actionParams());
        }
        actions.addView(action("Переименовать", new Runnable() {
            public void run() {
                rename(inst);
            }
        }), actionParams());
        actions.addView(action("Копировать", new Runnable() {
            public void run() {
                copy(inst);
            }
        }), actionParams());
        actions.addView(action("Экспорт .mrpack", new Runnable() {
            public void run() {
                activity.exportInstallation(inst);
            }
        }), actionParams());
        Button delete = action("Удалить", new Runnable() {
            public void run() {
                delete(inst);
            }
        });
        delete.setTextColor(Ui.DANGER);
        actions.addView(delete, actionParams());
    }

    private LinearLayout.LayoutParams actionParams() {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.bottomMargin = dp(6);
        return p;
    }

    private Button action(String text, Runnable run) {
        Button b = Ui.button(activity, text, false);
        b.setOnClickListener(click(run));
        return b;
    }

    private static View.OnClickListener click(final Runnable run) {
        return new View.OnClickListener() {
            public void onClick(View v) {
                run.run();
            }
        };
    }

    private void rename(final Installation inst) {
        final EditText name = new EditText(activity);
        name.setText(inst.name);
        name.setSelectAllOnFocus(true);
        new AlertDialog.Builder(activity)
            .setTitle("Название установки")
            .setView(name)
            .setPositiveButton("Сохранить", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    String text = name.getText().toString().trim();
                    if (!text.isEmpty()) {
                        inst.name = text;
                        activity.saveInstallation(inst);
                        refresh();
                    }
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    private void copy(Installation inst) {
        Installation copy = inst.copy(inst.name + " (копия)");
        try {
            if (inst.hasSourcePack) {
                java.io.File from = activity.store().sourcePack(inst.id);
                java.io.FileInputStream in = new java.io.FileInputStream(from);
                java.io.FileOutputStream out = new java.io.FileOutputStream(activity.store().sourcePack(copy.id));
                try {
                    byte[] buffer = new byte[65536];
                    int n;
                    while ((n = in.read(buffer)) > 0) {
                        out.write(buffer, 0, n);
                    }
                } finally {
                    in.close();
                    out.close();
                }
            }
            activity.store().save(copy);
            selectedId = copy.id;
            refresh();
        } catch (Exception e) {
            Toast.makeText(activity, "Не удалось скопировать: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    private void delete(final Installation inst) {
        new AlertDialog.Builder(activity)
            .setMessage("Удалить установку «" + inst.name + "»? Чат с Лунни, если он есть, останется.")
            .setPositiveButton("Удалить", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    activity.store().delete(inst.id);
                    selectedId = null;
                    refresh();
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    /* ------------------------------------------------------------ new installation */

    private void newInstallation() {
        LinearLayout form = new LinearLayout(activity);
        form.setOrientation(LinearLayout.VERTICAL);
        form.setPadding(dp(20), dp(8), dp(20), 0);
        final EditText name = new EditText(activity);
        name.setHint("Название");
        name.setText("Моя установка");
        name.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
        form.addView(name);
        form.addView(Ui.label(activity, "Версия Minecraft", 13, Ui.HINT, false));
        final Spinner version = new Spinner(activity);
        final List<String> versions = new ArrayList<String>(Arrays.asList("1.21.1", "1.20.1", "1.19.2", "1.18.2", "1.16.5", "1.12.2"));
        final ArrayAdapter<String> versionAdapter = new ArrayAdapter<String>(activity, android.R.layout.simple_spinner_dropdown_item, versions);
        version.setAdapter(versionAdapter);
        form.addView(version);
        form.addView(Ui.label(activity, "Загрузчик модов", 13, Ui.HINT, false));
        final RadioGroup loaders = new RadioGroup(activity);
        loaders.setOrientation(RadioGroup.HORIZONTAL);
        final String[] loaderIds = { "neoforge", "forge", "fabric", "quilt" };
        for (int i = 0; i < loaderIds.length; i++) {
            RadioButton r = new RadioButton(activity);
            r.setText(Plan.loaderName(loaderIds[i]));
            r.setId(1000 + i);
            loaders.addView(r);
        }
        loaders.check(1000);
        form.addView(loaders);
        final CheckBox essentials = new CheckBox(activity);
        essentials.setText("Сразу добавить оптимизацию и удобство (Sodium, JEI, Jade...)");
        essentials.setChecked(true);
        form.addView(essentials);

        // the full list of Minecraft versions comes from Modrinth
        activity.background(new MainActivity.Job<List<String>>() {
            public List<String> run() throws Exception {
                return Modrinth.gameVersions();
            }

            public void done(List<String> list, Exception error) {
                if (list != null && !list.isEmpty()) {
                    versions.clear();
                    versions.addAll(list);
                    versionAdapter.notifyDataSetChanged();
                    int i = list.indexOf("1.21.1");
                    version.setSelection(Math.max(0, i));
                }
            }
        });

        new AlertDialog.Builder(activity)
            .setTitle("Новая установка")
            .setView(form)
            .setPositiveButton("Создать", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    String title = name.getText().toString().trim();
                    String mc = String.valueOf(version.getSelectedItem());
                    String loader = loaderIds[Math.max(0, loaders.getCheckedRadioButtonId() - 1000)];
                    create(title.isEmpty() ? "Моя установка" : title, mc, loader, essentials.isChecked());
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    private void create(final String name, final String mc, final String loader, final boolean essentials) {
        final Progress progress = new Progress(activity, "Создаю «" + name + "»");
        activity.background(new MainActivity.Job<Installation>() {
            public Installation run() throws Exception {
                Installation inst = Installer.create(name, mc, loader, essentials, progress);
                activity.store().save(inst);
                return inst;
            }

            public void done(Installation inst, Exception error) {
                progress.dismiss();
                if (error != null) {
                    new AlertDialog.Builder(activity).setTitle("Не получилось").setMessage(error.getMessage()).setPositiveButton("OK", null).show();
                    return;
                }
                selectedId = inst.id;
                refresh();
                Toast.makeText(activity, "Установка «" + inst.name + "» готова. Добавь моды в «Моды и ресурсы».", Toast.LENGTH_LONG).show();
            }
        });
    }

    /** A small window with a spinner and the last log line; also a Builder listener. */
    static final class Progress implements Builder.Listener {
        private final AlertDialog dialog;
        private final TextView status;
        private final MainActivity activity;

        Progress(MainActivity activity, String title) {
            this.activity = activity;
            LinearLayout box = new LinearLayout(activity);
            box.setOrientation(LinearLayout.VERTICAL);
            box.setPadding(Ui.dp(activity, 20), Ui.dp(activity, 10), Ui.dp(activity, 20), Ui.dp(activity, 10));
            ProgressBar bar = new ProgressBar(activity, null, android.R.attr.progressBarStyleHorizontal);
            bar.setIndeterminate(true);
            box.addView(bar);
            status = Ui.label(activity, "Подожди немного...", 13, Ui.HINT, false);
            box.addView(status);
            dialog = new AlertDialog.Builder(activity).setTitle(title).setView(box).setCancelable(false).show();
        }

        public void log(final String line) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    status.setText(line);
                }
            });
        }

        void dismiss() {
            dialog.dismiss();
        }
    }
}
