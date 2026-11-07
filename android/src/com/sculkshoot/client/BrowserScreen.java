package com.sculkshoot.client;

import android.app.AlertDialog;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Searches Modrinth for mods, resource packs or shaders that fit an installation (or for modpacks, which become
 * new installations) and installs them, like the mod browser of the desktop launcher.
 */
final class BrowserScreen {
    static final String MODPACK = "modpack";

    private static final String[][] SORTS = { { "downloads", "Популярные" }, { "relevance", "Подходящие" }, { "updated", "Обновлённые" },
                                              { "newest", "Новые" } };
    private static final int PAGE = 20;

    private final MainActivity activity;
    private final Installation inst;  // null for modpacks
    private final String kind;
    private final LinearLayout root;
    private final EditText query;
    private final Spinner sort;
    private final ListView list;
    private final TextView status;
    private final Button more;
    private final List<JSONObject> results = new ArrayList<JSONObject>();
    private int offset;
    private boolean loading;

    BrowserScreen(MainActivity activity, Installation inst, String kind) {
        this.activity = activity;
        this.inst = inst;
        this.kind = kind;
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
                goBack();
            }
        });
        bar.addView(back);
        TextView title = Ui.label(activity, title(), 15, Ui.ACCENT, true);
        title.setPadding(dp(12), 0, dp(12), 0);
        bar.addView(title);
        query = new EditText(activity);
        query.setHint("Название или тема, например: zombie, магия...");
        query.setHintTextColor(Ui.HINT);
        query.setTextColor(Ui.TEXT);
        query.setTextSize(14);
        query.setSingleLine(true);
        query.setImeOptions(EditorInfo.IME_ACTION_SEARCH);
        query.setBackground(Ui.rounded(activity, Ui.GLASS, 18, Ui.LINE));
        query.setPadding(dp(12), dp(6), dp(12), dp(6));
        query.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                search(true);
                return true;
            }
        });
        bar.addView(query, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        sort = new Spinner(activity);
        String[] names = new String[SORTS.length];
        for (int i = 0; i < SORTS.length; i++) {
            names[i] = SORTS[i][1];
        }
        sort.setAdapter(new ArrayAdapter<String>(activity, android.R.layout.simple_spinner_dropdown_item, names));
        sort.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            boolean first = true;

            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (first) {
                    first = false;
                    return;
                }
                search(true);
            }

            public void onNothingSelected(AdapterView<?> parent) {}
        });
        bar.addView(sort);
        Button find = Ui.button(activity, "Найти", true);
        find.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                search(true);
            }
        });
        bar.addView(find);
        root.addView(bar);

        status = Ui.label(activity, "", 12, Ui.HINT, false);
        status.setPadding(dp(14), dp(4), dp(14), 0);
        root.addView(status);
        list = new ListView(activity);
        list.setDividerHeight(dp(6));
        list.setDivider(new android.graphics.drawable.ColorDrawable(0));
        list.setPadding(dp(10), dp(6), dp(10), dp(10));
        list.setClipToPadding(false);
        more = Ui.button(activity, "Показать ещё", false);
        more.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                search(false);
            }
        });
        list.addFooterView(more);
        list.setAdapter(adapter);
        root.addView(list, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        search(true);
    }

    View view() {
        return root;
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    private String title() {
        if (inst == null) {
            return "Готовые сборки";
        }
        String what = Mod.RESOURCE_PACK.equals(kind) ? "Ресурспаки" : Mod.SHADER.equals(kind) ? "Шейдеры" : "Моды";
        return what + " для " + inst.minecraftVersion + " " + Plan.loaderName(inst.loader);
    }

    void goBack() {
        if (inst == null) {
            activity.showHome();
        } else {
            activity.openEditor(inst);
        }
    }

    private void search(final boolean fresh) {
        if (loading) {
            return;
        }
        loading = true;
        if (fresh) {
            offset = 0;
        }
        status.setText("Ищу на Modrinth...");
        final String text = query.getText().toString().trim();
        final String index = SORTS[Math.max(0, sort.getSelectedItemPosition())][0];
        final String facets = inst == null ? "[[\"project_type:modpack\"]]" : Installer.facetsFor(inst, kind);
        final int from = offset;
        activity.background(new MainActivity.Job<JSONArray>() {
            public JSONArray run() throws Exception {
                // Russian words are understood like Moony understands them: "зомби" searches "zombie"
                String q = Parser.hasCyrillic(text) ? Util.join(Parser.keywords(text), " ") : text;
                return Modrinth.search(q, facets, index, from, PAGE);
            }

            public void done(JSONArray hits, Exception error) {
                loading = false;
                if (error != null) {
                    status.setText("Нет связи с Modrinth: " + error.getMessage());
                    return;
                }
                if (fresh) {
                    results.clear();
                }
                for (int i = 0; hits != null && i < hits.length(); i++) {
                    results.add(hits.optJSONObject(i));
                }
                offset = from + PAGE;
                more.setVisibility(hits != null && hits.length() == PAGE ? View.VISIBLE : View.GONE);
                status.setText(results.isEmpty() ? "Ничего не нашлось. Попробуй другое слово." : "Найдено: " + results.size()
                                                     + (more.getVisibility() == View.VISIBLE ? "+" : ""));
                adapter.notifyDataSetChanged();
                if (fresh) {
                    list.setSelection(0);
                }
            }
        });
    }

    private final BaseAdapter adapter = new BaseAdapter() {
        public int getCount() {
            return results.size();
        }

        public Object getItem(int position) {
            return results.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            final JSONObject hit = results.get(position);
            LinearLayout row = new LinearLayout(activity);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setPadding(dp(10), dp(8), dp(10), dp(8));
            row.setBackground(Ui.rounded(activity, Ui.GLASS, 10, Ui.LINE));
            ImageView icon = new ImageView(activity);
            Icons.load(icon, hit.isNull("icon_url") ? "" : hit.optString("icon_url"), R.drawable.install_default);
            row.addView(icon, new LinearLayout.LayoutParams(dp(48), dp(48)));
            LinearLayout text = new LinearLayout(activity);
            text.setOrientation(LinearLayout.VERTICAL);
            text.setPadding(dp(12), 0, dp(8), 0);
            text.addView(Ui.label(activity, hit.optString("title"), 15, Ui.TEXT, false));
            TextView desc = Ui.label(activity, hit.optString("description"), 12, Ui.HINT, false);
            desc.setMaxLines(2);
            text.addView(desc);
            text.addView(Ui.label(activity, "автор: " + hit.optString("author") + " · скачиваний: " + Ui.downloads(hit.optLong("downloads")),
                                  11, Ui.ACCENT_DARK, false));
            row.addView(text, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            final boolean installed = inst != null && inst.find(hit.optString("project_id")) != null;
            Button install = Ui.button(activity, installed ? "Установлено" : "Установить", !installed);
            install.setEnabled(!installed);
            install.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    install(hit);
                }
            });
            row.addView(install);
            return row;
        }
    };

    private void install(final JSONObject hit) {
        final String projectId = hit.optString("project_id");
        final String name = hit.optString("title");
        final HomeScreen.Progress progress = new HomeScreen.Progress(activity, "Устанавливаю «" + name + "»");
        if (inst == null) {
            activity.background(new MainActivity.Job<Installation>() {
                public Installation run() throws Exception {
                    Installation pack = Installer.importModpack(projectId, activity.store(), progress);
                    activity.store().save(pack);
                    return pack;
                }

                public void done(Installation pack, Exception error) {
                    progress.dismiss();
                    if (error != null) {
                        fail(name, error);
                        return;
                    }
                    Toast.makeText(activity, "Сборка «" + pack.name + "» установлена: " + pack.describe(), Toast.LENGTH_LONG).show();
                    activity.showHome(pack.id);
                }
            });
            return;
        }
        progress.log("Ищу файл для " + inst.describe() + " и нужные библиотеки...");
        activity.background(new MainActivity.Job<List<String>>() {
            public List<String> run() throws Exception {
                List<String> added = Installer.install(inst, projectId, kind);
                activity.store().save(inst);
                return added;
            }

            public void done(List<String> added, Exception error) {
                progress.dismiss();
                if (error != null) {
                    fail(name, error);
                    return;
                }
                String message = "Установлено: " + added.get(0);
                if (added.size() > 1) {
                    message += "\nВместе с ним: " + Util.join(added.subList(1, added.size()), ", ");
                }
                Toast.makeText(activity, message, Toast.LENGTH_LONG).show();
                adapter.notifyDataSetChanged();
            }
        });
    }

    private void fail(String name, Exception error) {
        new AlertDialog.Builder(activity)
            .setTitle("Не удалось установить «" + name + "»")
            .setMessage(error instanceof Installer.InstallException ? error.getMessage() : "Нет связи с Modrinth (" + error.getMessage() + ")")
            .setPositiveButton("OK", null)
            .show();
    }
}
