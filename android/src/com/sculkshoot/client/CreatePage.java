package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * The "Создать" tab: a new installation made the way Millida Launcher's "Новая сборка" makes one (the name, the loader as
 * tiles with a sign, the Minecraft version as a grid with a search and a "snapshots" switch, "Буст FPS", "Собрать с ИИ"
 * and "Создать"), and next to it the ready packs: ideas Arkan assembles and the most popular modpacks, installed with
 * one tap.
 */
final class CreatePage {
    /** the loaders like Millida's tiles: id, name, what it is for */
    private static final String[][] LOADERS = { { "fabric", "Fabric", "лёгкий, быстрый" }, { "neoforge", "NeoForge", "новые моды" },
                                                { "forge", "Forge", "классика" }, { "quilt", "Quilt", "как Fabric" }, { "vanilla", "Vanilla", "без модов" } };
    private static final String[] FALLBACK_VERSIONS = { "1.21.4", "1.21.1", "1.20.6", "1.20.4", "1.20.1", "1.19.4", "1.19.2", "1.18.2", "1.17.1",
                                                        "1.16.5", "1.15.2", "1.14.4", "1.12.2", "1.8.9", "1.7.10" };
    /** "Начни с идеи": what Arkan is asked to assemble */
    private static final String[][] IDEAS = { { "🤝 С друзьями", "сборка для выживания с друзьями" }, { "📱 Для слабого телефона", "лёгкая сборка для слабого телефона, много FPS" },
                                              { "⚙ Техно", "технологии и автоматизация, заводы" }, { "🔮 Магия", "магия и заклинания" },
                                              { "🗡 Приключения", "приключения, данжи, боссы, RPG" }, { "👻 Хоррор", "страшная хоррор сборка" },
                                              { "🌄 Красивая графика", "красивая графика с шейдерами" }, { "🧟 Зомби", "выживание в зомби апокалипсисе" } };

    private final MainActivity activity;
    private final HomeScreen home;
    private final LinearLayout root;
    private final EditText name;
    private final EditText versionSearch;
    private final FlowLayout versionGrid;
    private final List<View> loaderTiles = new ArrayList<View>();
    private final Button snapshots;
    private final Button fps;
    private final Button essentials;
    private final Button create;
    private final LinearLayout packs;
    private final TextView packsTitle;
    private final List<String[]> versions = new ArrayList<String[]>();  // {version, type}
    private boolean showSnapshots;
    private boolean nameTouched;
    private boolean settingName;
    private boolean fpsOn = true;
    private boolean essentialsOn = true;
    private String version = "1.21.1";
    private String loader = "fabric";

    CreatePage(MainActivity activity, HomeScreen home) {
        this.activity = activity;
        this.home = home;
        float density = activity.getResources().getDisplayMetrics().density;
        root = new LinearLayout(activity);
        // the form scrolls, "Создать" stays at the bottom of the panel
        LinearLayout leftPanel = new LinearLayout(activity);
        leftPanel.setOrientation(LinearLayout.VERTICAL);
        leftPanel.setBackground(PixelDrawable.panel(0xF0140509, 0xFF3A121B, density));
        ScrollView leftScroll = new ScrollView(activity);
        LinearLayout left = new LinearLayout(activity);
        left.setOrientation(LinearLayout.VERTICAL);
        left.setPadding(dp(14), dp(10), dp(14), dp(6));
        leftScroll.addView(left);
        leftPanel.addView(leftScroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        LinearLayout.LayoutParams leftParams = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 13);
        leftParams.setMargins(dp(10), dp(8), dp(5), dp(8));
        root.addView(leftPanel, leftParams);

        // the icon and the name, like Millida's head of the window
        LinearLayout head = new LinearLayout(activity);
        head.setGravity(Gravity.CENTER_VERTICAL);
        head.setPadding(0, 0, 0, dp(2));
        ImageView icon = new ImageView(activity);
        icon.setImageResource(R.drawable.install_default);
        icon.setBackground(PixelDrawable.panel(0xFF3A0D16, 0xFF7A1F2E, density));
        icon.setPadding(dp(4), dp(4), dp(4), dp(4));
        head.addView(icon, new LinearLayout.LayoutParams(dp(48), dp(48)));
        LinearLayout nameColumn = new LinearLayout(activity);
        nameColumn.setOrientation(LinearLayout.VERTICAL);
        nameColumn.setPadding(dp(10), 0, 0, 0);
        nameColumn.addView(caption("Новая сборка · название"));
        name = new EditText(activity);
        name.setTextColor(Ui.TEXT);
        name.setHintTextColor(Ui.HINT);
        name.setTextSize(15);
        name.setSingleLine(true);
        name.setBackground(Ui.rounded(activity, 0xFF0E0407, 0, 0xFF5A1E2A));
        name.setPadding(dp(10), dp(7), dp(10), dp(7));
        name.addTextChangedListener(new TextWatcher() {
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            public void afterTextChanged(Editable s) {
                if (!settingName) {
                    nameTouched = true;
                }
            }
        });
        nameColumn.addView(name, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        head.addView(nameColumn, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        left.addView(head);

        // the loader: tiles with a sign, a loader that does not exist for the version is dimmed and says why
        left.addView(caption("Загрузчик"));
        LinearLayout loaders = new LinearLayout(activity);
        for (final String[] l : LOADERS) {
            LinearLayout tile = new LinearLayout(activity);
            tile.setOrientation(LinearLayout.VERTICAL);
            tile.setGravity(Gravity.CENTER_HORIZONTAL);
            tile.setPadding(dp(2), dp(5), dp(2), dp(6));
            tile.addView(new LoaderSign(activity, l[0]), new LinearLayout.LayoutParams(dp(24), dp(20)));
            TextView t = Ui.label(activity, l[1], 12, Ui.TEXT, true);
            t.setGravity(Gravity.CENTER);
            tile.addView(t);
            TextView sub = Ui.label(activity, l[2], 9, Ui.HINT, false);
            sub.setGravity(Gravity.CENTER);
            sub.setSingleLine(true);
            tile.addView(sub);
            tile.setTag(l[0]);
            tile.setClickable(true);
            tile.setContentDescription(l[1]);
            tile.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    if (why(l[0], version) == null) {
                        loader = l[0];
                        update();
                    }
                }
            });
            loaderTiles.add(tile);
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(0, dp(68), 1);
            p.rightMargin = dp(4);
            loaders.addView(tile, p);
        }
        left.addView(loaders);

        // the version: a search, the "snapshots" switch and the grid
        LinearLayout versionHead = new LinearLayout(activity);
        versionHead.setGravity(Gravity.CENTER_VERTICAL);
        versionHead.setPadding(0, dp(10), 0, dp(4));
        TextView versionCaption = caption("Версия Minecraft");
        versionHead.addView(versionCaption, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        versionSearch = new EditText(activity);
        versionSearch.setHint("🔍 Найти");
        versionSearch.setHintTextColor(Ui.HINT);
        versionSearch.setTextColor(Ui.TEXT);
        versionSearch.setTextSize(12);
        versionSearch.setSingleLine(true);
        versionSearch.setBackground(Ui.rounded(activity, 0xFF0E0407, 0, 0xFF5A1E2A));
        versionSearch.setPadding(dp(8), dp(4), dp(8), dp(4));
        versionSearch.addTextChangedListener(new TextWatcher() {
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            public void afterTextChanged(Editable s) {
                fillVersions();
            }
        });
        versionHead.addView(versionSearch, new LinearLayout.LayoutParams(dp(110), ViewGroup.LayoutParams.WRAP_CONTENT));
        snapshots = Ui.button(activity, "Снапшоты", false);
        snapshots.setTextSize(11);
        snapshots.setMinHeight(dp(32));
        snapshots.setMinimumHeight(dp(32));
        snapshots.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showSnapshots = !showSnapshots;
                Ui.styleButton(CreatePage.this.activity, snapshots, showSnapshots);
                fillVersions();
            }
        });
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        sp.leftMargin = dp(6);
        versionHead.addView(snapshots, sp);
        left.addView(versionHead);
        versionGrid = new FlowLayout(activity, dp(5));
        left.addView(versionGrid);

        // the options
        LinearLayout options = new LinearLayout(activity);
        options.setPadding(0, dp(10), 0, 0);
        fps = Ui.button(activity, "", true);
        fps.setTextSize(12);
        fps.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                fpsOn = !fpsOn;
                update();
            }
        });
        options.addView(fps, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        essentials = Ui.button(activity, "", true);
        essentials.setTextSize(12);
        essentials.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                essentialsOn = !essentialsOn;
                update();
            }
        });
        LinearLayout.LayoutParams ep = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1);
        ep.leftMargin = dp(6);
        options.addView(essentials, ep);
        left.addView(options);

        // the foot: "Собрать с ИИ" and "Создать"
        LinearLayout foot = new LinearLayout(activity);
        foot.setPadding(dp(12), dp(6), dp(12), dp(10));
        foot.setBackgroundColor(0xF0100406);
        Button ai = Ui.button(activity, "🔮 Собрать с ИИ", false);
        ai.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                CreatePage.this.activity.askWizard("Собери сборку на Minecraft " + version + ("vanilla".equals(loader) ? "" : " " + Plan.loaderName(loader)));
            }
        });
        foot.addView(ai, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        create = Ui.button(activity, "Создать", true);
        create.setTextSize(18);
        create.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                String title = name.getText().toString().trim();
                CreatePage.this.home.create(title.isEmpty() ? autoName() : title, version, loader, essentialsOn && !"vanilla".equals(loader),
                                            fpsOn && !"vanilla".equals(loader));
            }
        });
        LinearLayout.LayoutParams cp = new LinearLayout.LayoutParams(0, dp(52), 1);
        cp.leftMargin = dp(6);
        foot.addView(create, cp);
        leftPanel.addView(foot);

        /* ---------------- the ready packs */
        ScrollView rightScroll = new ScrollView(activity);
        LinearLayout right = new LinearLayout(activity);
        right.setOrientation(LinearLayout.VERTICAL);
        right.setPadding(dp(12), dp(10), dp(12), dp(12));
        right.setBackground(PixelDrawable.panel(0xF0140509, 0xFF3A121B, density));
        rightScroll.addView(right);
        LinearLayout.LayoutParams rightParams = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 10);
        rightParams.setMargins(dp(5), dp(8), dp(10), dp(8));
        root.addView(rightScroll, rightParams);
        right.addView(Ui.label(activity, "Готовые сборки", 20, Ui.TEXT, true));
        TextView ideasCaption = caption("Начни с идеи — Аркан соберёт под твою версию");
        ideasCaption.setPadding(0, dp(8), 0, dp(4));
        right.addView(ideasCaption);
        FlowLayout ideas = new FlowLayout(activity, dp(5));
        for (final String[] idea : IDEAS) {
            Button b = Ui.button(activity, idea[0], false);
            b.setTextSize(12);
            b.setMinHeight(dp(34));
            b.setMinimumHeight(dp(34));
            b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    CreatePage.this.activity.askWizard("Собери " + idea[1] + ", Minecraft " + version
                                                       + ("vanilla".equals(loader) ? "" : ", " + Plan.loaderName(loader)));
                }
            });
            ideas.addView(b);
        }
        right.addView(ideas);
        LinearLayout packsHead = new LinearLayout(activity);
        packsHead.setGravity(Gravity.CENTER_VERTICAL);
        packsHead.setPadding(0, dp(10), 0, dp(4));
        packsTitle = caption("Популярные сборки");
        packsHead.addView(packsTitle, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button all = Ui.button(activity, "Все в каталоге ›", false);
        all.setTextSize(11);
        all.setMinHeight(dp(32));
        all.setMinimumHeight(dp(32));
        all.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                CreatePage.this.activity.openBrowser(null, BrowserScreen.MODPACK);
            }
        });
        packsHead.addView(all);
        right.addView(packsHead);
        packs = new LinearLayout(activity);
        packs.setOrientation(LinearLayout.VERTICAL);
        right.addView(packs);

        for (String v : FALLBACK_VERSIONS) {
            versions.add(new String[] { v, "release" });
        }
        update();
        loadVersions();
        loadPacks();
    }

    View view() {
        return root;
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    private TextView caption(String text) {
        TextView t = Ui.label(activity, text, 12, 0xFFFF8090, true);
        t.setPadding(0, dp(4), 0, dp(4));
        return t;
    }

    /** opens the tab fresh: a new name for the chosen version */
    void reset() {
        nameTouched = false;
        update();
    }

    /** "Сборка 1.21.1", or "Сборка 1.21.1 (2)" when there is one */
    private String autoName() {
        String base = "Сборка " + version;
        List<String> names = new ArrayList<String>();
        for (Installation inst : activity.store().list()) {
            names.add(inst.name);
        }
        if (!names.contains(base)) {
            return base;
        }
        for (int i = 2;; i++) {
            if (!names.contains(base + " (" + i + ")")) {
                return base + " (" + i + ")";
            }
        }
    }

    /** {major, minor, patch} of a release like 1.20.1; null for snapshots */
    private static int[] parts(String v) {
        String[] p = v.split("\\.");
        if (p.length < 2) {
            return null;
        }
        try {
            return new int[] { Integer.parseInt(p[0]), Integer.parseInt(p[1]), p.length > 2 ? Integer.parseInt(p[2]) : 0 };
        } catch (NumberFormatException e) {
            return null;
        }
    }

    private static boolean atLeast(int[] v, int minor, int patch) {
        return v[0] > 1 || v[1] > minor || (v[1] == minor && v[2] >= patch);
    }

    /** why a loader cannot be had for this version, like Millida's dimmed loaders; null when it can */
    static String why(String loader, String version) {
        int[] v = parts(version);
        if ("vanilla".equals(loader)) {
            return null;
        }
        if (v == null) {
            return "fabric".equals(loader) ? null : "не для снапшотов";
        }
        if ("neoforge".equals(loader) && !atLeast(v, 20, 1)) {
            return "с 1.20.1";
        }
        if (("fabric".equals(loader) || "quilt".equals(loader)) && !atLeast(v, 14, 0)) {
            return "с 1.14";
        }
        if ("forge".equals(loader) && !atLeast(v, 1, 0)) {
            return "нет";
        }
        return null;
    }

    private void update() {
        float density = activity.getResources().getDisplayMetrics().density;
        if (why(loader, version) != null) {
            loader = why("fabric", version) == null ? "fabric" : "vanilla";
        }
        for (View tile : loaderTiles) {
            String id = (String) tile.getTag();
            String reason = why(id, version);
            boolean on = id.equals(loader);
            tile.setBackground(on ? PixelDrawable.accent(density) : PixelDrawable.plain(density));
            tile.setAlpha(reason == null ? 1f : 0.4f);
            TextView sub = (TextView) ((LinearLayout) tile).getChildAt(2);
            for (String[] l : LOADERS) {
                if (l[0].equals(id)) {
                    sub.setText(reason != null ? reason : l[2]);
                }
            }
        }
        boolean vanilla = "vanilla".equals(loader);
        fps.setText("⚡ Буст FPS: " + (fpsOn && !vanilla ? "вкл" : "выкл"));
        Ui.styleButton(activity, fps, fpsOn && !vanilla);
        fps.setEnabled(!vanilla);
        essentials.setText("🧰 Оптимизация и удобство: " + (essentialsOn && !vanilla ? "вкл" : "выкл"));
        Ui.styleButton(activity, essentials, essentialsOn && !vanilla);
        essentials.setEnabled(!vanilla);
        if (!nameTouched) {
            settingName = true;
            name.setText("");
            name.setHint(autoName());
            settingName = false;
        }
        fillVersions();
    }

    private void fillVersions() {
        float density = activity.getResources().getDisplayMetrics().density;
        String query = versionSearch.getText().toString().trim().toLowerCase(Locale.ROOT);
        versionGrid.removeAllViews();
        int shown = 0;
        for (final String[] v : versions) {
            boolean release = "release".equals(v[1]);
            if ((!release && !showSnapshots) || (!query.isEmpty() && !v[0].toLowerCase(Locale.ROOT).contains(query))) {
                continue;
            }
            Button b = new Button(activity);
            b.setText(v[0]);
            b.setAllCaps(false);
            b.setTypeface(Ui.pixel(activity));
            b.setTextSize(11);
            boolean on = v[0].equals(version);
            b.setTextColor(on ? 0xFFFFFFFF : release ? Ui.TEXT : Ui.HINT);
            b.setBackground(on ? PixelDrawable.accent(density) : PixelDrawable.plain(density));
            b.setPadding(dp(4), dp(2), dp(4), dp(5));
            b.setMinWidth(0);
            b.setMinimumWidth(0);
            b.setMinHeight(0);
            b.setMinimumHeight(0);
            b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View view) {
                    version = v[0];
                    update();
                }
            });
            versionGrid.addView(b, new ViewGroup.LayoutParams(dp(release ? 64 : 84), dp(32)));
            shown++;
        }
        if (shown == 0) {
            versionGrid.addView(Ui.label(activity, "Нет такой версии", 12, Ui.HINT, false));
        }
    }

    private void loadVersions() {
        activity.background(new MainActivity.Job<List<String[]>>() {
            public List<String[]> run() throws Exception {
                return Modrinth.allGameVersions();
            }

            public void done(List<String[]> list, Exception error) {
                if (list == null || list.isEmpty()) {
                    return;  // offline: the usual versions stay
                }
                versions.clear();
                versions.addAll(list);
                fillVersions();
            }
        });
    }

    private void loadPacks() {
        activity.background(new MainActivity.Job<JSONArray>() {
            public JSONArray run() throws Exception {
                return Modrinth.search("", "[[\"project_type:modpack\"]]", "downloads", 0, 8);
            }

            public void done(JSONArray hits, Exception error) {
                packs.removeAllViews();
                if (error != null || hits == null || hits.length() == 0) {
                    packs.addView(Ui.label(activity, "Нет связи с Modrinth — сборки появятся, когда будет интернет.", 12, Ui.HINT, false));
                    return;
                }
                for (int i = 0; i < hits.length(); i++) {
                    packs.addView(packRow(hits.optJSONObject(i)));
                }
            }
        });
    }

    /** a ready pack: the icon, the name, downloads and what it is, "Установить" */
    private View packRow(final JSONObject hit) {
        float density = activity.getResources().getDisplayMetrics().density;
        LinearLayout row = new LinearLayout(activity);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(6), dp(5), dp(6), dp(7));
        row.setBackground(PixelDrawable.plain(density));
        ImageView icon = new ImageView(activity);
        Icons.load(icon, hit.isNull("icon_url") ? "" : hit.optString("icon_url"));
        row.addView(icon, new LinearLayout.LayoutParams(dp(40), dp(40)));
        LinearLayout texts = new LinearLayout(activity);
        texts.setOrientation(LinearLayout.VERTICAL);
        texts.setPadding(dp(8), 0, dp(6), 0);
        TextView title = Ui.label(activity, hit.optString("title"), 13, Ui.TEXT, true);
        title.setSingleLine(true);
        title.setEllipsize(android.text.TextUtils.TruncateAt.END);
        texts.addView(title);
        TextView sub = Ui.label(activity, "⬇ " + Ui.downloads(hit.optLong("downloads")) + " · " + hit.optString("description"), 10, Ui.HINT, false);
        sub.setSingleLine(true);
        sub.setEllipsize(android.text.TextUtils.TruncateAt.END);
        texts.addView(sub);
        row.addView(texts, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button install = Ui.button(activity, "Установить", true);
        install.setTextSize(12);
        install.setMinHeight(dp(34));
        install.setMinimumHeight(dp(34));
        install.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.installModpack(hit.optString("project_id"), hit.optString("title"));
            }
        });
        row.addView(install);
        row.setClickable(true);
        row.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.openCatalog(hit.optString("title"));
            }
        });
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.bottomMargin = dp(5);
        row.setLayoutParams(p);
        return row;
    }

    /** the sign of a loader, drawn in pixels: a cloth for Fabric, anvils for Forge and NeoForge, patches for Quilt, grass */
    static final class LoaderSign extends View {
        private final String[] rows;
        private final int[] colors = new int[128];
        private final Paint paint = new Paint();

        LoaderSign(Context context, String loader) {
            super(context);
            if ("fabric".equals(loader)) {
                rows = new String[] { "..wwww..", ".wbwwbw.", "wwwbwwbw", "wbwwbwww", "wwbwwbww", ".wwbwwb.", "..wwww.." };
                colors['w'] = 0xFFDBD0B4;
                colors['b'] = 0xFF8F8466;
            } else if ("quilt".equals(loader)) {
                rows = new String[] { "ppp.bbb", "ppp.bbb", "ppp.bbb", ".......", "ooo.mmm", "ooo.mmm", "ooo.mmm" };
                colors['p'] = 0xFFB07CE8;
                colors['b'] = 0xFF4FA3E0;
                colors['o'] = 0xFFF0A040;
                colors['m'] = 0xFFE8608C;
            } else if ("vanilla".equals(loader)) {
                rows = new String[] { "gggggggg", "gGgggGgg", "dgdddgdd", "dddddddd", "ddDddddd", "dddddDdd", "dDdddddd" };
                colors['g'] = 0xFF6DBE45;
                colors['G'] = 0xFF8FD460;
                colors['d'] = 0xFF8B5A2B;
                colors['D'] = 0xFF6B4320;
            } else {
                rows = new String[] { "wwwwwwwww...", ".wwwwwwwwwww", "..lllllll...", "....www.....", "....www.....", "..wwwwwww...", ".wwwwwwwww.." };
                boolean neo = "neoforge".equals(loader);
                colors['w'] = neo ? 0xFFE8743C : 0xFF8A8F9C;
                colors['l'] = neo ? 0xFFB04E20 : 0xFF5C616C;
            }
        }

        @Override
        protected void onDraw(Canvas canvas) {
            int cols = 0;
            for (String row : rows) {
                cols = Math.max(cols, row.length());
            }
            float u = Math.min(getWidth() / (float) cols, getHeight() / (float) rows.length);
            float ox = (getWidth() - cols * u) / 2, oy = (getHeight() - rows.length * u) / 2;
            for (int j = 0; j < rows.length; j++) {
                for (int i = 0; i < rows[j].length(); i++) {
                    char c = rows[j].charAt(i);
                    if (c != '.' && colors[c] != 0) {
                        paint.setColor(colors[c]);
                        canvas.drawRect(ox + i * u, oy + j * u, ox + (i + 1) * u, oy + (j + 1) * u, paint);
                    }
                }
            }
        }
    }
}
