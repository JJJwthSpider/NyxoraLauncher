package com.sculkshoot.client;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

/**
 * Mods, resource packs and shaders of one installation, like the instance editor of the desktop launcher:
 * switch them on and off, remove them, or add new ones from Modrinth.
 */
final class EditorScreen {
    private final MainActivity activity;
    private final Installation inst;
    private final LinearLayout root;
    private final LinearLayout tabs;
    private final ListView list;
    private final TextView summary;
    private final Button add;
    private String kind = Mod.MOD;
    private final List<Mod> rows = new ArrayList<Mod>();

    private static final String[][] TABS = {
        { Mod.MOD, "Моды", "Добавить моды" },
        { Mod.RESOURCE_PACK, "Ресурспаки", "Добавить ресурспаки" },
        { Mod.SHADER, "Шейдеры", "Добавить шейдеры" },
        { Mod.FILE, "Файлы сборки", "" },
    };

    EditorScreen(MainActivity activity, Installation inst) {
        this.activity = activity;
        this.inst = inst;
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new RainDrawable(activity.getResources().getDisplayMetrics().density));

        LinearLayout bar = new LinearLayout(activity);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(8), dp(6), dp(8), dp(6));
        bar.setBackgroundColor(0xCC140509);
        Button back = Ui.button(activity, "← Назад", false);
        back.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                EditorScreen.this.activity.showHome();
            }
        });
        bar.addView(back);
        LinearLayout titles = new LinearLayout(activity);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.setPadding(dp(12), 0, 0, 0);
        titles.addView(Ui.label(activity, inst.name, 17, Ui.ACCENT, true));
        titles.addView(Ui.label(activity, inst.describe() + " " + inst.loaderVersion, 12, Ui.HINT, false));
        bar.addView(titles, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        tabs = new LinearLayout(activity);
        bar.addView(tabs);
        root.addView(bar);

        LinearLayout body = new LinearLayout(activity);
        root.addView(body, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        list = new ListView(activity);
        list.setDividerHeight(dp(6));
        list.setDivider(new android.graphics.drawable.ColorDrawable(0));
        list.setPadding(dp(10), dp(10), dp(10), dp(10));
        list.setClipToPadding(false);
        list.setAdapter(adapter);
        body.addView(list, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));

        LinearLayout side = new LinearLayout(activity);
        side.setOrientation(LinearLayout.VERTICAL);
        side.setPadding(dp(10), dp(10), dp(10), dp(10));
        side.setBackgroundColor(0x99140509);
        summary = Ui.label(activity, "", 12, Ui.HINT, false);
        side.addView(summary);
        add = Ui.button(activity, "Добавить моды", true);
        add.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                EditorScreen.this.activity.openBrowser(EditorScreen.this.inst, kind);
            }
        });
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.topMargin = dp(10);
        side.addView(add, p);
        Button play = Ui.button(activity, "Играть", false);
        play.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                EditorScreen.this.activity.playInstallation(EditorScreen.this.inst);
            }
        });
        side.addView(play, new LinearLayout.LayoutParams(p));
        Button update = Ui.button(activity, "Проверить обновления", false);
        update.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                checkUpdates();
            }
        });
        side.addView(update, new LinearLayout.LayoutParams(p));
        TextView hint = Ui.label(activity, "Библиотеки, нужные модам, ставятся и убираются сами. Снятая галочка — мод не попадёт в игру.",
                                 11, Ui.HINT, false);
        hint.setPadding(0, dp(12), 0, 0);
        side.addView(hint);
        body.addView(side, new LinearLayout.LayoutParams(dp(220), ViewGroup.LayoutParams.MATCH_PARENT));

        showTab(Mod.MOD);
        fillIcons();
    }

    /** mods from modpacks (or made before icons were kept) get their Modrinth icons once */
    private void fillIcons() {
        boolean missing = false;
        for (Mod mod : inst.mods) {
            missing |= mod.iconUrl.isEmpty() && !mod.projectId.isEmpty() && !CurseForge.isCurseForge(mod.projectId);
        }
        if (!missing) {
            return;
        }
        activity.background(new MainActivity.Job<Boolean>() {
            public Boolean run() throws Exception {
                return Installer.fillIcons(inst);
            }

            public void done(Boolean changed, Exception error) {
                if (error == null && changed != null && changed) {
                    activity.saveInstallation(inst);
                    showTab(kind);
                }
            }
        });
    }

    View view() {
        return root;
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    void showTab(String newKind) {
        if (kind != null && !kind.equals(newKind) && list != null) {
            // another tab: the list slides in with a blip, like Millida's tabs
            Sounds.play(Sounds.NAV);
            list.animate().cancel();
            list.setAlpha(0f);
            list.setTranslationX(Ui.dp(activity, 24));
            list.animate().alpha(1f).translationX(0).setDuration(240).setInterpolator(new android.view.animation.DecelerateInterpolator(1.6f)).start();
        }
        kind = newKind;
        tabs.removeAllViews();
        for (final String[] tab : TABS) {
            int count = inst.count(tab[0]);
            if (Mod.FILE.equals(tab[0]) && count == 0) {
                continue;  // only modpacks have other files (configs, worlds...)
            }
            Button b = Ui.button(activity, tab[1] + " (" + count + ")", tab[0].equals(kind));
            b.setTextSize(13);
            b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    showTab(tab[0]);
                }
            });
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            p.leftMargin = dp(6);
            tabs.addView(b, p);
            if (tab[0].equals(kind)) {
                add.setText(tab[2]);
                add.setVisibility(tab[2].isEmpty() ? View.GONE : View.VISIBLE);
            }
        }
        rows.clear();
        // the user's own choices first, the libraries after them
        for (Mod mod : inst.mods) {
            if (kind.equals(mod.kind) && !mod.dependency) {
                rows.add(mod);
            }
        }
        for (Mod mod : inst.mods) {
            if (kind.equals(mod.kind) && mod.dependency) {
                rows.add(mod);
            }
        }
        int enabled = 0;
        for (Mod mod : rows) {
            enabled += mod.enabled ? 1 : 0;
        }
        summary.setText(rows.isEmpty() ? "Здесь пока пусто." : "Всего: " + rows.size() + ", включено: " + enabled);
        adapter.notifyDataSetChanged();
    }

    private final BaseAdapter adapter = new BaseAdapter() {
        public int getCount() {
            return rows.size();
        }

        public Object getItem(int position) {
            return rows.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            final Mod mod = rows.get(position);
            LinearLayout row = new LinearLayout(activity);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setPadding(dp(10), dp(8), dp(8), dp(8));
            row.setBackground(Ui.rounded(activity, Ui.GLASS, 10, Ui.LINE));
            ImageView icon = new ImageView(activity);
            Icons.load(icon, mod.iconUrl);
            row.addView(icon, new LinearLayout.LayoutParams(dp(40), dp(40)));
            LinearLayout text = new LinearLayout(activity);
            text.setOrientation(LinearLayout.VERTICAL);
            text.setPadding(dp(10), 0, dp(6), 0);
            text.addView(Ui.label(activity, mod.title + (mod.versionNumber.isEmpty() ? "" : "  " + mod.versionNumber), 14, Ui.TEXT, false));
            String detail = mod.dependency ? "библиотека" + (mod.neededBy.isEmpty() ? "" : " для " + Util.join(mod.neededBy, ", "))
                                           : mod.description.isEmpty() ? mod.targetPath() : mod.description;
            TextView d = Ui.label(activity, detail, 11, Ui.HINT, false);
            d.setMaxLines(2);
            text.addView(d);
            row.addView(text, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            if (!mod.dependency) {
                CheckBox on = new CheckBox(activity);
                on.setChecked(mod.enabled);
                on.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    public void onCheckedChanged(CompoundButton button, boolean checked) {
                        mod.enabled = checked;
                        activity.saveInstallation(inst);
                        showTab(kind);
                    }
                });
                row.addView(on);
                Button remove = Ui.button(activity, "✕", false);
                remove.setTextColor(Ui.DANGER);
                remove.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View v) {
                        confirmRemove(mod);
                    }
                });
                row.addView(remove, new LinearLayout.LayoutParams(dp(44), dp(40)));
            }
            return row;
        }
    };

    /** newest versions of everything from Modrinth and CurseForge, like the desktop launcher's update check */
    private void checkUpdates() {
        final HomeScreen.Progress progress = new HomeScreen.Progress(activity, "Обновления для «" + inst.name + "»");
        progress.log("Ищу новые версии на Modrinth и CurseForge...");
        activity.background(new MainActivity.Job<List<String>>() {
            public List<String> run() throws Exception {
                return Installer.updateAll(inst, progress);
            }

            public void done(List<String> changes, Exception error) {
                progress.dismiss();
                if (error != null) {
                    new AlertDialog.Builder(activity).setMessage("Не удалось проверить обновления: " + error.getMessage()).setPositiveButton("OK", null).show();
                    return;
                }
                if (!changes.isEmpty()) {
                    activity.saveInstallation(inst);
                    showTab(kind);
                }
                new AlertDialog.Builder(activity)
                    .setTitle(changes.isEmpty() ? "Всё уже новое" : "Обновлено: " + changes.size())
                    .setMessage(changes.isEmpty() ? "Для " + inst.describe() + " новее версий нет." : Util.join(changes, "\n"))
                    .setPositiveButton("OK", null)
                    .show();
            }
        });
    }

    private void confirmRemove(final Mod mod) {
        new AlertDialog.Builder(activity)
            .setMessage("Убрать «" + mod.title + "» из установки?")
            .setPositiveButton("Убрать", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    Installer.remove(inst, mod);
                    activity.saveInstallation(inst);
                    showTab(kind);
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }
}
