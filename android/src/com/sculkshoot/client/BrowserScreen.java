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
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
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
    private static final String FAVORITES = "★";  // the goal that shows the favorites instead of a search
    /** "Для чего": Modrinth categories as quick filters, like the catalog of Millida Launcher */
    private static final String[][] PACK_GOALS = { { "", "Все" }, { FAVORITES, "★ Избранное" }, { "lightweight", "Для слабого телефона" }, { "multiplayer", "С друзьями" },
                                                   { "adventure", "Приключения" }, { "technology", "Технологии" }, { "magic", "Магия" },
                                                   { "quests", "Квесты" }, { "challenging", "Хардкор" }, { "optimization", "Оптимизация" } };
    private static final String[][] MOD_GOALS = { { "", "Все" }, { FAVORITES, "★ Избранное" }, { "optimization", "Оптимизация" }, { "adventure", "Приключения" },
                                                  { "technology", "Технологии" }, { "magic", "Магия" }, { "mobs", "Мобы" },
                                                  { "worldgen", "Мир" }, { "decoration", "Декор" }, { "food", "Еда" }, { "storage", "Хранение" },
                                                  { "utility", "Удобство" } };
    private static final String[][] SHADER_GOALS = { { "", "Все" }, { FAVORITES, "★ Избранное" }, { "potato", "Очень лёгкие" }, { "low", "Лёгкие" }, { "medium", "Средние" },
                                                     { "realistic", "Реалистичные" }, { "fantasy", "Фэнтези" }, { "cartoon", "Мультяшные" } };
    private static final String[][] PACK_STYLE_GOALS = { { "", "Все" }, { FAVORITES, "★ Избранное" }, { "16x", "16x" }, { "32x", "32x" }, { "realistic", "Реалистичные" },
                                                         { "simplistic", "Простые" }, { "cartoon", "Мультяшные" } };
    private String goal = "";  // the chosen category ("" = all)
    private final List<Button> goalButtons = new ArrayList<Button>();

    private final MainActivity activity;
    private final Installation inst;  // null for modpacks
    private final String kind;
    private final LinearLayout root;
    private final EditText query;
    private final Spinner sort;
    private final GridView list;  // the results as cards, like Millida's catalog
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

        // the top bar like Millida's: for the packs catalog "‹ Лобби", the tabs, import and a new pack
        LinearLayout bar = new LinearLayout(activity);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(10), dp(6), dp(10), dp(6));
        bar.setBackgroundColor(0xF0020208);
        Button back = Ui.button(activity, inst == null ? "‹ Лобби" : "‹ Назад", true);
        back.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                goBack();
            }
        });
        bar.addView(back);
        bar.addView(new View(activity), new LinearLayout.LayoutParams(0, 1, 1));
        if (inst == null) {
            Button library = Ui.button(activity, "📦 Библиотека", false);
            library.setTextSize(14);
            library.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    BrowserScreen.this.activity.showLibrary();
                }
            });
            bar.addView(library);
            Button catalog = Ui.button(activity, "📖 Каталог", true);
            catalog.setTextSize(14);
            LinearLayout.LayoutParams cp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            cp.leftMargin = dp(4);
            bar.addView(catalog, cp);
            Button create = Ui.button(activity, "✚ Создать", false);
            create.setTextSize(14);
            create.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    BrowserScreen.this.activity.showCreate();
                }
            });
            bar.addView(create, new LinearLayout.LayoutParams(cp));
        } else {
            bar.addView(Ui.label(activity, title(), 16, Ui.TEXT, true));
        }
        bar.addView(new View(activity), new LinearLayout.LayoutParams(0, 1, 1));
        if (inst == null) {
            Button fromFile = Ui.button(activity, "⬇ Импорт", false);
            fromFile.setTextSize(13);
            fromFile.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    BrowserScreen.this.activity.pickPackFile();
                }
            });
            bar.addView(fromFile);
        }
        root.addView(bar);

        // the heading and the search, like Millida's "Сборки модов для Minecraft  23 588 материалов"
        LinearLayout heading = new LinearLayout(activity);
        heading.setGravity(Gravity.BOTTOM);
        heading.setPadding(dp(14), dp(10), dp(14), dp(4));
        heading.addView(Ui.label(activity, inst == null ? "Сборки модов для Minecraft" : title(), 20, Ui.TEXT, true));
        status = Ui.label(activity, "", 12, Ui.HINT, false);
        status.setPadding(dp(10), 0, 0, dp(2));
        heading.addView(status);
        root.addView(heading);
        LinearLayout searchRow = new LinearLayout(activity);
        searchRow.setGravity(Gravity.CENTER_VERTICAL);
        searchRow.setPadding(dp(10), dp(4), dp(10), dp(4));
        query = new EditText(activity);
        query.setHint(inst == null ? "Сборка, тема или просьба к ИИ..." : "Название или тема, например: zombie, магия...");
        query.setHintTextColor(Ui.HINT);
        query.setTextColor(Ui.TEXT);
        query.setTextSize(14);
        query.setSingleLine(true);
        query.setImeOptions(EditorInfo.IME_ACTION_SEARCH);
        query.setBackground(Ui.rounded(activity, 0xE6060514, 0, 0xFF322364));
        query.setPadding(dp(12), dp(8), dp(12), dp(8));
        query.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                search(true);
                return true;
            }
        });
        searchRow.addView(query, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button find = Ui.button(activity, "Найти", true);
        find.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                search(true);
            }
        });
        LinearLayout.LayoutParams fp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        fp.leftMargin = dp(6);
        searchRow.addView(find, fp);
        if (inst == null) {
            // like Millida's "Собрать с ИИ": the words of the search go to Arkan, who assembles such a pack
            Button wizard = Ui.button(activity, "🔮 Собрать с ИИ", false);
            wizard.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    BrowserScreen.this.activity.askWizard(query.getText().toString());
                }
            });
            searchRow.addView(wizard, new LinearLayout.LayoutParams(fp));
        }
        root.addView(searchRow);

        LinearLayout body = new LinearLayout(activity);
        root.addView(body, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        // left: the filters, like Millida's catalog
        android.widget.ScrollView filtersScroll = new android.widget.ScrollView(activity);
        LinearLayout filters = new LinearLayout(activity);
        filters.setOrientation(LinearLayout.VERTICAL);
        filters.setPadding(dp(10), dp(8), dp(10), dp(10));
        filters.setBackground(Ui.rounded(activity, 0xD9060410, 0, 0xFF110E2B));
        filters.addView(filterTitle("Сортировка"));
        sort = new Spinner(activity);
        String[] names = new String[SORTS.length];
        for (int i = 0; i < SORTS.length; i++) {
            names[i] = SORTS[i][1];
        }
        sort.setAdapter(new ArrayAdapter<String>(activity, android.R.layout.simple_spinner_dropdown_item, names));
        sort.setBackground(Ui.rounded(activity, 0xFF110E2B, 0, 0xFF322364));
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
        filters.addView(sort);
        filters.addView(filterTitle("Площадка"));
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
        LinearLayout.LayoutParams full = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        full.bottomMargin = dp(4);
        filters.addView(modrinthTab, full);
        filters.addView(curseForgeTab, new LinearLayout.LayoutParams(full));
        filters.addView(filterTitle("Для чего"));
        String[][] choices = inst == null ? PACK_GOALS : Mod.SHADER.equals(kind) ? SHADER_GOALS : Mod.RESOURCE_PACK.equals(kind) ? PACK_STYLE_GOALS : MOD_GOALS;
        for (final String[] choice : choices) {
            final Button chip = Ui.button(activity, choice[1], choice[0].isEmpty());
            chip.setTextSize(12);
            chip.setMinHeight(dp(34));
            chip.setMinimumHeight(dp(34));
            chip.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    goal = choice[0];
                    Sounds.play(Sounds.CLICK);
                    for (Button other : goalButtons) {
                        Ui.styleButton(BrowserScreen.this.activity, other, other == chip);
                    }
                    if (curseForge && !goal.isEmpty() && !FAVORITES.equals(goal)) {
                        setSource(false);  // the categories are Modrinth's
                    } else {
                        search(true);
                    }
                }
            });
            goalButtons.add(chip);
            filters.addView(chip, new LinearLayout.LayoutParams(full));
        }
        filtersScroll.addView(filters);
        LinearLayout.LayoutParams filtersParams = new LinearLayout.LayoutParams(dp(200), ViewGroup.LayoutParams.MATCH_PARENT);
        filtersParams.setMargins(dp(10), dp(4), 0, dp(10));
        body.addView(filtersScroll, filtersParams);

        // right: the results as cards in a grid
        LinearLayout right = new LinearLayout(activity);
        right.setOrientation(LinearLayout.VERTICAL);
        list = new GridView(activity);
        list.setNumColumns(GridView.AUTO_FIT);
        list.setColumnWidth(dp(220));
        list.setStretchMode(GridView.STRETCH_COLUMN_WIDTH);
        list.setHorizontalSpacing(dp(8));
        list.setVerticalSpacing(dp(8));
        list.setPadding(dp(10), dp(4), dp(10), dp(10));
        list.setClipToPadding(false);
        list.setSelector(new android.graphics.drawable.ColorDrawable(0));
        list.setAdapter(adapter);
        list.setOnScrollListener(new android.widget.AbsListView.OnScrollListener() {
            public void onScrollStateChanged(android.widget.AbsListView view, int state) {}

            public void onScroll(android.widget.AbsListView view, int first, int visible, int total) {
                // like Millida's catalog: more come by themselves at the end
                if (total > 0 && first + visible >= total - 2 && more.getVisibility() == View.VISIBLE && !loading) {
                    search(false);
                }
            }
        });
        right.addView(list, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        more = Ui.button(activity, "Показать ещё", false);
        more.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                search(false);
            }
        });
        more.setVisibility(View.GONE);
        LinearLayout.LayoutParams moreParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        moreParams.setMargins(dp(10), 0, dp(10), dp(6));
        right.addView(more, moreParams);
        body.addView(right, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        search(true);
    }

    /** opens with these words already searched (from "Рекомендуем" in the library) */
    void searchFor(String text) {
        query.setText(text);
        search(true);
    }

    View view() {
        return root;
    }

    private TextView filterTitle(String text) {
        TextView t = Ui.label(activity, text, 12, 0xFFB493FF, true);
        t.setPadding(dp(2), dp(10), 0, dp(4));
        return t;
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
            activity.showLobby();
        } else {
            activity.openEditor(inst);
        }
    }

    void setSource(boolean useCurseForge) {
        if (curseForge == useCurseForge) {
            return;
        }
        curseForge = useCurseForge;
        Sounds.play(Sounds.NAV);
        Ui.styleButton(activity, modrinthTab, !curseForge);
        Ui.styleButton(activity, curseForgeTab, curseForge);
        search(true);
    }

    /* ------------------------------------------------------------ favorites, like Millida's stars */

    private String favoritesKey() {
        return (inst == null ? MODPACK : kind) + "";
    }

    private List<JSONObject> favorites() {
        List<JSONObject> out = new ArrayList<JSONObject>();
        try {
            JSONArray saved = new JSONArray(activity.getSharedPreferences("favorites", android.content.Context.MODE_PRIVATE).getString(favoritesKey(), "[]"));
            for (int i = 0; i < saved.length(); i++) {
                out.add(saved.getJSONObject(i));
            }
        } catch (org.json.JSONException e) {
            // a broken list starts again empty
        }
        return out;
    }

    private boolean isFavorite(JSONObject hit) {
        for (JSONObject f : favorites()) {
            if (f.optString("project_id").equals(hit.optString("project_id"))) {
                return true;
            }
        }
        return false;
    }

    private void toggleFavorite(JSONObject hit) {
        List<JSONObject> list = favorites();
        boolean removed = false;
        for (int i = list.size() - 1; i >= 0; i--) {
            if (list.get(i).optString("project_id").equals(hit.optString("project_id"))) {
                list.remove(i);
                removed = true;
            }
        }
        if (!removed) {
            list.add(0, hit);
        }
        JSONArray out = new JSONArray();
        for (JSONObject f : list) {
            out.put(f);
        }
        activity.getSharedPreferences("favorites", android.content.Context.MODE_PRIVATE).edit().putString(favoritesKey(), out.toString()).apply();
        if (FAVORITES.equals(goal)) {
            search(true);
        } else {
            adapter.notifyDataSetChanged();
        }
    }

    private void search(final boolean fresh) {
        if (FAVORITES.equals(goal)) {
            loading = false;
            generation++;
            results.clear();
            results.addAll(favorites());
            more.setVisibility(View.GONE);
            status.setText(results.isEmpty() ? "В избранном пусто: нажми ☆ у сборки или мода" : "В избранном: " + results.size());
            adapter.notifyDataSetChanged();
            return;
        }
        if (loading && !fresh) {
            return;  // "show more" waits for the running search; a new search replaces it
        }
        loading = true;
        final int mine = ++generation;
        if (fresh) {
            offset = 0;
        }
        if (fresh) {
            list.animate().cancel();
            list.animate().alpha(0.35f).setDuration(150).start();
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
                list.animate().cancel();
                if (fresh) {
                    list.setTranslationY(Ui.dp(activity, 10));
                }
                list.animate().alpha(1f).translationY(0).setDuration(220).start();
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
            // a card like Millida's: the picture on top, the name, downloads and author, the description, the buttons
            LinearLayout card = new LinearLayout(activity);
            card.setOrientation(LinearLayout.VERTICAL);
            card.setPadding(dp(6), dp(6), dp(6), dp(9));
            card.setBackground(PixelDrawable.plain(activity.getResources().getDisplayMetrics().density));
            android.widget.FrameLayout banner = new android.widget.FrameLayout(activity);
            banner.setBackgroundColor(0xFF110E2B);
            ImageView icon = new ImageView(activity);
            Icons.load(icon, hit.isNull("icon_url") ? "" : hit.optString("icon_url"));
            banner.addView(icon, new android.widget.FrameLayout.LayoutParams(dp(64), dp(64), Gravity.CENTER));
            card.addView(banner, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(84)));
            TextView name = Ui.label(activity, hit.optString("title"), 14, Ui.TEXT, true);
            name.setSingleLine(true);
            name.setEllipsize(android.text.TextUtils.TruncateAt.END);
            name.setPadding(dp(2), dp(6), dp(2), 0);
            card.addView(name);
            TextView meta = Ui.label(activity, "⬇ " + Ui.downloads(hit.optLong("downloads")) + " · " + hit.optString("author")
                                                   + (CurseForge.isCurseForge(hit.optString("project_id")) ? " · CurseForge" : ""), 11, Ui.HINT, false);
            meta.setSingleLine(true);
            meta.setPadding(dp(2), 0, dp(2), 0);
            card.addView(meta);
            TextView desc = Ui.label(activity, hit.optString("description"), 12, 0xFFCDC7E0, false);
            desc.setMaxLines(2);
            desc.setMinLines(2);
            desc.setEllipsize(android.text.TextUtils.TruncateAt.END);
            desc.setPadding(dp(2), dp(4), dp(2), dp(6));
            card.addView(desc);
            LinearLayout buttons = new LinearLayout(activity);
            final boolean installed = inst != null && (inst.find(hit.optString("project_id")) != null
                                                       || (!hit.optString("slug").isEmpty() && Installer.findBySlug(inst, hit.optString("slug")) != null));
            Button install = Ui.button(activity, installed ? "Установлено" : "Установить", !installed);
            install.setTextSize(13);
            install.setEnabled(!installed);
            install.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    install(hit);
                }
            });
            buttons.addView(install, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            final boolean favorite = isFavorite(hit);
            Button star = Ui.button(activity, favorite ? "★" : "☆", favorite);
            star.setTextSize(17);
            star.setContentDescription(favorite ? "Убрать из избранного" : "В избранное");
            star.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    toggleFavorite(hit);
                }
            });
            LinearLayout.LayoutParams starParams = new LinearLayout.LayoutParams(dp(48), ViewGroup.LayoutParams.WRAP_CONTENT);
            starParams.leftMargin = dp(6);
            buttons.addView(star, starParams);
            card.addView(buttons);
            return card;
        }
    };

    private void install(final JSONObject hit) {
        final String projectId = hit.optString("project_id");
        final String name = hit.optString("title");
        if (inst == null) {
            activity.installModpack(projectId, name);
            return;
        }
        final HomeScreen.Progress progress = new HomeScreen.Progress(activity, "Устанавливаю «" + name + "»");
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
