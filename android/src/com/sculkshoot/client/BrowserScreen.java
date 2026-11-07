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
    /** "Для чего": Modrinth categories as quick filters, like the catalog of Millida Launcher */
    private static final String[][] PACK_GOALS = { { "", "Все" }, { "lightweight", "Для слабого телефона" }, { "multiplayer", "С друзьями" },
                                                   { "adventure", "Приключения" }, { "technology", "Технологии" }, { "magic", "Магия" },
                                                   { "quests", "Квесты" }, { "challenging", "Хардкор" }, { "optimization", "Оптимизация" } };
    private static final String[][] MOD_GOALS = { { "", "Все" }, { "optimization", "Оптимизация" }, { "adventure", "Приключения" },
                                                  { "technology", "Технологии" }, { "magic", "Магия" }, { "mobs", "Мобы" },
                                                  { "worldgen", "Мир" }, { "decoration", "Декор" }, { "food", "Еда" }, { "storage", "Хранение" },
                                                  { "utility", "Удобство" } };
    private static final String[][] SHADER_GOALS = { { "", "Все" }, { "potato", "Очень лёгкие" }, { "low", "Лёгкие" }, { "medium", "Средние" },
                                                     { "realistic", "Реалистичные" }, { "fantasy", "Фэнтези" }, { "cartoon", "Мультяшные" } };
    private static final String[][] PACK_STYLE_GOALS = { { "", "Все" }, { "16x", "16x" }, { "32x", "32x" }, { "realistic", "Реалистичные" },
                                                         { "simplistic", "Простые" }, { "cartoon", "Мультяшные" } };
    private String goal = "";  // the chosen category ("" = all)
    private final List<Button> goalButtons = new ArrayList<Button>();

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
    private boolean curseForge;  // where to search: Modrinth or CurseForge
    private Button modrinthTab;
    private Button curseForgeTab;
    private boolean loading;
    private int generation;  // a new search makes the answers of older ones stale

    BrowserScreen(MainActivity activity, Installation inst, String kind) {
        this.activity = activity;
        this.inst = inst;
        this.kind = kind;
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

        LinearLayout info = new LinearLayout(activity);
        info.setGravity(Gravity.CENTER_VERTICAL);
        info.setPadding(dp(10), dp(6), dp(10), 0);
        {
            // the same two sites as the desktop launcher, for mods and for whole modpacks
            modrinthTab = Ui.button(activity, "Modrinth", true);
            curseForgeTab = Ui.button(activity, "CurseForge", false);
            modrinthTab.setTextSize(13);
            curseForgeTab.setTextSize(13);
            modrinthTab.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    setSource(false);
                }
            });
            curseForgeTab.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    setSource(true);
                }
            });
            LinearLayout.LayoutParams tp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            tp.rightMargin = dp(6);
            info.addView(modrinthTab, tp);
            info.addView(curseForgeTab, new LinearLayout.LayoutParams(tp));
            if (inst == null) {
                // a pack file already on the phone: .mrpack (Modrinth) or .zip (CurseForge)
                Button fromFile = Ui.button(activity, "Из файла", false);
                fromFile.setTextSize(13);
                fromFile.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View v) {
                        BrowserScreen.this.activity.pickPackFile();
                    }
                });
                info.addView(fromFile, new LinearLayout.LayoutParams(tp));
            }
        }
        if (inst == null) {
            // like Millida's "Собрать с ИИ": the words of the search go to Arkan, who assembles such a pack
            Button wizard = Ui.button(activity, "Собрать с ИИ", false);
            wizard.setTextSize(13);
            wizard.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    BrowserScreen.this.activity.askWizard(query.getText().toString());
                }
            });
            info.addView(wizard, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        }
        status = Ui.label(activity, "", 12, Ui.HINT, false);
        status.setPadding(dp(8), 0, dp(8), 0);
        info.addView(status);
        root.addView(info);

        // "Для чего": quick filters by category (Modrinth)
        android.widget.HorizontalScrollView goalsScroll = new android.widget.HorizontalScrollView(activity);
        goalsScroll.setHorizontalScrollBarEnabled(false);
        LinearLayout goals = new LinearLayout(activity);
        goals.setPadding(dp(10), dp(6), dp(10), 0);
        String[][] choices = inst == null ? PACK_GOALS : Mod.SHADER.equals(kind) ? SHADER_GOALS : Mod.RESOURCE_PACK.equals(kind) ? PACK_STYLE_GOALS : MOD_GOALS;
        for (final String[] choice : choices) {
            final Button chip = Ui.button(activity, choice[1], choice[0].isEmpty());
            chip.setTextSize(12);
            chip.setMinHeight(dp(32));
            chip.setMinimumHeight(dp(32));
            chip.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    goal = choice[0];
                    for (Button other : goalButtons) {
                        Ui.styleButton(BrowserScreen.this.activity, other, other == chip);
                    }
                    if (curseForge && !goal.isEmpty()) {
                        setSource(false);  // the categories are Modrinth's
                    } else {
                        search(true);
                    }
                }
            });
            goalButtons.add(chip);
            LinearLayout.LayoutParams cp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            cp.rightMargin = dp(6);
            goals.addView(chip, cp);
        }
        goalsScroll.addView(goals);
        root.addView(goalsScroll);
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

    void setSource(boolean useCurseForge) {
        if (curseForge == useCurseForge) {
            return;
        }
        curseForge = useCurseForge;
        Ui.styleButton(activity, modrinthTab, !curseForge);
        Ui.styleButton(activity, curseForgeTab, curseForge);
        search(true);
    }

    private void search(final boolean fresh) {
        if (loading && !fresh) {
            return;  // "show more" waits for the running search; a new search replaces it
        }
        loading = true;
        final int mine = ++generation;
        if (fresh) {
            offset = 0;
        }
        final boolean cf = curseForge;
        final String site = cf ? "CurseForge" : "Modrinth";
        status.setText("Ищу на " + site + "...");
        final String text = query.getText().toString().trim();
        final String index = SORTS[Math.max(0, sort.getSelectedItemPosition())][0];
        String baseFacets = inst == null ? "[[\"project_type:modpack\"]]" : Installer.facetsFor(inst, kind);
        if (!goal.isEmpty() && baseFacets.endsWith("]]")) {
            baseFacets = baseFacets.substring(0, baseFacets.length() - 1) + ",[\"categories:" + goal + "\"]]";
        }
        final String facets = baseFacets;
        final int from = offset;
        activity.background(new MainActivity.Job<JSONArray>() {
            public JSONArray run() throws Exception {
                // Russian words are understood like Moony understands them: "зомби" searches "zombie"
                String q = Parser.hasCyrillic(text) ? Util.join(Parser.keywords(text), " ") : text;
                return cf ? CurseForge.search(q, inst, kind, index, from, PAGE) : Modrinth.search(q, facets, index, from, PAGE);
            }

            public void done(JSONArray hits, Exception error) {
                if (mine != generation) {
                    return;
                }
                loading = false;
                if (error != null) {
                    status.setText("Нет связи с " + site + ": " + error.getMessage());
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
            Icons.load(icon, hit.isNull("icon_url") ? "" : hit.optString("icon_url"));
            row.addView(icon, new LinearLayout.LayoutParams(dp(48), dp(48)));
            LinearLayout text = new LinearLayout(activity);
            text.setOrientation(LinearLayout.VERTICAL);
            text.setPadding(dp(12), 0, dp(8), 0);
            text.addView(Ui.label(activity, hit.optString("title"), 15, Ui.TEXT, false));
            TextView desc = Ui.label(activity, hit.optString("description"), 12, Ui.HINT, false);
            desc.setMaxLines(2);
            text.addView(desc);
            text.addView(Ui.label(activity, "автор: " + hit.optString("author") + " · скачиваний: " + Ui.downloads(hit.optLong("downloads"))
                                  + (CurseForge.isCurseForge(hit.optString("project_id")) ? " · CurseForge" : ""),
                                  11, Ui.ACCENT_DARK, false));
            row.addView(text, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            final boolean installed = inst != null && (inst.find(hit.optString("project_id")) != null
                                                       || (!hit.optString("slug").isEmpty() && Installer.findBySlug(inst, hit.optString("slug")) != null));
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
                    Toast.makeText(activity, "Сборка «" + pack.name + "» установлена: " + pack.describe() + disabledNote(pack), Toast.LENGTH_LONG).show();
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

    /** files of a CurseForge pack that could not be downloaded */
    static String disabledNote(Installation pack) {
        int off = 0;
        for (Mod mod : pack.mods) {
            off += !mod.enabled && mod.url.isEmpty() ? 1 : 0;
        }
        return off == 0 ? "" : "\nНе скачать без приложения CurseForge: " + off + " (они выключены, см. «Моды и ресурсы»)";
    }

    private void fail(String name, Exception error) {
        new AlertDialog.Builder(activity)
            .setTitle("Не удалось установить «" + name + "»")
            .setMessage(error instanceof Installer.InstallException ? error.getMessage() : "Нет связи (" + error.getMessage() + ")")
            .setPositiveButton("OK", null)
            .show();
    }
}
