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
 * The main screen, made like Millida Launcher in Vellora's colours: the lobby (picture tiles on the left, the player in
 * the middle, Arkan the wizard on the right, "Сегодня играем" with a big Play button below) and the library (search,
 * "Мои сборки" and what to do with the selected one); the catalog opens from the tabs at the top.
 */
final class HomeScreen {
    private final MainActivity activity;
    private final LinearLayout root;
    private final GridView grid;
    private LinearLayout accountChip;
    private ImageView head;
    private TextView accountName;
    private final TextView empty;
    private final EditText search;
    private final LinearLayout actions;
    private final List<Installation> all = new ArrayList<Installation>();
    private final List<Installation> shown = new ArrayList<Installation>();
    private String selectedId;
    private LinearLayout playBar;  // "Сегодня играем": the chosen installation and a big Play button, like Millida's lobby
    private ImageView playIcon;
    private TextView playName;
    private TextView playInfo;
    private LinearLayout lobby;    // like Millida Launcher: the lobby with the player and Play
    private LinearLayout library;  // and the library of installations
    private Button lobbyTab;
    private Button libraryTab;
    private CharacterView playerBody;  // the player with what they wear, alive
    private LinearLayout recommendations;
    private TextView playerName;

    HomeScreen(MainActivity activity) {
        this.activity = activity;
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new RainDrawable(activity.getResources().getDisplayMetrics().density));

        // the top bar like Millida Launcher's: the name, the tabs, square icon buttons and the player
        LinearLayout bar = new LinearLayout(activity);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(10), dp(6), dp(8), dp(6));
        bar.setBackgroundColor(0xE60B0305);
        ImageView logo = new ImageView(activity);
        logo.setImageResource(R.drawable.install_default);
        bar.addView(logo, new LinearLayout.LayoutParams(dp(34), dp(34)));
        TextView title = Ui.label(activity, "VELLORA", 17, Ui.ACCENT, true);
        title.setPadding(dp(8), 0, dp(14), 0);
        bar.addView(title);
        lobbyTab = tab("Лобби", true);
        libraryTab = tab("Библиотека", false);
        Button catalogTab = tab("Каталог", false);
        lobbyTab.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showPage(true);
            }
        });
        libraryTab.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showPage(false);
            }
        });
        catalogTab.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.openBrowser(null, BrowserScreen.MODPACK);
            }
        });
        bar.addView(lobbyTab);
        bar.addView(libraryTab);
        bar.addView(catalogTab);
        bar.addView(new View(activity), new LinearLayout.LayoutParams(0, 1, 1));
        bar.addView(square("🎮", "Управление", new Runnable() {
            public void run() {
                HomeScreen.this.activity.openControls();
            }
        }));
        bar.addView(square("💬", "Чаты с Арканом", new Runnable() {
            public void run() {
                HomeScreen.this.activity.showChatList();
            }
        }));
        bar.addView(square("⚙", "Меню", new Runnable() {
            public void run() {
                HomeScreen.this.activity.showAppMenu();
            }
        }));
        // the signed-in player: head and name
        accountChip = new LinearLayout(activity);
        accountChip.setGravity(Gravity.CENTER_VERTICAL);
        accountChip.setPadding(dp(8), dp(4), dp(10), dp(6));
        accountChip.setBackground(PixelDrawable.plain(activity.getResources().getDisplayMetrics().density));
        head = new ImageView(activity);
        accountChip.addView(head, new LinearLayout.LayoutParams(dp(28), dp(28)));
        LinearLayout who = new LinearLayout(activity);
        who.setOrientation(LinearLayout.VERTICAL);
        who.setPadding(dp(8), 0, 0, 0);
        accountName = Ui.label(activity, "", 13, Ui.TEXT, true);
        who.addView(accountName);
        accountChip.addView(who);
        accountChip.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.showAccount();
            }
        });
        LinearLayout.LayoutParams chipParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, dp(44));
        chipParams.leftMargin = dp(6);
        bar.addView(accountChip, chipParams);
        root.addView(bar);

        android.widget.FrameLayout pages = new android.widget.FrameLayout(activity);
        root.addView(pages, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        /* ---------------- the lobby: tiles on the left, the player in the middle, Arkan on the right, Play below */
        lobby = new LinearLayout(activity);
        lobby.setOrientation(LinearLayout.VERTICAL);
        LinearLayout stage = new LinearLayout(activity);
        lobby.addView(stage, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        LinearLayout tiles = new LinearLayout(activity);
        tiles.setOrientation(LinearLayout.VERTICAL);
        tiles.setPadding(dp(10), dp(10), dp(4), dp(6));
        // like Millida's lobby: the wardrobe, ready packs and playing with friends
        WardrobeScreen.ItemPicture outfit = new WardrobeScreen.ItemPicture(activity, "crown", "chain", "demon_wings", "cube_pet");
        tiles.addView(tile(outfit, "Гардероб", "Образы, плащи, питомцы", new Runnable() {
            public void run() {
                HomeScreen.this.activity.openWardrobe();
            }
        }), tileParams());
        tiles.addView(tile(R.drawable.tile_packs, "Готовые сборки", "Modrinth и CurseForge", new Runnable() {
            public void run() {
                HomeScreen.this.activity.openBrowser(null, BrowserScreen.MODPACK);
            }
        }), tileParams());
        tiles.addView(tile(R.drawable.install_default, "С друзьями", "Сервер друга по коду", new Runnable() {
            public void run() {
                HomeScreen.this.activity.showInviteCode();
            }
        }), tileParams());
        stage.addView(tiles, new LinearLayout.LayoutParams(dp(170), ViewGroup.LayoutParams.MATCH_PARENT));

        // the player, big, like the character in Millida's lobby
        LinearLayout middle = new LinearLayout(activity);
        middle.setOrientation(LinearLayout.VERTICAL);
        middle.setGravity(Gravity.CENTER);
        playerBody = new CharacterView(activity);
        middle.addView(playerBody, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        playerName = Ui.label(activity, "", 16, Ui.TEXT, true);
        playerName.setGravity(Gravity.CENTER);
        middle.addView(playerName);
        stage.addView(middle, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));

        // Arkan's card: the wizard and "Собрать с ИИ"
        LinearLayout arkan = new LinearLayout(activity);
        arkan.setOrientation(LinearLayout.VERTICAL);
        arkan.setGravity(Gravity.CENTER_HORIZONTAL);
        arkan.setPadding(dp(10), dp(10), dp(10), dp(10));
        arkan.setBackground(Ui.rounded(activity, 0xE6140509, 0, 0xFF8A3442));
        TextView arkanTop = Ui.label(activity, "ИИ-МАГ", 11, 0xFFFF8090, true);
        arkan.addView(arkanTop);
        WizardView wizard = new WizardView(activity);
        arkan.addView(wizard, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        TextView arkanName = Ui.label(activity, "Аркан", 18, Ui.TEXT, true);
        arkan.addView(arkanName);
        TextView arkanText = Ui.label(activity, "Опиши сборку словами — соберу её из модов", 12, Ui.HINT, false);
        arkanText.setGravity(Gravity.CENTER);
        arkan.addView(arkanText);
        Button ask = Ui.button(activity, "Собрать с ИИ", true);
        ask.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.openChat(null);
            }
        });
        LinearLayout.LayoutParams askParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        askParams.topMargin = dp(6);
        arkan.addView(ask, askParams);
        LinearLayout.LayoutParams arkanParams = new LinearLayout.LayoutParams(dp(200), ViewGroup.LayoutParams.MATCH_PARENT);
        arkanParams.setMargins(dp(4), dp(10), dp(10), dp(6));
        stage.addView(arkan, arkanParams);

        // bottom: what we play today and the big Play button
        playBar = new LinearLayout(activity);
        playBar.setGravity(Gravity.CENTER_VERTICAL);
        playBar.setPadding(dp(12), dp(8), dp(12), dp(10));
        playBar.setBackground(Ui.rounded(activity, 0xE61C080D, 0, 0xFF8A3442));
        playIcon = new ImageView(activity);
        playBar.addView(playIcon, new LinearLayout.LayoutParams(dp(46), dp(46)));
        LinearLayout texts = new LinearLayout(activity);
        texts.setOrientation(LinearLayout.VERTICAL);
        texts.setPadding(dp(12), 0, dp(12), 0);
        texts.addView(Ui.label(activity, "СЕГОДНЯ ИГРАЕМ", 11, 0xFFFF8090, true));
        playName = Ui.label(activity, "", 18, Ui.TEXT, true);
        playName.setSingleLine(true);
        playName.setEllipsize(android.text.TextUtils.TruncateAt.END);
        texts.addView(playName);
        playInfo = Ui.label(activity, "", 12, Ui.HINT, false);
        texts.addView(playInfo);
        playBar.addView(texts, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button choose = Ui.button(activity, "Сменить", false);
        choose.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showPage(false);
            }
        });
        playBar.addView(choose, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, dp(48)));
        Button play = Ui.button(activity, "▶  Играть", true);
        play.setTextSize(22);
        play.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Installation inst = selected();
                if (inst != null) {
                    HomeScreen.this.activity.playInstallation(inst);
                } else {
                    newInstallation();
                }
            }
        });
        LinearLayout.LayoutParams playParams = new LinearLayout.LayoutParams(dp(220), dp(56));
        playParams.leftMargin = dp(8);
        playBar.addView(play, playParams);
        LinearLayout.LayoutParams barParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        barParams.setMargins(dp(10), 0, dp(10), dp(8));
        lobby.addView(playBar, barParams);
        pages.addView(lobby);

        /* ---------------- the library: search, the installations, what to do with the selected one */
        library = new LinearLayout(activity);
        LinearLayout listColumn = new LinearLayout(activity);
        listColumn.setOrientation(LinearLayout.VERTICAL);
        LinearLayout searchRow = new LinearLayout(activity);
        searchRow.setGravity(Gravity.CENTER_VERTICAL);
        searchRow.setPadding(dp(10), dp(8), dp(4), 0);
        search = new EditText(activity);
        search.setHint("Сборка, мод или просьба к ИИ...");
        search.setHintTextColor(Ui.HINT);
        search.setTextColor(Ui.TEXT);
        search.setTextSize(14);
        search.setSingleLine(true);
        search.setBackground(Ui.rounded(activity, 0xE6140509, 0, 0xFF5A1E2A));
        search.setPadding(dp(12), dp(8), dp(12), dp(8));
        search.addTextChangedListener(new TextWatcher() {
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            public void afterTextChanged(Editable s) {
                filter();
            }
        });
        searchRow.addView(search, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button withAi = Ui.button(activity, "Собрать с ИИ", false);
        withAi.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.askWizard(search.getText().toString());
            }
        });
        LinearLayout.LayoutParams aiParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        aiParams.leftMargin = dp(6);
        searchRow.addView(withAi, aiParams);
        Button add = Ui.button(activity, "+ Новая", true);
        add.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                newInstallation();
            }
        });
        searchRow.addView(add, aiParams);
        listColumn.addView(searchRow);
        TextView mine = Ui.label(activity, "Мои сборки", 20, Ui.TEXT, true);
        mine.setPadding(dp(12), dp(10), 0, 0);
        listColumn.addView(mine);
        grid = new GridView(activity);
        grid.setNumColumns(GridView.AUTO_FIT);
        grid.setColumnWidth(dp(150));
        grid.setStretchMode(GridView.STRETCH_COLUMN_WIDTH);
        grid.setHorizontalSpacing(dp(8));
        grid.setVerticalSpacing(dp(8));
        grid.setPadding(dp(8), dp(8), dp(8), dp(10));
        grid.setClipToPadding(false);
        grid.setSelector(new android.graphics.drawable.ColorDrawable(0));
        grid.setAdapter(adapter);
        grid.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                select(shown.get(position).id);
            }
        });
        empty = Ui.label(activity, "Пока пусто. Нажми «+ Новая», загляни в «Каталог» или попроси Аркана собрать сборку.", 16, Ui.TEXT, true);
        empty.setGravity(Gravity.CENTER);
        empty.setPadding(dp(18), dp(14), dp(18), dp(16));
        empty.setBackground(Ui.rounded(activity, Ui.GLASS, 0, 0xFFFF5A6E));
        LinearLayout.LayoutParams emptyParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        emptyParams.setMargins(dp(24), dp(30), dp(24), 0);
        listColumn.addView(empty, emptyParams);
        listColumn.addView(grid, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        // "Рекомендуем", like Millida Launcher's library: the most popular modpacks on Modrinth
        TextView recommended = Ui.label(activity, "Рекомендуем", 16, Ui.TEXT, true);
        recommended.setPadding(dp(12), dp(2), 0, dp(4));
        listColumn.addView(recommended);
        android.widget.HorizontalScrollView recScroll = new android.widget.HorizontalScrollView(activity);
        recScroll.setHorizontalScrollBarEnabled(false);
        recommendations = new LinearLayout(activity);
        recommendations.setPadding(dp(8), 0, dp(8), dp(8));
        recScroll.addView(recommendations);
        listColumn.addView(recScroll);
        loadRecommendations(recommended);
        library.addView(listColumn, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        ScrollView side = new ScrollView(activity);
        actions = new LinearLayout(activity);
        actions.setOrientation(LinearLayout.VERTICAL);
        actions.setPadding(dp(10), dp(10), dp(10), dp(10));
        side.addView(actions);
        side.setBackgroundColor(0xCC0B0305);
        library.addView(side, new LinearLayout.LayoutParams(dp(210), ViewGroup.LayoutParams.MATCH_PARENT));
        pages.addView(library);
        showPage(true);
    }

    private void loadRecommendations(final TextView title) {
        activity.background(new MainActivity.Job<org.json.JSONArray>() {
            public org.json.JSONArray run() throws Exception {
                return Modrinth.search("", "[[\"project_type:modpack\"]]", "downloads", 0, 10);
            }

            public void done(org.json.JSONArray hits, Exception error) {
                if (error != null || hits == null || hits.length() == 0) {
                    title.setVisibility(View.GONE);  // offline: no row
                    return;
                }
                recommendations.removeAllViews();
                for (int i = 0; i < hits.length(); i++) {
                    final org.json.JSONObject hit = hits.optJSONObject(i);
                    LinearLayout card = new LinearLayout(activity);
                    card.setGravity(Gravity.CENTER_VERTICAL);
                    card.setPadding(dp(6), dp(5), dp(10), dp(8));
                    card.setBackground(PixelDrawable.plain(activity.getResources().getDisplayMetrics().density));
                    ImageView icon = new ImageView(activity);
                    Icons.load(icon, hit.isNull("icon_url") ? "" : hit.optString("icon_url"));
                    card.addView(icon, new LinearLayout.LayoutParams(dp(36), dp(36)));
                    LinearLayout texts = new LinearLayout(activity);
                    texts.setOrientation(LinearLayout.VERTICAL);
                    texts.setPadding(dp(6), 0, 0, 0);
                    TextView name = Ui.label(activity, hit.optString("title"), 12, Ui.TEXT, true);
                    name.setSingleLine(true);
                    name.setEllipsize(android.text.TextUtils.TruncateAt.END);
                    name.setMaxWidth(dp(120));
                    texts.addView(name);
                    texts.addView(Ui.label(activity, "⬇ " + Ui.downloads(hit.optLong("downloads")), 10, Ui.HINT, false));
                    card.addView(texts);
                    card.setClickable(true);
                    card.setOnClickListener(new View.OnClickListener() {
                        public void onClick(View v) {
                            activity.openCatalog(hit.optString("title"));
                        }
                    });
                    LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                    p.rightMargin = dp(8);
                    recommendations.addView(card, p);
                }
            }
        });
    }

    /** the lobby or the library */
    void showPage(boolean lobbyPage) {
        lobby.setVisibility(lobbyPage ? View.VISIBLE : View.GONE);
        library.setVisibility(lobbyPage ? View.GONE : View.VISIBLE);
        Ui.styleButton(activity, lobbyTab, lobbyPage);
        Ui.styleButton(activity, libraryTab, !lobbyPage);
        if (!lobbyPage) {
            // the grid does not lay itself out while hidden: refresh it when it comes back
            adapter.notifyDataSetChanged();
            grid.invalidateViews();
            grid.requestLayout();
        }
    }

    private Button tab(String text, boolean chosen) {
        Button b = Ui.button(activity, text, chosen);
        b.setTextSize(14);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.rightMargin = dp(4);
        b.setLayoutParams(p);
        return b;
    }

    /** a square icon button of the top bar */
    private Button square(String icon, final String what, final Runnable action) {
        Button b = Ui.button(activity, icon, false);
        b.setTextSize(18);
        b.setPadding(0, 0, 0, dp(3));
        b.setContentDescription(what);
        b.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                action.run();
            }
        });
        b.setOnLongClickListener(new View.OnLongClickListener() {
            public boolean onLongClick(View v) {
                android.widget.Toast.makeText(activity, what, android.widget.Toast.LENGTH_SHORT).show();
                return true;
            }
        });
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(dp(46), dp(46));
        p.leftMargin = dp(6);
        b.setLayoutParams(p);
        return b;
    }

    private void updatePlayBar() {
        Installation inst = selected();
        if (playBar == null) {
            return;
        }
        if (inst == null) {
            playBar.setVisibility(View.GONE);
            return;
        }
        playBar.setVisibility(View.VISIBLE);
        playName.setText(inst.name);
        playInfo.setText(inst.minecraftVersion + " · " + ("vanilla".equals(inst.loader) || inst.loader.isEmpty() ? "без модов" : Plan.loaderName(inst.loader)));
        Icons.load(playIcon, inst.iconUrl, inst.chatId.isEmpty() ? R.drawable.install_default : R.drawable.wizard);
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
        ImageView image = new ImageView(activity);
        image.setImageResource(picture);
        image.setScaleType(ImageView.ScaleType.CENTER_CROP);
        return tile(image, title, subtitle, action);
    }

    /** a big picture tile with its name on a dark strip over the bottom of the picture, like the tiles in Millida's lobby */
    private View tile(View image, String title, String subtitle, final Runnable action) {
        android.widget.FrameLayout tile = new android.widget.FrameLayout(activity);
        tile.setPadding(dp(4), dp(4), dp(4), dp(7));
        tile.setBackground(PixelDrawable.plain(activity.getResources().getDisplayMetrics().density));
        image.setBackgroundColor(0xFF1E070D);
        tile.addView(image, new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        LinearLayout strip = new LinearLayout(activity);
        strip.setOrientation(LinearLayout.VERTICAL);
        strip.setBackgroundColor(0xD9140509);
        strip.setPadding(dp(8), dp(3), dp(6), dp(4));
        TextView t = Ui.label(activity, title, 14, Ui.TEXT, true);
        strip.addView(t);
        TextView s = Ui.label(activity, subtitle, 10, Ui.HINT, false);
        s.setSingleLine(true);
        strip.addView(s);
        tile.addView(strip, new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                                         ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM));
        tile.setClickable(true);
        tile.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                action.run();
            }
        });
        return tile;
    }

    /** shows who is signed in */
    void updateAccount() {
        Account account = activity.account();
        if (account == null || !account.signedIn()) {
            accountChip.setVisibility(View.GONE);
            return;
        }
        accountChip.setVisibility(View.VISIBLE);
        accountName.setText(account.ownsJava ? account.name : "Без Java Edition");
        Icons.loadHead(head, account.skinUrl, R.drawable.wizard);
        WardrobeScreen.loadSkinAndCape(activity, playerBody);
        playerName.setText(account.name);
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
        showPage(false);  // a chosen installation is shown with its actions in the library
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
            Icons.load(icon, inst.iconUrl, inst.chatId.isEmpty() ? R.drawable.install_default : R.drawable.wizard);
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
        updatePlayBar();
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
        String played = Installation.playTime(inst.playedMillis);
        if (!played.isEmpty()) {
            sub.append("\nнаиграно: " + played);
        }
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
            actions.addView(action("Чат с Арканом", new Runnable() {
                public void run() {
                    activity.openChat(inst.chatId);
                }
            }), actionParams());
        }
        if (GameEngine.get() != null) {
            // like Millida Launcher: "Буст FPS", the screenshots and the worlds of the installation
            final Button boost = action(inst.fpsBoost ? "Буст FPS: вкл" : "Буст FPS: выкл", null);
            Ui.styleButton(activity, boost, inst.fpsBoost);
            boost.setOnClickListener(click(new Runnable() {
                public void run() {
                    inst.fpsBoost = !inst.fpsBoost;
                    activity.saveInstallation(inst);
                    android.widget.Toast.makeText(activity, inst.fpsBoost
                        ? "Буст FPS включён: при запуске графика станет легче (дальность 6, быстрая графика, без облаков и теней)"
                        : "Буст FPS выключен: при запуске вернутся твои настройки графики", android.widget.Toast.LENGTH_LONG).show();
                    updateActions();
                }
            }));
            actions.addView(boost, actionParams());
            actions.addView(action("Скриншоты", new Runnable() {
                public void run() {
                    new GameFiles(activity, inst).showScreenshots();
                }
            }), actionParams());
            actions.addView(action("Миры", new Runnable() {
                public void run() {
                    new GameFiles(activity, inst).showWorlds();
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
            .setMessage("Удалить установку «" + inst.name + "»? Чат с Арканом, если он есть, останется.")
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
                select(inst.id);  // the new installation opens in the library, with its actions
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
