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
    static final int LOBBY = 0, LIBRARY = 1, CREATE = 2;

    /** "201 вещь", "56 вещей", "3 вещи" */
    static String things(int n) {
        int last = n % 10, two = n % 100;
        String word = last == 1 && two != 11 ? "вещь" : last >= 2 && last <= 4 && (two < 12 || two > 14) ? "вещи" : "вещей";
        return n + " " + word;
    }

    private final MainActivity activity;
    private final LinearLayout root;
    private LinearLayout accountChip;
    private ImageView head;
    private TextView accountName;
    private TextView accountSub;
    private final EditText search;
    private final LinearLayout actions;
    private final List<Installation> all = new ArrayList<Installation>();
    private final List<Installation> shown = new ArrayList<Installation>();
    private String selectedId;
    private TextView playChange;  // "Сменить сборку ›" under the name
    private LinearLayout playBar;  // "Сегодня играем": the chosen installation and a big Play button, like Millida's lobby
    private ImageView playIcon;
    private TextView playName;
    private TextView playInfo;
    private Button play;
    private android.widget.FrameLayout lobby;  // like Millida Launcher: the lobby with the player and Play
    private android.widget.FrameLayout library;  // the library of installations
    private View createView;       // the "Создать" tab: a new installation and the ready packs
    private CreatePage createPage;
    private int page = -1;
    private final List<Button> pageTabs = new ArrayList<Button>();
    private LinearLayout lobbyBar;    // the lobby's buttons in the top right corner
    private LinearLayout libraryBar;  // the library's top bar: "‹ Лобби", the tabs, import and a new pack
    private CharacterView playerBody;
    private CharacterView skinTile;  // "Мой скин" shows the player too
    private HubTile wardrobeTile;
    private LinearLayout recommendations;
    private LinearLayout mine;       // "Мои сборки": the cards in a row, like Millida's library
    private View buildPanel;         // the page of the chosen installation, slides in from the right
    private android.widget.FrameLayout pagesBox;  // the pages; the one leaving fades out in its overlay
    private Button soundButton;
    private Button musicButton;

    HomeScreen(MainActivity activity) {
        this.activity = activity;
        float density = activity.getResources().getDisplayMetrics().density;
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new RainDrawable(density));

        /* ---------------- the library's top bar, like Millida's: "‹ Лобби", the tabs in the middle, import and a new pack */
        libraryBar = new LinearLayout(activity);
        libraryBar.setGravity(Gravity.CENTER_VERTICAL);
        libraryBar.setPadding(dp(10), dp(6), dp(10), dp(6));
        libraryBar.setBackgroundColor(0xF0080204);
        Button back = Ui.button(activity, "‹ Лобби", true);
        back.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showPage(LOBBY);
            }
        });
        libraryBar.addView(back);
        libraryBar.addView(new View(activity), new LinearLayout.LayoutParams(0, 1, 1));
        Button libraryTab = tab("📦 Библиотека", LIBRARY);
        Button catalogTab = tab("📖 Каталог", -1);
        catalogTab.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.openBrowser(null, BrowserScreen.MODPACK);
            }
        });
        Button createTab = tab("✚ Создать", CREATE);
        libraryBar.addView(libraryTab);
        libraryBar.addView(catalogTab);
        libraryBar.addView(createTab);
        libraryBar.addView(new View(activity), new LinearLayout.LayoutParams(0, 1, 1));
        Button importPack = Ui.button(activity, "⬇ Импорт", false);
        importPack.setTextSize(13);
        importPack.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.pickPackFile();
            }
        });
        libraryBar.addView(importPack);
        Button newPack = Ui.button(activity, "+ Новая сборка", true);
        newPack.setTextSize(13);
        newPack.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                newInstallation();
            }
        });
        LinearLayout.LayoutParams newParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        newParams.leftMargin = dp(6);
        libraryBar.addView(newPack, newParams);
        root.addView(libraryBar);

        android.widget.FrameLayout pages = new android.widget.FrameLayout(activity);
        pagesBox = pages;
        root.addView(pages, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        /* ---------------- the lobby, laid out like Millida's: tiles on the left, the player in the middle, the buttons in
           the top right corner, a banner on the right and "Сегодня играем" with a big Play button at the bottom */
        lobby = new android.widget.FrameLayout(activity);
        lobby.setClipChildren(false);

        // the player, big, in the middle
        playerBody = new CharacterView(activity);
        android.widget.FrameLayout.LayoutParams bodyParams = new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                                                                         ViewGroup.LayoutParams.MATCH_PARENT);
        bodyParams.setMargins(dp(180), dp(70), dp(250), dp(92));
        lobby.addView(playerBody, bodyParams);


        // the tiles on the left, like Millida's: the wardrobe, my skin, playing with friends
        LinearLayout tiles = new LinearLayout(activity);
        tiles.setOrientation(LinearLayout.VERTICAL);
        tiles.setClipChildren(false);
        WardrobeScreen.ItemPicture outfit = new WardrobeScreen.ItemPicture(activity, "crown", "chain", "demon_wings", "cube_pet");
        wardrobeTile = new HubTile(activity, 0xFFB3263A, outfit, "Гардероб", things(Cosmetics.items(activity).size()) + " — всё открыто");
        wardrobeTile.setAlert(!activity.getSharedPreferences("lobby", android.content.Context.MODE_PRIVATE).getBoolean("wardrobeSeen", false));
        wardrobeTile.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.getSharedPreferences("lobby", android.content.Context.MODE_PRIVATE).edit().putBoolean("wardrobeSeen", true).apply();
                wardrobeTile.setAlert(false);
                HomeScreen.this.activity.openWardrobe(WardrobeScreen.SETS);
            }
        });
        tiles.addView(wardrobeTile, tileParams());
        skinTile = new CharacterView(activity);
        skinTile.setTurnable(false);
        skinTile.setCloseUp(true);
        HubTile skin = new HubTile(activity, 0xFF7A1E4E, skinTile, "Мой скин", "Загрузи свой");
        skin.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.openWardrobe(WardrobeScreen.SKINS);
            }
        });
        tiles.addView(skin, tileParams());
        LinearLayout duo = new LinearLayout(activity);
        WardrobeScreen.ItemPicture friendA = new WardrobeScreen.ItemPicture(activity, "cat_ears", "scarf", "", "");
        WardrobeScreen.ItemPicture friendB = new WardrobeScreen.ItemPicture(activity, "headphones", "", "", "");
        friendB.emote = "wave";
        duo.addView(friendA, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        duo.addView(friendB, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        HubTile friends = new HubTile(activity, 0xFF5A1A1A, duo, "Играть с друзьями", "Сервер друга по коду");
        friends.setRibbon("Бесплатно");
        friends.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.showInviteCode();
            }
        });
        tiles.addView(friends, tileParams());
        android.widget.FrameLayout.LayoutParams tilesParams = new android.widget.FrameLayout.LayoutParams(dp(164), ViewGroup.LayoutParams.MATCH_PARENT);
        tilesParams.setMargins(dp(12), dp(8), 0, dp(10));
        lobby.addView(tiles, tilesParams);

        // the buttons in the top right corner, like Millida's: friends, the account, the library, chats, controls, settings
        lobbyBar = new LinearLayout(activity);
        lobbyBar.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout friendsChip = new LinearLayout(activity);
        friendsChip.setGravity(Gravity.CENTER_VERTICAL);
        friendsChip.setPadding(dp(8), dp(3), dp(10), dp(6));
        friendsChip.setBackground(PixelDrawable.plain(density));
        View people = new View(activity);
        people.setBackground(new PixelGlyph(null, PixelGlyph.PEOPLE, density));
        friendsChip.addView(people, new LinearLayout.LayoutParams(dp(28), dp(26)));
        LinearLayout friendsTexts = new LinearLayout(activity);
        friendsTexts.setOrientation(LinearLayout.VERTICAL);
        friendsTexts.setPadding(dp(6), 0, 0, 0);
        friendsTexts.addView(tight(Ui.label(activity, "Друзья", 12, Ui.TEXT, true)));
        friendsTexts.addView(tight(Ui.label(activity, "по коду", 9, Ui.HINT, false)));
        friendsChip.addView(friendsTexts);
        friendsChip.setClickable(true);
        friendsChip.setContentDescription("Играть с друзьями");
        friendsChip.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.showInviteCode();
            }
        });
        friendsChip.setMinimumHeight(dp(46));
        lobbyBar.addView(friendsChip, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        accountChip = new LinearLayout(activity);
        accountChip.setGravity(Gravity.CENTER_VERTICAL);
        accountChip.setPadding(dp(6), dp(3), dp(10), dp(6));
        accountChip.setBackground(PixelDrawable.plain(density));
        head = new ImageView(activity);
        accountChip.addView(head, new LinearLayout.LayoutParams(dp(30), dp(30)));
        LinearLayout who = new LinearLayout(activity);
        who.setOrientation(LinearLayout.VERTICAL);
        who.setPadding(dp(8), 0, dp(6), 0);
        accountName = tight(Ui.label(activity, "", 13, Ui.TEXT, true));
        who.addView(accountName);
        accountSub = tight(Ui.label(activity, "Microsoft", 9, Ui.HINT, false));
        who.addView(accountSub);
        accountChip.addView(who);
        accountChip.addView(Ui.label(activity, "˅", 12, Ui.HINT, true));
        accountChip.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.showAccount();
            }
        });
        accountChip.setMinimumHeight(dp(46));
        LinearLayout.LayoutParams chipParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        chipParams.leftMargin = dp(6);
        lobbyBar.addView(accountChip, chipParams);
        soundButton = square(Sounds.enabled(activity) ? PixelGlyph.SPEAKER : PixelGlyph.SPEAKER_OFF, "Звуки интерфейса", new Runnable() {
            public void run() {
                boolean on = !Sounds.enabled(HomeScreen.this.activity);
                Sounds.setEnabled(HomeScreen.this.activity, on);
                ((PixelGlyph) soundButton.getBackground()).setRows(on ? PixelGlyph.SPEAKER : PixelGlyph.SPEAKER_OFF);
                Sounds.play(Sounds.TOGGLE);
                android.widget.Toast.makeText(HomeScreen.this.activity, on ? "Звуки включены" : "Звуки выключены", android.widget.Toast.LENGTH_SHORT).show();
            }
        });
        lobbyBar.addView(soundButton);
        musicButton = square(Sounds.musicEnabled(activity) ? PixelGlyph.NOTE : PixelGlyph.NOTE_OFF, "Музыка", new Runnable() {
            public void run() {
                boolean on = !Sounds.musicEnabled(HomeScreen.this.activity);
                Sounds.setMusicEnabled(HomeScreen.this.activity, on);
                ((PixelGlyph) musicButton.getBackground()).setRows(on ? PixelGlyph.NOTE : PixelGlyph.NOTE_OFF);
                Sounds.play(Sounds.TOGGLE);
                android.widget.Toast.makeText(HomeScreen.this.activity, on ? "Музыка включена" : "Музыка выключена", android.widget.Toast.LENGTH_SHORT).show();
            }
        });
        lobbyBar.addView(musicButton);
        lobbyBar.addView(square(PixelGlyph.CHEST, "Мои сборки", new Runnable() {
            public void run() {
                showPage(LIBRARY);
            }
        }));
        lobbyBar.addView(square(PixelGlyph.CHAT, "Чаты с Арканом", new Runnable() {
            public void run() {
                HomeScreen.this.activity.showChatList();
            }
        }));
        lobbyBar.addView(square(PixelGlyph.PAD, "Управление", new Runnable() {
            public void run() {
                HomeScreen.this.activity.openControls();
            }
        }));
        lobbyBar.addView(square(PixelGlyph.GEAR, "Настройки", new Runnable() {
            public void run() {
                HomeScreen.this.activity.showAppMenu();
            }
        }));
        android.widget.FrameLayout.LayoutParams barParams = new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                                                                                                        ViewGroup.LayoutParams.WRAP_CONTENT,
                                                                                                        Gravity.TOP | Gravity.END);
        barParams.setMargins(0, dp(8), dp(10), 0);
        lobby.addView(lobbyBar, barParams);

        // on the right: the banner (Arkan the wizard assembles a pack from words, like Millida's promo card) and under it
        // the big button that makes an installation by hand: the version, the loader and the mods
        LinearLayout rightColumn = new LinearLayout(activity);
        rightColumn.setOrientation(LinearLayout.VERTICAL);
        rightColumn.addView(banner(), new LinearLayout.LayoutParams(dp(236), dp(142)));
        Button bigCreate = Ui.button(activity, "", true);
        android.text.SpannableString createText = new android.text.SpannableString("✚  Создать установку\nверсия · загрузчик · моды");
        int cut = createText.toString().indexOf('\n');
        createText.setSpan(new android.text.style.RelativeSizeSpan(0.62f), cut, createText.length(), android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        createText.setSpan(new android.text.style.ForegroundColorSpan(0xE6FFFFFF), cut, createText.length(), android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        bigCreate.setText(createText);
        bigCreate.setTextSize(16);
        bigCreate.setContentDescription("Создать установку");
        bigCreate.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showPage(CREATE);
            }
        });
        LinearLayout.LayoutParams bigParams = new LinearLayout.LayoutParams(dp(236), dp(62));
        bigParams.topMargin = dp(8);
        rightColumn.addView(bigCreate, bigParams);
        android.widget.FrameLayout.LayoutParams columnParams = new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                                                                                                           ViewGroup.LayoutParams.WRAP_CONTENT,
                                                                                                           Gravity.END | Gravity.CENTER_VERTICAL);
        columnParams.setMargins(0, dp(14), dp(10), 0);
        lobby.addView(rightColumn, columnParams);

        // the bottom: what we play today and the big Play button
        playBar = new LinearLayout(activity);
        playBar.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout today = new LinearLayout(activity);
        today.setGravity(Gravity.CENTER_VERTICAL);
        today.setPadding(dp(8), dp(6), dp(12), dp(8));
        today.setBackground(PixelDrawable.panel(0xF02A0A12, 0xFF8A3442, density));
        android.widget.FrameLayout cover = new android.widget.FrameLayout(activity);
        cover.setBackground(PixelDrawable.panel(0xFF3A0D16, 0xFF0B0305, density));
        playIcon = new ImageView(activity);
        playIcon.setPadding(dp(6), dp(6), dp(6), dp(6));
        cover.addView(playIcon);
        TextView coverTag = Ui.label(activity, "Сборка", 8, Ui.TEXT, true);
        coverTag.setPadding(dp(3), 0, dp(3), dp(1));
        coverTag.setBackgroundColor(0xCC0B0305);
        cover.addView(coverTag, new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT,
                                                                            Gravity.TOP | Gravity.START));
        today.addView(cover, new LinearLayout.LayoutParams(dp(58), dp(58)));
        LinearLayout texts = new LinearLayout(activity);
        texts.setOrientation(LinearLayout.VERTICAL);
        texts.setPadding(dp(12), 0, dp(4), 0);
        texts.addView(Ui.label(activity, "СЕГОДНЯ ИГРАЕМ", 10, 0xFFFF8090, true));
        playName = Ui.label(activity, "", 18, Ui.TEXT, true);
        playName.setSingleLine(true);
        playName.setEllipsize(android.text.TextUtils.TruncateAt.END);
        texts.addView(playName);
        playInfo = Ui.label(activity, "", 11, Ui.HINT, false);
        playInfo.setSingleLine(true);
        texts.addView(playInfo);

        today.addView(texts, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        // like Millida's card: it says it opens the packs
        playChange = Ui.label(activity, "Сменить\nсборку ›", 10, 0xFFFF8090, true);
        playChange.setGravity(Gravity.END | Gravity.CENTER_VERTICAL);
        today.addView(playChange);
        today.setClickable(true);
        today.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showPage(LIBRARY);  // like Millida: the card opens the packs
            }
        });
        playBar.addView(today, new LinearLayout.LayoutParams(0, dp(76), 1));
        play = Ui.button(activity, "▶  Играть", true);
        play.setTextSize(24);
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
        LinearLayout.LayoutParams playParams = new LinearLayout.LayoutParams(dp(200), dp(76));
        playParams.leftMargin = dp(4);
        playBar.addView(play, playParams);
        android.widget.FrameLayout.LayoutParams playBarParams = new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                                                                            ViewGroup.LayoutParams.WRAP_CONTENT,
                                                                                                            Gravity.BOTTOM);
        playBarParams.setMargins(dp(190), 0, dp(190), dp(8));
        lobby.addView(playBar, playBarParams);
        pages.addView(lobby);

        /* ---------------- the library, like Millida's: the search, "Мои сборки" as cards, "Рекомендуем" */
        library = new android.widget.FrameLayout(activity);
        ScrollView libraryScroll = new ScrollView(activity);
        LinearLayout column = new LinearLayout(activity);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setPadding(dp(12), dp(8), dp(12), dp(12));
        libraryScroll.addView(column);
        library.addView(libraryScroll);
        LinearLayout searchRow = new LinearLayout(activity);
        searchRow.setGravity(Gravity.CENTER_VERTICAL);
        search = new EditText(activity);
        search.setHint("🔍  Мод, сборка или просьба к ИИ");
        search.setHintTextColor(Ui.HINT);
        search.setTextColor(Ui.TEXT);
        search.setTextSize(14);
        search.setSingleLine(true);
        search.setBackground(Ui.rounded(activity, 0xE6140509, 0, 0xFF5A1E2A));
        search.setPadding(dp(12), dp(9), dp(12), dp(9));
        search.addTextChangedListener(new TextWatcher() {
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            public void afterTextChanged(Editable s) {
                filter();
            }
        });
        searchRow.addView(search, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button find = Ui.button(activity, "Найти", true);
        find.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                String text = search.getText().toString().trim();
                if (!text.isEmpty() && shown.isEmpty()) {
                    HomeScreen.this.activity.openCatalog(text);  // not among my packs: look in the catalog
                }
            }
        });
        LinearLayout.LayoutParams findParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        findParams.leftMargin = dp(6);
        searchRow.addView(find, findParams);
        Button withAi = Ui.button(activity, "🔮 Собрать с ИИ", false);
        withAi.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                HomeScreen.this.activity.askWizard(search.getText().toString());
            }
        });
        searchRow.addView(withAi, new LinearLayout.LayoutParams(findParams));
        column.addView(searchRow);
        TextView mineTitle = Ui.label(activity, "Мои сборки", 20, Ui.TEXT, true);
        mineTitle.setPadding(dp(2), dp(12), 0, dp(6));
        column.addView(mineTitle);
        android.widget.HorizontalScrollView mineScroll = new android.widget.HorizontalScrollView(activity);
        mineScroll.setHorizontalScrollBarEnabled(false);
        mine = new LinearLayout(activity);
        mineScroll.addView(mine);
        column.addView(mineScroll);
        TextView recommended = Ui.label(activity, "Рекомендуем", 20, Ui.TEXT, true);
        recommended.setPadding(dp(2), dp(14), 0, dp(6));
        column.addView(recommended);
        android.widget.HorizontalScrollView recScroll = new android.widget.HorizontalScrollView(activity);
        recScroll.setHorizontalScrollBarEnabled(false);
        recommendations = new LinearLayout(activity);
        recScroll.addView(recommendations);
        column.addView(recScroll);
        recommendations.addView(playTogether(), recParams(dp(300)));
        loadRecommendations();

        // the page of the chosen installation: slides in from the right, like Millida's build page
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackground(PixelDrawable.panel(0xF70E0407, 0xFF8A3442, density));
        panel.setClickable(true);
        LinearLayout panelHead = new LinearLayout(activity);
        panelHead.setGravity(Gravity.CENTER_VERTICAL);
        panelHead.setPadding(dp(10), dp(6), dp(6), 0);
        panelHead.addView(Ui.label(activity, "Сборка", 11, 0xFFFF8090, true), new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button close = Ui.button(activity, "✕", false);
        close.setTextSize(13);
        close.setMinHeight(dp(34));
        close.setMinimumHeight(dp(34));
        close.setContentDescription("Закрыть");
        close.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showBuildPanel(false);
            }
        });
        panelHead.addView(close);
        panel.addView(panelHead);
        ScrollView side = new ScrollView(activity);
        actions = new LinearLayout(activity);
        actions.setOrientation(LinearLayout.VERTICAL);
        actions.setPadding(dp(12), dp(4), dp(12), dp(12));
        side.addView(actions);
        panel.addView(side, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        buildPanel = panel;
        buildPanel.setVisibility(View.GONE);
        library.addView(buildPanel, new android.widget.FrameLayout.LayoutParams(dp(250), ViewGroup.LayoutParams.MATCH_PARENT, Gravity.END));
        pages.addView(library);

        /* ---------------- the "Создать" tab */
        createPage = new CreatePage(activity, this);
        createView = createPage.view();
        pages.addView(createView);

        showPage(LOBBY);
    }

    /** the banner on the right of the lobby: Arkan, "Собери сборку словами" and a button, over a crimson pixel edge */
    private View banner() {
        float density = activity.getResources().getDisplayMetrics().density;
        android.widget.FrameLayout card = new android.widget.FrameLayout(activity);
        card.setBackground(new BannerDrawable(density));
        WizardView wizard = new WizardView(activity);
        android.widget.FrameLayout.LayoutParams wizardParams = new android.widget.FrameLayout.LayoutParams(dp(104), dp(116), Gravity.END | Gravity.TOP);
        wizardParams.setMargins(0, dp(4), dp(2), 0);
        card.addView(wizard, wizardParams);
        LinearLayout texts = new LinearLayout(activity);
        texts.setOrientation(LinearLayout.VERTICAL);
        texts.setPadding(dp(12), dp(10), dp(96), 0);
        texts.addView(tight(Ui.label(activity, "ИИ-МАГ × VELLORA", 9, Ui.HINT, true)));
        TextView headline = tight(Ui.label(activity, "СОБЕРИ СБОРКУ", 17, Ui.TEXT, true));
        headline.setPadding(0, dp(4), 0, 0);
        texts.addView(headline);
        texts.addView(tight(Ui.label(activity, "СЛОВАМИ", 17, 0xFFFF4654, true)));
        Button go = Ui.button(activity, "▶ СОБРАТЬ", true);
        go.setTextSize(12);
        go.setMinHeight(dp(32));
        go.setMinimumHeight(dp(32));
        go.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.openChat(null);
            }
        });
        LinearLayout.LayoutParams goParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        goParams.topMargin = dp(8);
        texts.addView(go, goParams);
        card.addView(texts);
        card.setClickable(true);
        card.setContentDescription("Аркан");
        card.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.openChat(null);
            }
        });
        return card;
    }

    /** the dark banner with a crimson pixel edge at the bottom, like the lava edge of Millida's promo card */
    static final class BannerDrawable extends android.graphics.drawable.Drawable {
        private final android.graphics.Paint paint = new android.graphics.Paint();
        private final PixelDrawable frame;
        private final float step;

        BannerDrawable(float density) {
            frame = PixelDrawable.panel(0xFF14060A, 0xFF060103, density);
            step = Math.max(2, Math.round(4 * density));
        }

        @Override
        public void draw(android.graphics.Canvas canvas) {
            android.graphics.Rect b = getBounds();
            frame.setBounds(b);
            frame.draw(canvas);
            int[] reds = { 0xFFE8364F, 0xFFB3263A, 0xFFFF6B7D, 0xFF8E1428, 0xFFC81F3A };
            float bottom = b.bottom - step / 2;
            int columns = (int) (b.width() / step);
            for (int i = 0; i < columns; i++) {
                int h = 2 + ((i * 7 + 3) % 3);
                for (int j = 0; j < h; j++) {
                    paint.setColor(reds[(i * 3 + j * 5) % reds.length]);
                    float x = b.left + i * step, y = bottom - (j + 1) * step;
                    canvas.drawRect(x, y, x + step, y + step, paint);
                }
            }
        }

        @Override
        public void setAlpha(int alpha) {}

        @Override
        public void setColorFilter(android.graphics.ColorFilter filter) {}

        @Override
        public int getOpacity() {
            return android.graphics.PixelFormat.TRANSLUCENT;
        }
    }

    /** "Играть по сети с другом", the big card that opens "Рекомендуем", like Millida's */
    private View playTogether() {
        LinearLayout duo = new LinearLayout(activity);
        WardrobeScreen.ItemPicture a = new WardrobeScreen.ItemPicture(activity, "pirate_hat", "", "", "");
        WardrobeScreen.ItemPicture b = new WardrobeScreen.ItemPicture(activity, "viking_helmet", "", "", "fox_pet");
        b.emote = "cheer";
        duo.addView(a, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        duo.addView(b, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        android.widget.FrameLayout card = new android.widget.FrameLayout(activity);
        card.setBackground(PixelDrawable.panel(0xFF3A0D16, 0xFF0B0305, activity.getResources().getDisplayMetrics().density));
        card.addView(duo, new android.widget.FrameLayout.LayoutParams(dp(130), ViewGroup.LayoutParams.MATCH_PARENT, Gravity.START));
        LinearLayout texts = new LinearLayout(activity);
        texts.setOrientation(LinearLayout.VERTICAL);
        texts.setGravity(Gravity.CENTER_VERTICAL);
        texts.setPadding(dp(136), dp(8), dp(10), dp(8));
        TextView tag = Ui.label(activity, "Vellora", 9, Ui.TEXT, true);
        tag.setPadding(dp(5), dp(1), dp(5), dp(2));
        tag.setBackgroundColor(0xFFB3263A);
        texts.addView(tag, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        texts.addView(Ui.label(activity, "Играть по сети с другом", 15, Ui.TEXT, true));
        Button code = Ui.button(activity, "Код приглашения", true);
        code.setTextSize(12);
        code.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.showInviteCode();
            }
        });
        LinearLayout.LayoutParams cp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        cp.topMargin = dp(6);
        texts.addView(code, cp);
        card.addView(texts);
        return card;
    }

    private LinearLayout.LayoutParams recParams(int width) {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(width, dp(150));
        p.rightMargin = dp(8);
        return p;
    }

    private void loadRecommendations() {
        activity.background(new MainActivity.Job<org.json.JSONArray>() {
            public org.json.JSONArray run() throws Exception {
                return Modrinth.search("", "[[\"project_type:modpack\"]]", "downloads", 0, 10);
            }

            public void done(org.json.JSONArray hits, Exception error) {
                if (error != null || hits == null) {
                    return;  // offline: only the card with friends
                }
                float density = activity.getResources().getDisplayMetrics().density;
                for (int i = 0; i < hits.length(); i++) {
                    final org.json.JSONObject hit = hits.optJSONObject(i);
                    // a card like Millida's: the picture with a tag, the name and what it is
                    LinearLayout card = new LinearLayout(activity);
                    card.setOrientation(LinearLayout.VERTICAL);
                    card.setPadding(dp(5), dp(5), dp(5), dp(7));
                    card.setBackground(PixelDrawable.plain(density));
                    android.widget.FrameLayout picture = new android.widget.FrameLayout(activity);
                    int color = hit.optInt("color", 0x3A0D16);
                    picture.setBackground(PixelDrawable.panel(0xFF000000 | ((color >> 1) & 0x7F7F7F), 0xFF0B0305, density));
                    ImageView icon = new ImageView(activity);
                    Icons.load(icon, hit.isNull("icon_url") ? "" : hit.optString("icon_url"));
                    picture.addView(icon, new android.widget.FrameLayout.LayoutParams(dp(60), dp(60), Gravity.CENTER));
                    TextView tag = Ui.label(activity, "▶ Сборка", 9, Ui.TEXT, true);
                    tag.setPadding(dp(5), dp(1), dp(5), dp(2));
                    tag.setBackgroundColor(0xFFB3263A);
                    android.widget.FrameLayout.LayoutParams tagParams = new android.widget.FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.TOP | Gravity.START);
                    tagParams.setMargins(dp(5), dp(5), 0, 0);
                    picture.addView(tag, tagParams);
                    card.addView(picture, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
                    TextView name = Ui.label(activity, hit.optString("title"), 13, Ui.TEXT, true);
                    name.setSingleLine(true);
                    name.setEllipsize(android.text.TextUtils.TruncateAt.END);
                    name.setPadding(dp(2), dp(4), 0, 0);
                    card.addView(name);
                    TextView sub = Ui.label(activity, "⬇ " + Ui.downloads(hit.optLong("downloads")) + " · " + hit.optString("author"), 10, Ui.HINT, false);
                    sub.setSingleLine(true);
                    sub.setPadding(dp(2), 0, 0, 0);
                    card.addView(sub);
                    card.setClickable(true);
                    card.setOnClickListener(new View.OnClickListener() {
                        public void onClick(View v) {
                            activity.openCatalog(hit.optString("title"));
                        }
                    });
                    recommendations.addView(card, recParams(dp(150)));
                }
            }
        });
    }

    /** the lobby or the library: kept for the callers that know only these two */
    void showPage(boolean lobbyPage) {
        showPage(lobbyPage ? LOBBY : LIBRARY);
    }

    /** the lobby, the library or "Создать": they slide and fade into each other, like Millida's screens, with a blip */
    void showPage(int next) {
        View[] views = { lobby, library, createView };
        for (Button b : pageTabs) {
            Ui.styleButton(activity, b, ((Integer) b.getTag()) == next);
        }
        if (next == CREATE && page != CREATE) {
            createPage.reset();
        }
        if (next != LIBRARY) {
            buildPanel.setVisibility(View.GONE);
        }
        if (next == page) {
            return;
        }
        final int before = page;
        page = next;
        final View in = views[next];
        attach(in);
        for (int i = 0; i < views.length; i++) {
            if (i != next && i != before) {
                views[i].setVisibility(View.GONE);
            }
        }
        in.setVisibility(View.VISIBLE);
        boolean barBefore = libraryBar.getVisibility() == View.VISIBLE;
        libraryBar.setVisibility(next == LOBBY ? View.GONE : View.VISIBLE);
        if (before < 0) {
            return;  // the first time: no animation, no sound
        }
        Sounds.play(Sounds.NAV);
        final float direction = next < before ? -1 : 1;
        in.setAlpha(0f);
        in.setTranslationX(dp(32) * direction);
        in.animate().alpha(1f).translationX(0).setDuration(260).setInterpolator(new android.view.animation.DecelerateInterpolator(1.6f)).start();
        if (next != LOBBY && !barBefore) {
            libraryBar.setAlpha(0f);
            libraryBar.setTranslationY(-dp(12));
            libraryBar.animate().alpha(1f).translationY(0).setDuration(220).start();
        }
        final View out = views[before];
        if (out.getParent() == pagesBox && out.getWidth() > 0) {
            // the old page leaves into the overlay: drawn while it fades, no longer touched
            pagesBox.getOverlay().add(out);
            out.animate().cancel();
            out.animate().alpha(0f).translationX(-dp(22) * direction).setDuration(200)
                .setInterpolator(new android.view.animation.AccelerateInterpolator())
                .withEndAction(new Runnable() {
                    public void run() {
                        pagesBox.getOverlay().remove(out);
                        out.setVisibility(View.GONE);
                        out.setAlpha(1f);
                        out.setTranslationX(0);
                        if (out.getParent() == null) {
                            pagesBox.addView(out);
                        }
                    }
                })
                .start();
        } else {
            out.setVisibility(View.GONE);
        }
    }

    /** a page back in its place (it may still be fading out in the overlay) */
    private void attach(View view) {
        view.animate().cancel();
        pagesBox.getOverlay().remove(view);
        if (view.getParent() == null) {
            pagesBox.addView(view);
        }
        view.setAlpha(1f);
        view.setTranslationX(0);
    }

    /** the page of the chosen installation, sliding in from the right */
    private void showBuildPanel(boolean open) {
        if (open == (buildPanel.getVisibility() == View.VISIBLE)) {
            return;
        }
        buildPanel.animate().cancel();
        if (open) {
            Sounds.play(Sounds.OPEN);
            buildPanel.setVisibility(View.VISIBLE);
            buildPanel.setAlpha(0.4f);
            buildPanel.setTranslationX(dp(250));
            buildPanel.animate().translationX(0).alpha(1f).setDuration(240).setInterpolator(new android.view.animation.DecelerateInterpolator(1.6f)).start();
        } else {
            Sounds.play(Sounds.CLOSE);
            buildPanel.animate().translationX(dp(250)).alpha(0.4f).setDuration(200).setInterpolator(new android.view.animation.AccelerateInterpolator())
                .withEndAction(new Runnable() {
                    public void run() {
                        buildPanel.setVisibility(View.GONE);
                        buildPanel.setTranslationX(0);
                        buildPanel.setAlpha(1f);
                    }
                })
                .start();
        }
    }

    private Button tab(String text, int target) {
        Button b = Ui.button(activity, text, false);
        b.setTextSize(14);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.rightMargin = dp(4);
        b.setLayoutParams(p);
        b.setTag(target);
        if (target >= 0) {
            final int page = target;
            pageTabs.add(b);
            b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    showPage(page);
                }
            });
        }
        return b;
    }

    /** a square icon button of the top bar */
    /** a square button of the lobby's corner with a white pixel picture, like Millida's */
    private Button square(String[] icon, final String what, final Runnable action) {
        Button b = Ui.button(activity, "", false);
        b.setBackground(new PixelGlyph(b.getBackground(), icon, activity.getResources().getDisplayMetrics().density));
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
            playName.setText("Пока ничего");
            playInfo.setText("Создай сборку или выбери готовую");
            playChange.setText("Выбрать\nсборку ›");
            playIcon.setImageResource(R.drawable.install_default);
            play.setText("+ Новая");
            return;
        }
        playChange.setText("Сменить\nсборку ›");
        // the hours played in all the packs, small under "Играть", like Millida's button
        long total = 0;
        for (Installation i : all) {
            total += i.playedMillis;
        }
        String played = Installation.playTime(total);
        Account account = activity.account();
        // a guest plays the demo of the game: said small under "Играть"
        String small = account != null && account.guest ? "демо-версия" : played.isEmpty() ? "" : "наиграно " + played;
        if (small.isEmpty()) {
            play.setText("▶  Играть");
        } else {
            android.text.SpannableString text = new android.text.SpannableString("▶  Играть\n" + small);
            int cut = text.toString().indexOf('\n');
            text.setSpan(new android.text.style.RelativeSizeSpan(0.42f), cut, text.length(), android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            text.setSpan(new android.text.style.ForegroundColorSpan(0xD0FFFFFF), cut, text.length(), android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            play.setText(text);
        }
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

    /** a label without the font's extra space above and below */
    private static TextView tight(TextView t) {
        t.setIncludeFontPadding(false);
        return t;
    }

    private LinearLayout.LayoutParams tileParams() {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1);
        p.topMargin = dp(16);  // room for the heads that come out over the top of the tile
        return p;
    }

    /** shows who is signed in */
    void updateAccount() {
        Account account = activity.account();
        if (account == null || !account.signedIn()) {
            accountChip.setVisibility(View.GONE);
            return;
        }
        accountChip.setVisibility(View.VISIBLE);
        accountName.setText(account.ownsJava || account.guest ? account.name : "Без Java Edition");
        accountSub.setText(account.guest ? "Гость · демо" : account.ownsJava ? "Microsoft" : "нет игры");
        if (SkinCache.skin(activity, account) == null) {
            Icons.loadHead(head, account.skinUrl, R.drawable.wizard);
        }
        WardrobeScreen.loadSkinAndCape(activity, playerBody, new Icons.Got() {
            public void got(android.graphics.Bitmap skin) {
                android.graphics.Bitmap face = SkinCache.head(skin);
                if (face != null) {
                    head.setImageBitmap(face);  // the face of the skin kept on the phone, also offline
                }
            }
        });
        WardrobeScreen.loadSkinAndCape(activity, skinTile);
        playerBody.setNametag(account.name);
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
        showPage(LIBRARY);  // a chosen installation opens its page in the library
        fillMine();
        updateActions();
        showBuildPanel(true);
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
        fillMine();
        updateActions();
    }

    /** "Мои сборки": a card for each, like Millida's (a picture on top, the name, the loader and version), and "+" at the end */
    private void fillMine() {
        float density = activity.getResources().getDisplayMetrics().density;
        mine.removeAllViews();
        for (final Installation inst : shown) {
            boolean chosen = inst.id.equals(selectedId);
            LinearLayout card = new LinearLayout(activity);
            card.setOrientation(LinearLayout.VERTICAL);
            card.setPadding(dp(5), dp(5), dp(5), dp(8));
            card.setBackground(chosen ? Ui.rainbowFrame(activity, 0) : PixelDrawable.plain(density));
            android.widget.FrameLayout picture = new android.widget.FrameLayout(activity);
            picture.setBackground(PixelDrawable.panel(chosen ? 0xFF4A1420 : 0xFF2A0A12, 0xFF0B0305, density));
            ImageView icon = new ImageView(activity);
            Icons.load(icon, inst.iconUrl, inst.chatId.isEmpty() ? R.drawable.install_default : R.drawable.wizard);
            picture.addView(icon, new android.widget.FrameLayout.LayoutParams(dp(58), dp(58), Gravity.CENTER));
            card.addView(picture, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(82)));
            TextView name = Ui.label(activity, inst.name, 13, Ui.TEXT, true);
            name.setSingleLine(true);
            name.setEllipsize(android.text.TextUtils.TruncateAt.END);
            name.setPadding(dp(2), dp(5), 0, 0);
            card.addView(name);
            TextView sub = Ui.label(activity, Plan.loaderName(inst.loader) + " · " + inst.minecraftVersion
                                                  + (inst.count(Mod.MOD) > 0 ? " · модов: " + inst.count(Mod.MOD) : ""), 10, Ui.HINT, false);
            sub.setSingleLine(true);
            sub.setPadding(dp(2), 0, 0, 0);
            card.addView(sub);
            card.setClickable(true);
            card.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    select(inst.id);
                }
            });
            mine.addView(card, mineParams());
        }
        // the last card: a new one, like Millida's "Все версии · Новая сборка"
        LinearLayout add = new LinearLayout(activity);
        add.setOrientation(LinearLayout.VERTICAL);
        add.setPadding(dp(5), dp(5), dp(5), dp(8));
        add.setBackground(PixelDrawable.plain(density));
        TextView plus = Ui.label(activity, "+", 34, 0xFFFF4654, true);
        plus.setGravity(Gravity.CENTER);
        plus.setBackground(PixelDrawable.panel(0xFF1C080D, 0xFF0B0305, density));
        add.addView(plus, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(82)));
        TextView addName = Ui.label(activity, "Все версии", 13, Ui.TEXT, true);
        addName.setPadding(dp(2), dp(5), 0, 0);
        add.addView(addName);
        TextView addSub = Ui.label(activity, "Новая сборка", 10, Ui.HINT, false);
        addSub.setPadding(dp(2), 0, 0, 0);
        add.addView(addSub);
        add.setClickable(true);
        add.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                newInstallation();
            }
        });
        mine.addView(add, mineParams());
    }

    private LinearLayout.LayoutParams mineParams() {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(dp(160), ViewGroup.LayoutParams.WRAP_CONTENT);
        p.rightMargin = dp(8);
        return p;
    }

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
                    showBuildPanel(false);
                    refresh();
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    /* ------------------------------------------------------------ new installation */

    /** "+ Новая сборка": the "Создать" tab, like Millida's "Новая сборка" */
    void newInstallation() {
        showPage(CREATE);
    }

    void create(final String name, final String mc, final String loader, final boolean essentials, final boolean fpsBoost) {
        create(name, mc, loader, essentials, fpsBoost, new ArrayList<String>());
    }

    /** a new installation with the mods chosen on "Создать" put in at once (with what they need); a mod without a file
        for this version is left out and named */
    void create(final String name, final String mc, final String loader, final boolean essentials, final boolean fpsBoost, final List<String> mods) {
        final Progress progress = new Progress(activity, "Создаю «" + name + "»");
        final List<String> missing = new ArrayList<String>();
        activity.background(new MainActivity.Job<Installation>() {
            public Installation run() throws Exception {
                Installation inst = Installer.create(name, mc, loader, essentials, progress);
                inst.fpsBoost = fpsBoost;
                for (String slug : mods) {
                    progress.log("Ставлю " + slug + "...");
                    try {
                        Installer.install(inst, slug, Mod.MOD);
                    } catch (Exception e) {
                        if (!String.valueOf(e.getMessage()).contains("уже установлен")) {
                            missing.add(slug + ": " + e.getMessage());
                        }
                    }
                }
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
                if (!missing.isEmpty()) {
                    new AlertDialog.Builder(activity)
                        .setTitle("Готово, но не всё")
                        .setMessage("«" + inst.name + "» создана. Эти моды не поставились:\n\n" + android.text.TextUtils.join("\n", missing))
                        .setPositiveButton("OK", null)
                        .show();
                } else {
                    Toast.makeText(activity, "Установка «" + inst.name + "» готова" + (mods.isEmpty() ? ". Добавь моды в «Моды и ресурсы»." : " — с модами!"),
                                   Toast.LENGTH_LONG).show();
                }
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
