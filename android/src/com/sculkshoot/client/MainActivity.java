package com.sculkshoot.client;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Html;
import android.text.InputType;
import android.text.method.LinkMovementMethod;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/**
 * Lullaby Launcher for Android: a chat with Moony, who assembles a mod pack from Modrinth exactly like the
 * desktop launcher, keeps every chat, and exports the pack as .mrpack for any Android Minecraft launcher.
 * Also decodes invite codes of the desktop launcher's "My server".
 */
public final class MainActivity extends Activity {
    private static final int STAGE_TOPIC = 0, STAGE_SETUP = 1, STAGE_BUILDING = 2, STAGE_READY = 3;
    private static final int REQUEST_SAVE = 1;
    private static final int REQUEST_STORAGE = 2;

    /** Minecraft: Java Edition launchers for Android that can install Modrinth packs (name, package) */
    private static final String[][] JAVA_LAUNCHERS = {
        { "PojavLauncher", "net.kdt.pojavlaunch" },
        { "Amethyst", "org.angelauramc.amethyst" },
        { "Fold Craft Launcher", "com.tungsten.fcl" },
        { "Zalith Launcher", "com.movtery.zalithlauncher" },
    };

    // sculk colors of the desktop theme
    // night black and moon white; main buttons wear a see-through rainbow
    private static final int BG = 0xFF0E0407, PANEL = 0xFF1C080D, PANEL_LIGHT = 0xFF2B0E15, ACCENT = 0xFFFFFFFF, ACCENT_DARK = 0xFF8A3442,
                             TEXT = 0xFFFBEFF1, HINT = 0xFFD1A3AA, USER_BUBBLE = 0xFF4A1420, LINE = 0xFF5A1E2A;
    private static final int[] RAINBOW_PASTEL = { 0xFFFF5A6E, 0xFFE8364F, 0xFFC81F3A, 0xFFA8162E, 0xFFC81F3A, 0xFFFF8090 };

    private static final String[] TOPIC_REPLIES = { "Зомби-апокалипсис: города, зомби и оружие", "Магия и приключения с подземельями",
                                                    "Технологии и космос", "Только оптимизация для слабого ПК" };
    private static final String[] SETUP_REPLIES = { "NeoForge 1.21.1", "Fabric 1.20.1", "Forge 1.20.1", "Forge 1.12.2" };
    private static final String[] LOADER_REPLIES = { "NeoForge", "Fabric", "Forge", "Quilt" };
    private static final String[] VERSION_REPLIES = { "1.21.1", "1.20.1", "1.19.2", "1.16.5" };
    private static final String[] AFTER_REPLIES = { "Добавь магию", "Добавь шейдеры", "Начать заново" };

    private final Handler main = new Handler(Looper.getMainLooper());
    private ChatStore store;
    private WizardView wizard;  // Arkan in the chat's top bar
    private Installation.Store installations;

    // screens: home (like the main window of the desktop launcher), the chat with Moony, an installation, Modrinth
    private HomeScreen home;
    private View chatRoot;
    private View current;
    private EditorScreen editor;
    private Account account;
    private LoginScreen login;
    private ControlsScreen controls;
    private BrowserScreen browser;
    private Runnable afterPermission;

    /** work for a background thread whose result comes back on the main thread */
    interface Job<T> {
        T run() throws Exception;

        void done(T result, Exception error);
    }
    private Typeface pixel;

    // conversation
    private int stage = STAGE_TOPIC;
    private String topic = "";
    private String loader = "";
    private String version = "";
    private List<String> unknownWords = new ArrayList<String>();
    private Plan pendingPlan;

    // the current chat
    private String chatId;
    private String chatTitle = "";
    private boolean chatTitleEdited;
    private final List<String[]> messages = new ArrayList<String[]>();  // { "warden" | "user", text }
    private List<String> currentReplies = new ArrayList<String>();
    private boolean replaying;

    // the pack
    private Plan packPlan;
    private String packLoaderVersion = "";
    private List<Mod> packMods = new ArrayList<Mod>();
    private final StringBuilder workLog = new StringBuilder();
    private Builder builder;
    private File pendingExport;

    // widgets
    private LinearLayout chatColumn;
    private ScrollView chatScroll;
    private LinearLayout repliesRow;
    private HorizontalScrollView repliesScroll;
    private EditText input;
    private LinearLayout packBar;
    private TextView packTitle;
    private TextView typingStatus;
    private View typingRow;

    /* ------------------------------------------------------------ lifecycle */

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        Sounds.init(this);
        store = new ChatStore(getFilesDir());
        try {
            pixel = Typeface.createFromAsset(getAssets(), "Tiny5-Regular.ttf");
        } catch (RuntimeException e) {
            pixel = Typeface.DEFAULT_BOLD;
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        getWindow().setStatusBarColor(BG);
        getWindow().setNavigationBarColor(BG);
        installations = new Installation.Store(getFilesDir());
        chatRoot = buildLayout();
        home = new HomeScreen(this);

        // the chat continues the most recent conversation, like a messenger
        List<JSONObject> chats = store.list();
        if (!chats.isEmpty()) {
            loadChat(chats.get(0).optString("id"));
        } else {
            newChat();
        }
        // the menu opens only for a signed-in Microsoft account
        account = Account.load(this);
        if (account.signedIn()) {
            showHome();
            if (account.microsoft()) {
                refreshAccount();  // a guest has nothing to renew
            }
        } else {
            showLogin(null);
        }
    }

    private static MainActivity resumedActivity;

    /** the window on screen now, or null (the Microsoft sign-in reports to it) */
    static MainActivity resumed() {
        return resumedActivity;
    }

    @Override
    protected void onResume() {
        super.onResume();
        Sounds.startMusic(this);
        resumedActivity = this;
        if (GameEngine.get() != null) {
            GameEngine.get().attach(this);
        }
        checkPlaySession();
        // back from the browser: the sign-in may have finished meanwhile
        if (login != null && current == login.view()) {
            signInFinished(LoginScreen.lastProblem);
        }
    }

    /** the game started from here has ended: its time goes to the installation's play time (at most 12 hours a session) */
    private void checkPlaySession() {
        android.content.SharedPreferences session = getSharedPreferences("session", MODE_PRIVATE);
        String id = session.getString("id", null);
        if (id == null || GameEngine.get() == null || GameEngine.gameRunning(this)) {
            return;
        }
        long start = session.getLong("start", 0);
        session.edit().clear().apply();
        Installation inst = installations.get(id);
        if (inst == null || start <= 0) {
            return;
        }
        long played = Math.max(0, Math.min(System.currentTimeMillis() - start, 12L * 3600 * 1000));
        inst.playedMillis += played;
        saveInstallation(inst);
        if (home != null) {
            home.refresh();
        }
    }

    /** the Microsoft sign-in thread is done: open the menu, or show why it did not work */
    void signInFinished(String problem) {
        LoginScreen.lastProblem = null;
        Account saved = Account.load(this);
        if (saved.signedIn()) {
            if (login != null && current == login.view()) {
                signedIn(saved);
            }
            return;
        }
        if (problem != null && login != null) {
            login.failed(problem);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Sounds.pauseMusic();
        if (resumedActivity == this) {
            resumedActivity = null;
        }
        if (GameEngine.get() != null) {
            GameEngine.get().detach(this);
        }
    }

    /* ------------------------------------------------------------ Microsoft account */

    Account account() {
        return account;
    }

    void showLogin(String why) {
        login = new LoginScreen(this);
        if (why != null) {
            login.setMessage(why);
        }
        login.resumePending();  // a code entered while the app was closed is still being waited for
        show(login.view());
    }

    /** "Войти как гость": a nickname, the menu, everything but the AI and the game itself */
    void signedInAsGuest(String name) {
        account = Account.guest(name);
        account.save(this);
        Account.clearPending(this);
        login = null;
        showHome();
        Toast.makeText(this, "Привет, " + name + "! Ты гость: ИИ и запуск игры — после входа через Microsoft.", Toast.LENGTH_LONG).show();
    }

    /** the AI (Arkan's chats and builds, the AI skin and cape generators) needs a Microsoft account */
    boolean aiAllowed() {
        return account != null && account.microsoft();
    }

    /** true when the AI may be used; for a guest it explains why not and offers the Microsoft sign-in */
    boolean requireAi() {
        if (aiAllowed()) {
            return true;
        }
        Sounds.play(Sounds.OPEN);
        new AlertDialog.Builder(this)
            .setTitle("🔒 ИИ — после входа")
            .setMessage("Аркан и ИИ-генераторы скинов и плащей работают с аккаунтом Microsoft. Гостю доступно всё остальное: "
                        + "сборки, версии, моды, каталог и гардероб.")
            .setPositiveButton("Войти через Microsoft", new android.content.DialogInterface.OnClickListener() {
                public void onClick(android.content.DialogInterface d, int w) {
                    leaveGuest();
                }
            })
            .setNegativeButton("Позже", null)
            .show();
        return false;
    }

    /** the guest goes to the Microsoft sign-in; the packs and the wardrobe stay */
    void leaveGuest() {
        Account.signOut(this);
        account = new Account();
        showLogin(null);
    }

    /** the sign-in on the login screen worked */
    void signedIn(Account signed) {
        account = signed;
        account.save(this);
        Account.clearPending(this);
        login = null;
        showHome();
        if (!account.ownsJava) {
            new AlertDialog.Builder(this)
                .setTitle("Вход выполнен")
                .setMessage("На этом аккаунте Microsoft нет Minecraft: Java Edition. Собирать установки можно, а чтобы играть, "
                            + "игру нужно купить на minecraft.net (или войти в другой аккаунт через меню ⋮).")
                .setPositiveButton("OK", null)
                .show();
        } else {
            Toast.makeText(this, "Привет, " + account.name + "!", Toast.LENGTH_SHORT).show();
        }
    }

    /** the start of the app: renews the sign-in quietly; only a sign-in Microsoft no longer accepts sends back to the login */
    private void refreshAccount() {
        background(new Job<Account>() {
            public Account run() throws Exception {
                Account fresh = Account.load(MainActivity.this);
                fresh.refresh();
                return fresh;
            }

            public void done(Account fresh, Exception error) {
                // only a sign-in Microsoft no longer takes ends it; no internet, a busy server (429) or a slow step
                // keep the saved account, its skin and its cape (before, any of them signed the player out)
                if (error instanceof Account.SessionExpired) {
                    Account.signOut(MainActivity.this);
                    account = new Account();
                    showLogin(error.getMessage());
                    return;
                }
                if (error != null) {
                    android.util.Log.w("Vellora", "the sign-in was not renewed now: " + error.getMessage());
                    return;
                }
                if (fresh != null) {  // without internet the saved profile stays
                    account = fresh;
                    account.save(MainActivity.this);
                    home.updateAccount();
                }
            }
        });
    }

    void openControls() {
        controls = new ControlsScreen(this);
        show(controls.view());
    }

    void showAccount() {
        if (account != null && account.guest) {
            final android.widget.EditText nick = new android.widget.EditText(this);
            nick.setText(account.name);
            nick.setSingleLine(true);
            new AlertDialog.Builder(this)
                .setTitle("Гость: " + account.name)
                .setMessage("Ты вошёл без аккаунта Microsoft. Можно собирать установки, ставить моды и наряжаться в гардеробе. "
                            + "ИИ (Аркан, генераторы скинов и плащей) и запуск игры — после входа через Microsoft "
                            + "с купленной Minecraft: Java Edition.\n\nНик гостя:")
                .setView(nick)
                .setPositiveButton("Войти через Microsoft", new android.content.DialogInterface.OnClickListener() {
                    public void onClick(android.content.DialogInterface d, int w) {
                        leaveGuest();
                    }
                })
                .setNeutralButton("Сохранить ник", new android.content.DialogInterface.OnClickListener() {
                    public void onClick(android.content.DialogInterface d, int w) {
                        String name = nick.getText().toString().trim();
                        if (!Account.validName(name)) {
                            Toast.makeText(MainActivity.this, "Ник: 3–16 латинских букв, цифр или _", Toast.LENGTH_LONG).show();
                            return;
                        }
                        account.name = name;
                        account.save(MainActivity.this);
                        home.updateAccount();
                    }
                })
                .setNegativeButton("Закрыть", null)
                .show();
            return;
        }
        String text = account.ownsJava ? "Ник: " + account.name + "\nMinecraft: Java Edition есть на аккаунте."
                                       : "На этом аккаунте нет Minecraft: Java Edition.";
        new AlertDialog.Builder(this)
            .setTitle("Аккаунт Microsoft")
            .setMessage(text)
            .setPositiveButton("Выйти из аккаунта", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    Account.signOut(MainActivity.this);
                    account = new Account();
                    showLogin("Ты вышел из аккаунта. Чтобы открыть меню, войди снова.");
                }
            })
            .setNegativeButton("Закрыть", null)
            .show();
    }

    /* ------------------------------------------------------------ screens */

    Installation.Store store() {
        return installations;
    }

    <T> void background(final Job<T> job) {
        new Thread(new Runnable() {
            public void run() {
                T result = null;
                Exception error = null;
                try {
                    result = job.run();
                } catch (Exception e) {
                    error = e;
                }
                final T r = result;
                final Exception e = error;
                main.post(new Runnable() {
                    public void run() {
                        if (!isFinishing()) {
                            job.done(r, e);
                        }
                    }
                });
            }
        }, "lullaby-job").start();
    }

    private android.widget.FrameLayout screens;  // holds the screen; the one leaving fades out above it, in its overlay

    private void show(View view) {
        if (current == view) {
            return;
        }
        if (screens == null) {
            screens = new android.widget.FrameLayout(this);
            screens.setBackgroundColor(BG);
            setContentView(screens);
        }
        // like Millida's screens: the new one slides in and fades up while the old one slides away and fades out;
        // back to the lobby they move the other way
        final boolean backwards = home != null && view == home.view();
        final View old = current;
        current = view;
        final float shift = Ui.dp(this, 36) * (backwards ? -1 : 1);
        view.animate().cancel();
        screens.getOverlay().remove(view);
        if (view.getParent() instanceof ViewGroup) {
            ((ViewGroup) view.getParent()).removeView(view);
        }
        view.setClickable(true);  // a tap on an empty place of the new screen never reaches the old one
        screens.addView(view, new android.widget.FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        if (old == null) {
            return;  // the first screen: no animation, no sound
        }
        Sounds.play(backwards ? Sounds.CLOSE : Sounds.OPEN);
        view.setAlpha(0f);
        view.setTranslationX(shift);
        view.animate().alpha(1f).translationX(0).setDuration(280).setInterpolator(new android.view.animation.DecelerateInterpolator(1.6f)).start();
        if (old.getParent() == screens) {
            // the old screen leaves into the overlay: still drawn while it fades, but no longer touched or found
            screens.getOverlay().add(old);
            old.animate().cancel();
            old.animate().alpha(0f).translationX(-shift * 0.6f).setDuration(220).setInterpolator(new android.view.animation.AccelerateInterpolator())
                .withEndAction(new Runnable() {
                    public void run() {
                        screens.getOverlay().remove(old);
                        old.setAlpha(1f);
                        old.setTranslationX(0);
                    }
                })
                .start();
        }
    }

    void showHome() {
        showHome(null);
    }

    void showHome(String selectId) {
        if (account != null && !account.signedIn()) {
            showLogin(null);
            return;
        }
        controls = null;
        home.refresh();
        home.updateAccount();
        if (selectId != null) {
            home.select(selectId);
        }
        editor = null;
        browser = null;
        show(home.view());
    }

    void openChat(String id) {
        if (!requireAi()) {
            return;
        }
        if (id != null && !id.equals(chatId)) {
            loadChat(id);
        }
        show(chatRoot);
        scrollDown();
    }

    /** "Собрать с ИИ" in the catalog: a new chat with Arkan that starts from these words */
    void askWizard(String text) {
        if (!requireAi()) {
            return;
        }
        newChat();
        show(chatRoot);
        if (text != null && !text.trim().isEmpty()) {
            userSaid(text.trim());
        }
        scrollDown();
    }

    void openEditor(Installation inst) {
        editor = new EditorScreen(this, inst);
        browser = null;
        show(editor.view());
    }

    void openBrowser(Installation inst, String kind) {
        browser = new BrowserScreen(this, inst, kind);
        show(browser.view());
    }

    /** the library of installations (the catalog's "Библиотека" tab) */
    void showLibrary() {
        showHome();
        if (home != null) {
            home.showPage(false);
        }
    }

    /** the lobby (the catalog's "‹ Лобби") */
    void showLobby() {
        showHome();
        if (home != null) {
            home.showPage(true);
        }
    }

    /** the wardrobe: outfits, capes and accessories on the player (like Millida Launcher's) */
    void openWardrobe() {
        openWardrobe(WardrobeScreen.SETS);
    }

    /** the wardrobe on this tab ("Мой скин" opens the skins) */
    void openWardrobe(String tab) {
        wardrobe = new WardrobeScreen(this);
        wardrobe.showTab(tab);
        show(wardrobe.view());
    }

    /** the "Создать" tab: a new installation and the ready packs */
    void showCreate() {
        showHome();
        if (home != null) {
            home.showPage(HomeScreen.CREATE);
        }
    }

    private WardrobeScreen wardrobe;
    private static final int REQUEST_SKIN = 43;

    /** "Загрузить скин" in the wardrobe: a PNG from the phone */
    void pickSkinFile(WardrobeScreen from) {
        wardrobe = from;
        Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        pick.addCategory(Intent.CATEGORY_OPENABLE);
        pick.setType("image/*");
        try {
            startActivityForResult(pick, REQUEST_SKIN);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, "Нет приложения для выбора файлов", Toast.LENGTH_LONG).show();
        }
    }

    private static final int REQUEST_SAVE_IMAGE = 44;
    private Bitmap pendingImage;

    /** "💾 PNG" of the generator: the picture goes where the player chooses */
    void saveImage(Bitmap image, String name) {
        pendingImage = image;
        Intent create = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        create.addCategory(Intent.CATEGORY_OPENABLE);
        create.setType("image/png");
        create.putExtra(Intent.EXTRA_TITLE, name);
        try {
            startActivityForResult(create, REQUEST_SAVE_IMAGE);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, "Нет приложения для сохранения файлов", Toast.LENGTH_LONG).show();
        }
    }

    /** the skin or the name changed: the lobby shows the new one */
    void accountChanged() {
        if (home != null) {
            home.updateAccount();
        }
    }

    /** installs a modpack (Modrinth, or CurseForge with the "cf:" prefix) as a new installation and opens it */
    void installModpack(final String projectId, final String name) {
        final HomeScreen.Progress progress = new HomeScreen.Progress(this, "Устанавливаю «" + name + "»");
        background(new Job<Installation>() {
            public Installation run() throws Exception {
                Installation pack = Installer.importModpack(projectId, installations, progress);
                installations.save(pack);
                return pack;
            }

            public void done(Installation pack, Exception error) {
                progress.dismiss();
                if (error != null) {
                    new AlertDialog.Builder(MainActivity.this)
                        .setTitle("Не удалось установить «" + name + "»")
                        .setMessage(error instanceof Installer.InstallException ? error.getMessage() : "Нет связи (" + error.getMessage() + ")")
                        .setPositiveButton("OK", null)
                        .show();
                    return;
                }
                Toast.makeText(MainActivity.this, "Сборка «" + pack.name + "» установлена: " + pack.describe() + BrowserScreen.disabledNote(pack),
                               Toast.LENGTH_LONG).show();
                showHome(pack.id);
            }
        });
    }

    /** the modpack catalog with this pack already found */
    void openCatalog(String text) {
        openBrowser(null, BrowserScreen.MODPACK);
        browser.searchFor(text);
    }

    void saveInstallation(Installation inst) {
        try {
            installations.save(inst);
        } catch (Exception e) {
            Toast.makeText(this, "Не удалось сохранить: " + e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    void showChatList() {
        if (!requireAi()) {
            return;
        }
        showChats();
    }

    void showAppMenu() {
        new AlertDialog.Builder(this)
            .setItems(GameEngine.get() != null ? new String[] { "Управление в игре", "Аккаунт Microsoft", "Код приглашения друга", "О приложении", "Java для игры", "Журнал игры", "Графика", "Память для игры" }
                                               : new String[] { "Управление в игре", "Аккаунт Microsoft", "Код приглашения друга", "О приложении" },
                      new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    if (which == 0) {
                        openControls();
                    } else if (which == 1) {
                        showAccount();
                    } else if (which == 2) {
                        showInviteCode();
                    } else if (which == 4) {
                        showJavaDialog(0, null);
                    } else if (which == 5) {
                        showGameLog();
                    } else if (which == 6) {
                        showGraphics();
                    } else if (which == 7) {
                        showMemory();
                    } else {
                        showAbout();
                    }
                }
            })
            .show();
    }

    @Override
    public void onBackPressed() {
        if (current == home.view() || (login != null && current == login.view())) {
            super.onBackPressed();
        } else if (browser != null && current == browser.view()) {
            browser.goBack();
        } else {
            showHome();
        }
    }

    /** the pack Moony built in this chat is also an installation, so it can be changed and played from the main screen */
    private void syncInstallationFromChat() {
        if (packPlan == null || chatId == null) {
            return;
        }
        Installation inst = installations.forChat(chatId);
        if (inst == null) {
            inst = new Installation();
            inst.chatId = chatId;
            inst.name = packPlan.name;
        }
        inst.minecraftVersion = packPlan.minecraftVersion;
        inst.loader = packPlan.loader;
        inst.loaderVersion = packLoaderVersion;
        // what was added by hand in the editor stays (resource packs, shaders, mods without a theme group),
        // with the libraries they need; Moony's own mods are replaced by the new build
        List<Mod> old = inst.mods;
        inst.mods = new ArrayList<Mod>(packMods);
        for (Mod mod : old) {
            boolean byHand = !Mod.MOD.equals(mod.kind) || (mod.group.isEmpty() && !mod.dependency);
            if (byHand && inst.find(mod.projectId) == null) {
                inst.mods.add(mod);
            }
        }
        boolean added = true;
        while (added) {
            added = false;
            for (Mod mod : new ArrayList<Mod>(inst.mods)) {
                for (String required : mod.required) {
                    String id = required.startsWith("mr:") ? required.substring(3) : "";
                    if (id.isEmpty() || inst.find(id) != null) {
                        continue;
                    }
                    for (Mod library : old) {
                        if (id.equals(library.projectId)) {
                            inst.mods.add(library);
                            added = true;
                        }
                    }
                }
            }
        }
        saveInstallation(inst);
    }


    @Override
    protected void onDestroy() {
        if (builder != null) {
            builder.cancel();
        }
        super.onDestroy();
    }

    private int dp(float value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, getResources().getDisplayMetrics());
    }

    private android.graphics.drawable.Drawable rounded(int color, float radius, int stroke) {
        return Ui.rounded(this, color, radius, stroke);
    }

    private Button button(String text, boolean accent) {
        Button b = new Button(this);
        b.setText(text);
        b.setAllCaps(false);
        b.setTypeface(pixel);
        b.setTextSize(15);
        b.setTextColor(accent ? Color.WHITE : TEXT);
        b.setBackground(accent ? Ui.rainbow(this, 10) : PixelDrawable.plain(getResources().getDisplayMetrics().density));
        b.setPadding(dp(14), dp(6), dp(14), dp(9));
        b.setMinHeight(dp(40));
        b.setMinimumHeight(dp(40));
        return b;
    }

    private View buildLayout() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new RainDrawable(getResources().getDisplayMetrics().density));

        // top bar: Moony, the name, chats and the invite code
        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(12), dp(8), dp(8), dp(8));
        bar.setBackgroundColor(0xCC140509);
        wizard = new WizardView(this);
        bar.addView(wizard, new LinearLayout.LayoutParams(dp(48), dp(48)));
        LinearLayout titles = new LinearLayout(this);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.setPadding(dp(10), 0, 0, 0);
        TextView title = new TextView(this);
        title.setText("Маг Аркан");
        title.setTypeface(pixel);
        title.setTextSize(18);
        title.setTextColor(ACCENT);
        titles.addView(title);
        TextView subtitle = new TextView(this);
        subtitle.setText("Vellora Client · моды с Modrinth");
        subtitle.setTextSize(11);
        subtitle.setTextColor(HINT);
        titles.addView(subtitle);
        bar.addView(titles, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button homeButton = button("⌂", false);
        homeButton.setTextSize(18);
        homeButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showHome();
            }
        });
        LinearLayout.LayoutParams homeParams = new LinearLayout.LayoutParams(dp(48), ViewGroup.LayoutParams.WRAP_CONTENT);
        homeParams.rightMargin = dp(6);
        bar.addView(homeButton, homeParams);
        Button chats = button("Чаты", false);
        chats.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showChats();
            }
        });
        bar.addView(chats);
        Button more = button("⋮", false);
        more.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showMenu();
            }
        });
        LinearLayout.LayoutParams moreParams = new LinearLayout.LayoutParams(dp(44), ViewGroup.LayoutParams.WRAP_CONTENT);
        moreParams.leftMargin = dp(6);
        bar.addView(more, moreParams);
        root.addView(bar);

        // the conversation
        chatScroll = new ScrollView(this);
        chatScroll.setFillViewport(true);
        chatColumn = new LinearLayout(this);
        chatColumn.setOrientation(LinearLayout.VERTICAL);
        chatColumn.setPadding(dp(10), dp(10), dp(10), dp(10));
        chatScroll.addView(chatColumn);
        // the screen is landscape: the chat on the left, the finished pack in a column on the right
        LinearLayout middle = new LinearLayout(this);
        middle.addView(chatScroll, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        root.addView(middle, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        // the finished pack: name, mod list and export
        packBar = new LinearLayout(this);
        packBar.setOrientation(LinearLayout.VERTICAL);
        packBar.setPadding(dp(12), dp(8), dp(12), dp(8));
        packBar.setBackground(rounded(0xE61C080D, 12, LINE));
        packTitle = new TextView(this);
        packTitle.setTextColor(TEXT);
        packTitle.setTextSize(13);
        packBar.addView(packTitle);
        LinearLayout packButtons = new LinearLayout(this);
        packButtons.setOrientation(LinearLayout.VERTICAL);
        packButtons.setPadding(0, dp(8), 0, 0);
        Button list = button("Моды", false);
        list.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showModList();
            }
        });
        Button play = button("Играть", true);
        play.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                play();
            }
        });
        Button share = button("Файл", false);
        share.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showExportChoices();
            }
        });
        // "Play" first and biggest, the other two below it
        LinearLayout.LayoutParams row = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        row.bottomMargin = dp(5);
        play.setMinHeight(dp(48));
        play.setMinimumHeight(dp(48));
        play.setTextSize(18);
        for (Button small : new Button[] { list, share }) {
            small.setMinHeight(dp(34));
            small.setMinimumHeight(dp(34));
            small.setPadding(dp(10), dp(2), dp(10), dp(2));
        }
        packButtons.addView(play, row);
        packButtons.addView(list, new LinearLayout.LayoutParams(row));
        packButtons.addView(share, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        packBar.addView(packButtons);
        LinearLayout.LayoutParams packParams = new LinearLayout.LayoutParams(dp(260), ViewGroup.LayoutParams.WRAP_CONTENT);
        packParams.setMargins(dp(4), dp(8), dp(10), dp(4));
        packBar.setVisibility(View.GONE);
        middle.addView(packBar, packParams);

        // quick replies
        repliesScroll = new HorizontalScrollView(this);
        repliesScroll.setHorizontalScrollBarEnabled(false);
        repliesRow = new LinearLayout(this);
        repliesRow.setPadding(dp(8), dp(4), dp(8), dp(4));
        repliesScroll.addView(repliesRow);
        root.addView(repliesScroll);

        // the input line
        LinearLayout inputRow = new LinearLayout(this);
        inputRow.setGravity(Gravity.CENTER_VERTICAL);
        inputRow.setPadding(dp(8), dp(4), dp(8), dp(10));
        input = new EditText(this);
        input.setHint("Напиши Аркану...");
        input.setHintTextColor(HINT);
        input.setTextColor(TEXT);
        input.setTextSize(15);
        input.setBackground(rounded(0xE61C080D, 20, LINE));
        input.setPadding(dp(14), dp(10), dp(14), dp(10));
        input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
        input.setImeOptions(EditorInfo.IME_ACTION_SEND);
        input.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                sendInput();
                return true;
            }
        });
        inputRow.addView(input, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button send = button("➤", true);
        send.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                sendInput();
            }
        });
        LinearLayout.LayoutParams sendParams = new LinearLayout.LayoutParams(dp(52), dp(46));
        sendParams.leftMargin = dp(6);
        inputRow.addView(send, sendParams);
        root.addView(inputRow);
        return root;
    }

    /* ------------------------------------------------------------ chat widgets */

    private void scrollDown() {
        chatScroll.post(new Runnable() {
            public void run() {
                chatScroll.fullScroll(View.FOCUS_DOWN);
            }
        });
    }

    private View addRow(View bubble, boolean fromMoony) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(fromMoony ? Gravity.START : Gravity.END);
        LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        rowParams.bottomMargin = dp(8);
        // the screen is landscape: bubbles keep a comfortable reading width instead of spanning it
        int maxWidth = Math.min((int) (getResources().getDisplayMetrics().widthPixels * 0.82f), dp(560));
        if (fromMoony) {
            ImageView avatar = new ImageView(this);
            avatar.setImageResource(R.drawable.wizard);
            LinearLayout.LayoutParams avatarParams = new LinearLayout.LayoutParams(dp(30), dp(30));
            avatarParams.rightMargin = dp(6);
            row.addView(avatar, avatarParams);
        }
        if (bubble instanceof TextView) {
            ((TextView) bubble).setMaxWidth(maxWidth);
        }
        row.addView(bubble, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        int index = typingRow != null ? chatColumn.indexOfChild(typingRow) : chatColumn.getChildCount();
        chatColumn.addView(row, index < 0 ? chatColumn.getChildCount() : index, rowParams);
        if (!replaying) {
            row.setAlpha(0f);
            row.setTranslationY(dp(12));
            row.animate().alpha(1f).translationY(0).setDuration(220).start();
        }
        scrollDown();
        return row;
    }

    @SuppressWarnings("deprecation")
    private void showMoonyBubble(String html) {
        TextView bubble = new TextView(this);
        bubble.setText(Html.fromHtml(html));
        bubble.setTextColor(TEXT);
        bubble.setTextSize(14.5f);
        bubble.setLineSpacing(0, 1.1f);
        bubble.setTextIsSelectable(true);
        bubble.setMovementMethod(LinkMovementMethod.getInstance());
        bubble.setBackground(rounded(0xF02B0E15, 14, LINE));
        bubble.setPadding(dp(12), dp(9), dp(12), dp(9));
        addRow(bubble, true);
    }

    private void showUserBubble(String text) {
        TextView bubble = new TextView(this);
        bubble.setText(text);
        bubble.setTextColor(Color.WHITE);
        bubble.setTextSize(14.5f);
        bubble.setTextIsSelectable(true);
        bubble.setBackground(rounded(USER_BUBBLE, 14, 0));
        bubble.setPadding(dp(12), dp(9), dp(12), dp(9));
        addRow(bubble, false);
    }

    private void say(String html, String... replies) {
        showMoonyBubble(html);
        messages.add(new String[] { "warden", html });
        setReplies(Arrays.asList(replies));
        saveChat();
    }

    private void sayWithReplies(String html, List<String> replies) {
        showMoonyBubble(html);
        messages.add(new String[] { "warden", html });
        setReplies(replies);
        saveChat();
    }

    private void addUserMessage(String text) {
        showUserBubble(text);
        messages.add(new String[] { "user", text });
        if (chatTitle.isEmpty()) {
            chatTitle = text.length() > 40 ? text.substring(0, 38).trim() + "…" : text;
        }
        saveChat();
    }

    private void setReplies(List<String> replies) {
        currentReplies = new ArrayList<String>(replies);
        repliesRow.removeAllViews();
        for (final String reply : replies) {
            Button chip = button(reply, false);
            chip.setTextSize(13);
            chip.setBackground(rounded(PANEL, 18, ACCENT_DARK));
            chip.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    userSaid(reply);
                }
            });
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, dp(40));
            params.rightMargin = dp(6);
            repliesRow.addView(chip, params);
        }
        repliesScroll.setVisibility(replies.isEmpty() ? View.GONE : View.VISIBLE);
        repliesScroll.scrollTo(0, 0);
    }

    private void showTyping(String status) {
        hideTyping();
        LinearLayout bubble = new LinearLayout(this);
        bubble.setOrientation(LinearLayout.VERTICAL);
        bubble.setBackground(rounded(0xF02B0E15, 14, LINE));
        bubble.setPadding(dp(12), dp(9), dp(12), dp(9));
        TextView head = new TextView(this);
        head.setText("Аркан работает...");
        head.setTypeface(pixel);
        head.setTextColor(ACCENT);
        bubble.addView(head);
        typingStatus = new TextView(this);
        typingStatus.setText(status);
        typingStatus.setTextColor(HINT);
        typingStatus.setTextSize(13);
        typingStatus.setMaxWidth(Math.min((int) (getResources().getDisplayMetrics().widthPixels * 0.7f), dp(480)));
        bubble.addView(typingStatus);
        ProgressBar progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        progress.setIndeterminate(true);
        bubble.addView(progress, new LinearLayout.LayoutParams(dp(220), dp(10)));
        typingRow = addRow(bubble, true);
        if (wizard != null) {
            wizard.setBusy(true);
        }
    }

    private void hideTyping() {
        if (wizard != null) {
            wizard.setBusy(false);
        }
        if (typingRow != null) {
            chatColumn.removeView(typingRow);
            typingRow = null;
            typingStatus = null;
        }
    }

    /* ------------------------------------------------------------ conversation */

    private static boolean isReset(String lower) {
        for (String word : new String[] { "заново", "сначала", "сброс", "новая сборка", "новый чат", "start over", "reset", "new pack" }) {
            if (lower.contains(word)) {
                return true;
            }
        }
        return false;
    }

    private void sendInput() {
        String text = input.getText().toString().trim();
        if (text.isEmpty()) {
            return;
        }
        input.setText("");
        userSaid(text);
    }

    private void userSaid(String text) {
        String lower = text.toLowerCase(Locale.ROOT);
        if (isReset(lower)) {
            newChat();
            return;
        }
        addUserMessage(text);
        if (stage == STAGE_BUILDING) {
            say("Я ещё собираю сборку, подожди немного.");
            return;
        }
        Plan parsed = Parser.parse(text);
        boolean setupChanged = false;
        if (parsed.versionGiven && !parsed.minecraftVersion.equals(version)) {
            version = parsed.minecraftVersion;
            setupChanged = true;
        }
        if (parsed.loaderGiven && !parsed.loader.equals(loader)) {
            loader = parsed.loader;
            setupChanged = true;
        }
        boolean topical = !parsed.themes.isEmpty() || !parsed.requestedMods.isEmpty() || !parsed.excluded.isEmpty()
                          || Parser.wantsOnlyPerformance(text) || Parser.wantsShaders(text) || Parser.wantsMap(text);
        if (topical) {
            // every message is kept, so later wishes ("убери зомби", "добавь Create") change the earlier ones
            topic = topic.isEmpty() ? text : topic + ". " + text;
        }
        unknownWords = parsed.unknownWords;

        if (stage == STAGE_READY) {
            if (topical || setupChanged) {
                startBuild();
            } else {
                say(unknownNote() + "Скажи, что поменять: добавить тему или мод («добавь магию», «добавь Create»), что-то убрать "
                        + "(«убери зомби»), другой загрузчик или версию («Fabric 1.20.1») или «начать заново». "
                        + "Или нажми <b>Играть</b>, чтобы поставить сборку.", AFTER_REPLIES);
            }
            return;
        }
        if (!topical && !setupChanged) {
            say(unknownNote() + "Я не понял тему. Расскажи, что должно быть в сборке: зомби, оружие, магия, технологии, города, "
                    + "приключения, хоррор, космос... Можно назвать и сами моды на английском, например <i>Create</i> или "
                    + "<i>Farmer's Delight</i>.", TOPIC_REPLIES);
            return;
        }
        continueConversation();
    }

    private String unknownNote() {
        if (unknownWords.isEmpty()) {
            return "";
        }
        return "Я не знаю этих слов: <i>" + Util.escape(Util.join(unknownWords, ", "))
            + "</i>. Скажи по-другому или напиши название мода на английском, как на Modrinth.<br>";
    }

    private static String themeList(Plan plan) {
        List<String> titles = new ArrayList<String>();
        for (Plan.Theme theme : plan.themes) {
            titles.add(theme.title.toLowerCase(Locale.ROOT));
        }
        if (titles.isEmpty() && !plan.requestedMods.isEmpty()) {
            return "названные тобой моды";
        }
        return titles.isEmpty() ? "только оптимизация" : Util.join(titles, ", ");
    }

    private static String wishesNote(Plan plan) {
        String note = "";
        if (!plan.requestedMods.isEmpty()) {
            note += " Названные тобой моды: <b>" + Util.escape(Util.join(plan.requestedMods, ", ")) + "</b>.";
        }
        if (!plan.excluded.isEmpty()) {
            note += " Не добавляю: <b>" + Util.escape(Util.join(plan.excluded, ", ")) + "</b>.";
        }
        return note;
    }

    private void continueConversation() {
        if (topic.isEmpty()) {
            stage = STAGE_TOPIC;
            say("Хорошо. А про что будет сборка?", TOPIC_REPLIES);
            return;
        }
        Plan plan = Parser.planFor(topic, loader, version);
        if (loader.isEmpty() && version.isEmpty()) {
            stage = STAGE_SETUP;
            say(unknownNote() + "Понял: <b>" + Util.escape(themeList(plan)) + "</b>." + wishesNote(plan)
                    + " Теперь напиши загрузчик и версию Minecraft. Подойдёт любая версия, для которой есть загрузчик, например "
                    + "<i>NeoForge 1.21.1</i>, <i>Forge 1.12.2</i> или <i>Fabric 1.20.1</i>.", SETUP_REPLIES);
            return;
        }
        if (loader.isEmpty()) {
            stage = STAGE_SETUP;
            say("Minecraft " + version + ". Какой загрузчик: NeoForge, Fabric, Forge или Quilt?", LOADER_REPLIES);
            return;
        }
        if (version.isEmpty()) {
            stage = STAGE_SETUP;
            say("Значит, " + Plan.loaderName(loader) + ". Какая версия Minecraft? Напиши любую, например 1.20.1 или 1.12.2.", VERSION_REPLIES);
            return;
        }
        startBuild();
    }

    private void startBuild() {
        stage = STAGE_BUILDING;
        pendingPlan = Parser.planFor(topic, loader, version);
        say(unknownNote() + "Понял! Тема: <b>" + Util.escape(themeList(pendingPlan)) + "</b>. Minecraft <b>" + pendingPlan.minecraftVersion
            + "</b> с <b>" + Plan.loaderName(pendingPlan.loader) + "</b>." + wishesNote(pendingPlan)
            + " Ищу моды на Modrinth, добавляю оптимизацию и все библиотеки, которые нужны модам.");
        setReplies(new ArrayList<String>());
        showTyping("Планирую сборку...");
        workLog.setLength(0);

        final String buildChat = chatId;
        final Builder job = new Builder(pendingPlan, new Builder.Listener() {
            public void log(final String line) {
                main.post(new Runnable() {
                    public void run() {
                        if (buildChat.equals(chatId)) {
                            workLog.append(line).append('\n');
                            if (typingStatus != null) {
                                typingStatus.setText(line);
                            }
                        }
                    }
                });
            }
        });
        builder = job;
        new Thread(new Runnable() {
            public void run() {
                String error = null;
                try {
                    job.run();
                } catch (Builder.BuildException e) {
                    error = e.getMessage();
                } catch (Exception e) {
                    error = "нет связи с Modrinth (" + e.getClass().getSimpleName() + "). Проверь интернет.";
                }
                final String failure = error;
                main.post(new Runnable() {
                    public void run() {
                        if (job.isCancelled() || !buildChat.equals(chatId)) {
                            return;
                        }
                        builder = null;
                        if (failure != null) {
                            buildFailed(failure);
                        } else {
                            buildFinished(job);
                        }
                    }
                });
            }
        }, "warden-builder").start();
    }

    private void buildFailed(String reason) {
        hideTyping();
        stage = STAGE_SETUP;
        say("Что-то пошло не так: " + Util.escape(reason) + "<br>Давай попробуем другой загрузчик или версию.", SETUP_REPLIES);
    }

    private void buildFinished(Builder job) {
        hideTyping();
        stage = STAGE_READY;
        packPlan = job.plan;
        packLoaderVersion = job.loaderVersion;
        packMods = job.mods;
        if (!chatTitleEdited) {
            chatTitle = packPlan.name;
        }
        updatePackBar();

        int themed = 0, libraries = 0;
        List<String> curatedGroups = new ArrayList<String>();
        for (Plan.CuratedMod curated : packPlan.curated) {
            curatedGroups.add(curated.group);
        }
        for (Mod mod : packMods) {
            if (mod.dependency) {
                libraries++;
            } else if (!curatedGroups.contains(mod.group)) {
                themed++;
            }
        }
        StringBuilder details = new StringBuilder();
        for (Plan.Theme theme : packPlan.themes) {
            List<String> titles = new ArrayList<String>();
            for (Mod mod : packMods) {
                if (!mod.dependency && mod.group.equals(theme.title)) {
                    titles.add(mod.title);
                }
            }
            if (!titles.isEmpty()) {
                details.append("<br><b>").append(Util.escape(theme.title)).append("</b>: ").append(Util.escape(Util.join(titles, ", "))).append('.');
            }
        }
        if (!job.foundRequested.isEmpty()) {
            details.append("<br>Добавил запрошенные моды: <b>").append(Util.escape(Util.join(job.foundRequested, ", "))).append("</b>.");
        }
        if (!job.missingRequested.isEmpty()) {
            details.append("<br>Не нашёл для Minecraft ").append(packPlan.minecraftVersion).append(" с ").append(Plan.loaderName(packPlan.loader))
                .append(": <b>").append(Util.escape(Util.join(job.missingRequested, ", "))).append("</b>. Проверь название или попробуй другую версию.");
        }
        List<String> replies = new ArrayList<String>(Arrays.asList(AFTER_REPLIES));
        if (!job.emptyThemes.isEmpty()) {
            details.append("<br><b>Для Minecraft ").append(packPlan.minecraftVersion).append(" с ").append(Plan.loaderName(packPlan.loader))
                .append(" нет модов про: ").append(Util.escape(Util.join(job.emptyThemes, ", "))).append(".</b> ");
            if (!job.suggestedSetup.isEmpty()) {
                details.append("Больше всего их для <b>").append(job.suggestedSetup).append("</b> (нашёл ").append(job.suggestedSetupMods)
                    .append("), напиши это или нажми кнопку внизу, и я пересоберу сборку.");
                replies.add(0, job.suggestedSetup);
            } else {
                details.append("Попробуй другую версию Minecraft или другой загрузчик.");
            }
        }
        List<String> libraryNotes = new ArrayList<String>();
        for (Mod mod : packMods) {
            if (mod.dependency) {
                libraryNotes.add(mod.neededBy.isEmpty() ? mod.title : mod.title + " (для " + Util.join(mod.neededBy, ", ") + ")");
            }
        }
        if (!libraryNotes.isEmpty()) {
            details.append("<br>Библиотеки, которые скачаются сами: ").append(Util.escape(Util.join(libraryNotes, "; "))).append('.');
        }
        sayWithReplies("Готово! <b>" + Util.escape(packPlan.name) + "</b>: модов — " + packMods.size() + ". По твоим темам: " + themed
                           + ", оптимизация и удобство: " + (packMods.size() - themed - libraries) + ", библиотеки для модов: " + libraries + "."
                           + details + "<br>Нажми <b>Играть</b>: я сохраню сборку и открою лаунчер Minecraft на телефоне, "
                           + "или <b>Моды</b>, чтобы снять лишнее. Или напиши, что поменять.",
                       replies);
    }

    private void updatePackBar() {
        if (stage != STAGE_READY || packPlan == null) {
            packBar.setVisibility(View.GONE);
            return;
        }
        int enabled = 0;
        for (Mod mod : packMods) {
            if (mod.enabled && !mod.dependency) {
                enabled++;
            }
        }
        packTitle.setText(Html.fromHtml("<b>" + Util.escape(packPlan.name) + "</b><br>Minecraft " + packPlan.minecraftVersion + ", "
                                        + Plan.loaderName(packPlan.loader) + " " + packLoaderVersion + " · модов: " + enabled + " + библиотеки"));
        packBar.setVisibility(View.VISIBLE);
    }

    /* ------------------------------------------------------------ saved chats */

    private void newChat() {
        if (builder != null) {
            builder.cancel();
            builder = null;
        }
        hideTyping();
        chatColumn.removeAllViews();
        chatId = UUID.randomUUID().toString();
        chatTitle = "";
        chatTitleEdited = false;
        messages.clear();
        stage = STAGE_TOPIC;
        topic = "";
        loader = "";
        version = "";
        unknownWords = new ArrayList<String>();
        packPlan = null;
        packMods = new ArrayList<Mod>();
        packLoaderVersion = "";
        workLog.setLength(0);
        updatePackBar();
        say("Привет! Я Аркан, маг-сборщик, собираю сборки модов. <b>Про что будет твоя сборка?</b> Расскажи тему и что в ней должно быть, например: "
                + "<i>зомби-апокалипсис, должны быть города, зомби и оружие</i>.",
            TOPIC_REPLIES);
    }

    private void saveChat() {
        if (replaying || chatId == null) {
            return;
        }
        // a chat with only the greeting is not worth keeping
        boolean userSpoke = false;
        for (String[] message : messages) {
            userSpoke = userSpoke || "user".equals(message[0]);
        }
        if (!userSpoke) {
            return;
        }
        try {
            JSONObject chat = new JSONObject();
            chat.put("formatVersion", 1);
            chat.put("id", chatId);
            chat.put("title", chatTitle);
            chat.put("titleEdited", chatTitleEdited);
            chat.put("updated", System.currentTimeMillis());
            chat.put("stage", stage);
            chat.put("topic", topic);
            chat.put("loader", loader);
            chat.put("version", version);
            JSONArray list = new JSONArray();
            for (String[] message : messages) {
                list.put(new JSONObject().put("from", message[0]).put("text", message[1]));
            }
            chat.put("messages", list);
            chat.put("replies", new JSONArray(currentReplies));
            if (stage == STAGE_READY && packPlan != null) {
                JSONArray mods = new JSONArray();
                for (Mod mod : packMods) {
                    mods.put(mod.toJson());
                }
                JSONObject pack = new JSONObject();
                pack.put("name", packPlan.name);
                pack.put("summary", packPlan.summary);
                pack.put("minecraftVersion", packPlan.minecraftVersion);
                pack.put("loader", packPlan.loader);
                pack.put("loaderVersion", packLoaderVersion);
                pack.put("mods", mods);
                pack.put("log", workLog.toString());
                chat.put("pack", pack);
            }
            store.save(chatId, chat);
            if (stage == STAGE_READY && packPlan != null) {
                syncInstallationFromChat();
            }
        } catch (Exception e) {
            Toast.makeText(this, "Не удалось сохранить чат: " + e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    private void loadChat(String id) {
        JSONObject chat = store.load(id);
        if (chat == null) {
            newChat();
            return;
        }
        if (builder != null) {
            builder.cancel();
            builder = null;
        }
        hideTyping();
        chatColumn.removeAllViews();
        chatId = id;
        chatTitle = chat.optString("title");
        chatTitleEdited = chat.optBoolean("titleEdited");
        topic = chat.optString("topic");
        loader = chat.optString("loader");
        version = chat.optString("version");
        unknownWords = new ArrayList<String>();
        messages.clear();
        workLog.setLength(0);

        replaying = true;
        JSONArray list = chat.optJSONArray("messages");
        for (int i = 0; list != null && i < list.length(); i++) {
            JSONObject message = list.optJSONObject(i);
            String from = message.optString("from");
            String text = message.optString("text");
            if ("warden".equals(from)) {
                showMoonyBubble(text);
            } else {
                showUserBubble(text);
            }
            messages.add(new String[] { from, text });
        }
        replaying = false;
        setReplies(Mod.strings(chat.optJSONArray("replies")));

        JSONObject pack = chat.optJSONObject("pack");
        if (pack != null) {
            packPlan = new Plan();
            packPlan.name = pack.optString("name");
            packPlan.summary = pack.optString("summary");
            packPlan.minecraftVersion = pack.optString("minecraftVersion");
            packPlan.loader = pack.optString("loader");
            packLoaderVersion = pack.optString("loaderVersion");
            packMods = new ArrayList<Mod>();
            JSONArray mods = pack.optJSONArray("mods");
            for (int i = 0; mods != null && i < mods.length(); i++) {
                packMods.add(Mod.fromJson(mods.optJSONObject(i)));
            }
            workLog.append(pack.optString("log"));
            stage = STAGE_READY;
        } else {
            packPlan = null;
            packMods = new ArrayList<Mod>();
            stage = Math.max(STAGE_TOPIC, Math.min(chat.optInt("stage"), STAGE_BUILDING));
        }
        updatePackBar();
        if (stage == STAGE_BUILDING) {
            // the app was closed while the pack was being assembled: finish the job
            if (!topic.isEmpty() && !loader.isEmpty() && !version.isEmpty()) {
                startBuild();
            } else {
                stage = STAGE_SETUP;
            }
        }
        scrollDown();
    }

    private void showChats() {
        final List<JSONObject> chats = store.list();
        if (chats.isEmpty()) {
            Toast.makeText(this, "Сохранённых чатов пока нет. Напиши Аркану, про что будет сборка.", Toast.LENGTH_LONG).show();
            return;
        }
        final List<String> rows = new ArrayList<String>();
        DateFormat format = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT);
        for (JSONObject chat : chats) {
            JSONObject pack = chat.optJSONObject("pack");
            String details = pack == null ? "ещё не собрана"
                                          : "Minecraft " + pack.optString("minecraftVersion") + ", " + Plan.loaderName(pack.optString("loader")) + ", модов: "
                                                + pack.optJSONArray("mods").length();
            String current = chat.optString("id").equals(chatId) ? "▶ " : "";
            rows.add(current + chat.optString("title", "Чат") + "\n" + details + " · " + format.format(new Date(chat.optLong("updated"))));
        }
        ListView list = new ListView(this);
        list.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, rows));
        final AlertDialog dialog = new AlertDialog.Builder(this)
                                       .setTitle("Мои сборки")
                                       .setView(list)
                                       .setPositiveButton("Новый чат", new DialogInterface.OnClickListener() {
                                           public void onClick(DialogInterface d, int which) {
                                               newChat();
                                           }
                                       })
                                       .setNegativeButton("Закрыть", null)
                                       .create();
        list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                dialog.dismiss();
                loadChat(chats.get(position).optString("id"));
            }
        });
        list.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                dialog.dismiss();
                chatActions(chats.get(position));
                return true;
            }
        });
        dialog.show();
        Toast.makeText(this, "Долгое нажатие на чат: переименовать или удалить", Toast.LENGTH_SHORT).show();
    }

    private void chatActions(final JSONObject chat) {
        final String id = chat.optString("id");
        new AlertDialog.Builder(this)
            .setTitle(chat.optString("title"))
            .setItems(new String[] { "Переименовать", "Удалить" }, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    if (which == 0) {
                        renameChat(chat);
                    } else {
                        new AlertDialog.Builder(MainActivity.this)
                            .setMessage("Удалить чат «" + chat.optString("title") + "»?")
                            .setPositiveButton("Удалить", new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface d2, int w) {
                                    store.delete(id);
                                    if (id.equals(chatId)) {
                                        newChat();
                                    }
                                }
                            })
                            .setNegativeButton("Отмена", null)
                            .show();
                    }
                }
            })
            .show();
    }

    private void renameChat(final JSONObject chat) {
        final EditText name = new EditText(this);
        name.setText(chat.optString("title"));
        name.setSelectAllOnFocus(true);
        new AlertDialog.Builder(this)
            .setTitle("Название чата")
            .setView(name)
            .setPositiveButton("Сохранить", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    String title = name.getText().toString().trim();
                    if (title.isEmpty()) {
                        return;
                    }
                    String id = chat.optString("id");
                    if (id.equals(chatId)) {
                        chatTitle = title;
                        chatTitleEdited = true;
                        saveChat();
                    } else {
                        try {
                            chat.put("title", title);
                            chat.put("titleEdited", true);
                            store.save(id, chat);
                        } catch (Exception e) {
                            Toast.makeText(MainActivity.this, e.getMessage(), Toast.LENGTH_SHORT).show();
                        }
                    }
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    /* ------------------------------------------------------------ menu, invite codes */

    private void showMenu() {
        new AlertDialog.Builder(this)
            .setItems(new String[] { "Новый чат", "Мои сборки", "Код приглашения друга", "Журнал работы", "О приложении" },
                      new DialogInterface.OnClickListener() {
                          public void onClick(DialogInterface d, int which) {
                              switch (which) {
                                  case 0:
                                      newChat();
                                      break;
                                  case 1:
                                      showChats();
                                      break;
                                  case 2:
                                      showInviteCode();
                                      break;
                                  case 3:
                                      showLog();
                                      break;
                                  default:
                                      showAbout();
                              }
                          }
                      })
            .show();
    }

    void showInviteCode() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(20), dp(8), dp(20), 0);
        TextView hint = new TextView(this);
        hint.setText("Вставь код, который прислал друг из Vellora Client на компьютере («Мой сервер»). Я покажу адрес сервера.");
        box.addView(hint);
        final EditText code = new EditText(this);
        code.setHint("LL-XXXX-XXXX-...");
        code.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS);
        ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
        if (clipboard != null && clipboard.hasPrimaryClip() && clipboard.getPrimaryClip().getItemCount() > 0) {
            CharSequence clip = clipboard.getPrimaryClip().getItemAt(0).getText();
            String clipText = clip == null ? "" : clip.toString().trim().toUpperCase(Locale.ROOT);
            if (clipText.startsWith("LL-") || clipText.startsWith("SC-")) {
                code.setText(clip.toString().trim());
            }
        }
        box.addView(code);
        new AlertDialog.Builder(this)
            .setTitle("Код приглашения")
            .setView(box)
            .setPositiveButton("Показать адрес", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    showAddress(code.getText().toString());
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    private void showAddress(String code) {
        InviteCode.Address address = InviteCode.decode(code);
        if (address == null) {
            new AlertDialog.Builder(this).setMessage("Это не похоже на код приглашения Vellora. Проверь, что код скопирован целиком.")
                .setPositiveButton("OK", null).show();
            return;
        }
        final String full = address.host + ":" + address.port;
        new AlertDialog.Builder(this)
            .setTitle("Код принят")
            .setMessage("Сервер: " + address.host + "\nПорт: " + address.port + "\n\nВ лаунчере Minecraft на телефоне запусти ту же сборку, "
                        + "открой «Сетевая игра» → «Добавить сервер» и вставь адрес:\n" + full)
            .setPositiveButton("Скопировать адрес", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                    clipboard.setPrimaryClip(ClipData.newPlainText("Minecraft server", full));
                    Toast.makeText(MainActivity.this, "Адрес скопирован: " + full, Toast.LENGTH_SHORT).show();
                }
            })
            .setNegativeButton("Закрыть", null)
            .show();
    }

    private void showLog() {
        TextView text = new TextView(this);
        text.setText(workLog.length() == 0 ? "Журнал пуст: Аркан ещё ничего не собирал в этом чате." : workLog.toString());
        text.setTextSize(12);
        text.setTextIsSelectable(true);
        text.setPadding(dp(20), dp(10), dp(20), dp(10));
        ScrollView scroll = new ScrollView(this);
        scroll.addView(text);
        new AlertDialog.Builder(this).setTitle("Журнал работы").setView(scroll).setPositiveButton("OK", null).show();
    }

    private void showAbout() {
        new AlertDialog.Builder(this)
            .setTitle("Vellora Client для Android")
            .setMessage("Аркан собирает сборки модов с Modrinth: понимает тему, названия модов, «убери» и «побольше», "
                        + "сам добавляет оптимизацию и все нужные библиотеки.\n\n"
                        + "Сама Minecraft: Java Edition на телефоне запускается в лаунчере для Android, который умеет ставить сборки "
                        + "Modrinth (.mrpack). Кнопка «Играть» сохраняет сборку в «Загрузки/Vellora» и открывает такой лаунчер, «Файл» — отправляет .mrpack.\n\n"
                        + "Шрифт Tiny5 — SIL Open Font License.")
            .setPositiveButton("OK", null)
            .show();
    }

    /* ------------------------------------------------------------ mod list and export */

    private void showModList() {
        if (packPlan == null) {
            return;
        }
        LinearLayout list = new LinearLayout(this);
        list.setOrientation(LinearLayout.VERTICAL);
        list.setPadding(dp(16), dp(4), dp(16), dp(4));
        String group = null;
        for (final Mod mod : packMods) {
            String modGroup = mod.dependency ? Builder.GROUP_LIBRARIES : mod.group;
            if (!modGroup.equals(group)) {
                group = modGroup;
                TextView header = new TextView(this);
                header.setText(group);
                header.setTypeface(pixel);
                header.setTextColor(ACCENT);
                header.setTextSize(15);
                header.setPadding(0, dp(10), 0, dp(2));
                list.addView(header);
            }
            if (mod.dependency) {
                TextView library = new TextView(this);
                library.setText(mod.title + (mod.neededBy.isEmpty() ? "" : "\n   нужна для: " + Util.join(mod.neededBy, ", ")));
                library.setTextSize(13);
                library.setPadding(dp(8), dp(4), 0, dp(4));
                list.addView(library);
                continue;
            }
            CheckBox box = new CheckBox(this);
            box.setText(mod.title + (mod.description.isEmpty() ? "" : "\n" + mod.description));
            box.setTextSize(13);
            box.setChecked(mod.enabled);
            box.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                public void onCheckedChanged(CompoundButton button, boolean checked) {
                    mod.enabled = checked;
                }
            });
            list.addView(box);
        }
        ScrollView scroll = new ScrollView(this);
        scroll.addView(list);
        new AlertDialog.Builder(this)
            .setTitle(packPlan.name)
            .setView(scroll)
            .setPositiveButton("Готово", null)
            .setOnDismissListener(new DialogInterface.OnDismissListener() {
                public void onDismiss(DialogInterface d) {
                    updatePackBar();
                    saveChat();
                }
            })
            .show();
    }

    private void showExportChoices() {
        new AlertDialog.Builder(this)
            .setTitle("Файл сборки (.mrpack)")
            .setItems(new String[] { "Открыть в лаунчере Minecraft", "Отправить (Telegram, почта...)", "Сохранить в файлы" },
                      new DialogInterface.OnClickListener() {
                          public void onClick(DialogInterface d, int which) {
                              exportPack(which);
                          }
                      })
            .show();
    }

    private File writePack() throws Exception {
        String safe = packPlan.name.replace('—', '-').replaceAll("[^\\p{L}\\p{N} ._-]", "").replaceAll("\\s+", " ").trim();
        if (safe.isEmpty()) {
            safe = "Vellora pack";
        }
        File file = new File(PackProvider.exportDir(this), safe + ".mrpack");
        ZipOutputStream zip = new ZipOutputStream(new FileOutputStream(file));
        try {
            zip.putNextEntry(new ZipEntry("modrinth.index.json"));
            zip.write(Builder.makeIndex(packPlan, packLoaderVersion, packMods).getBytes("UTF-8"));
            zip.closeEntry();
        } finally {
            zip.close();
        }
        return file;
    }

    /** 0: open in a Minecraft launcher, 1: share, 2: save with the system file picker. */
    private void exportPack(int how) {
        if (packPlan == null) {
            return;
        }
        File file;
        try {
            file = writePack();
        } catch (Exception e) {
            Toast.makeText(this, "Не удалось создать файл: " + e.getMessage(), Toast.LENGTH_LONG).show();
            return;
        }
        Uri uri = PackProvider.uriFor(file);
        String type = "application/x-modrinth-modpack+zip";
        try {
            if (how == 0) {
                // launchers register the Modrinth type, some only zip files
                Intent view = null;
                for (String mime : new String[] { type, "application/zip" }) {
                    Intent candidate = new Intent(Intent.ACTION_VIEW);
                    candidate.setDataAndType(uri, mime);
                    candidate.setClipData(ClipData.newRawUri(packPlan.name, uri));
                    candidate.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    if (!getPackageManager().queryIntentActivities(candidate, 0).isEmpty()) {
                        view = candidate;
                        break;
                    }
                }
                if (view == null) {
                    noLauncher();
                } else {
                    startActivity(Intent.createChooser(view, "Открыть сборку в лаунчере"));
                }
            } else if (how == 1) {
                Intent send = new Intent(Intent.ACTION_SEND);
                send.setType(type);
                send.putExtra(Intent.EXTRA_STREAM, uri);
                send.putExtra(Intent.EXTRA_SUBJECT, packPlan.name);
                send.setClipData(ClipData.newRawUri(packPlan.name, uri));
                send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                startActivity(Intent.createChooser(send, "Отправить сборку"));
            } else {
                pendingExport = file;
                Intent create = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                create.addCategory(Intent.CATEGORY_OPENABLE);
                create.setType("application/octet-stream");
                create.putExtra(Intent.EXTRA_TITLE, file.getName());
                startActivityForResult(create, REQUEST_SAVE);
            }
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, "Нет приложения для этого действия.", Toast.LENGTH_LONG).show();
        }
    }

    /* ------------------------------------------------------------ play on the phone */

    /** Saves the pack into Downloads/Lullaby, where every launcher's "import modpack" finds it; returns where it is, or null. */
    String saveToDownloads(File pack) {
        try {
            if (android.os.Build.VERSION.SDK_INT >= 29) {
                android.content.ContentValues values = new android.content.ContentValues();
                values.put("_display_name", pack.getName());
                values.put("mime_type", "application/octet-stream");  // keeps the .mrpack extension
                values.put("relative_path", "Download/Vellora");
                Uri target = getContentResolver().insert(Uri.parse("content://media/external/downloads"), values);
                if (target == null) {
                    return null;
                }
                copy(pack, getContentResolver().openOutputStream(target));
                return "Загрузки/Vellora/" + pack.getName();
            }
            if (!hasStoragePermission()) {
                requestPermissions(new String[] { android.Manifest.permission.WRITE_EXTERNAL_STORAGE }, REQUEST_STORAGE);
                return null;
            }
            File dir = new File(android.os.Environment.getExternalStoragePublicDirectory(android.os.Environment.DIRECTORY_DOWNLOADS), "Vellora");
            dir.mkdirs();
            File target = new File(dir, pack.getName());
            copy(pack, new FileOutputStream(target));
            return "Загрузки/Vellora/" + pack.getName();
        } catch (Exception e) {
            return null;
        }
    }

    /** Android 5 grants the permission at install time, Android 6 to 9 ask for it */
    private boolean hasStoragePermission() {
        return android.os.Build.VERSION.SDK_INT < 23
               || checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == android.content.pm.PackageManager.PERMISSION_GRANTED;
    }

    private static void copy(File from, OutputStream out) throws java.io.IOException {
        java.io.FileInputStream in = new java.io.FileInputStream(from);
        try {
            byte[] buffer = new byte[16384];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
        } finally {
            in.close();
            out.close();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(requestCode, permissions, results);
        if (requestCode == REQUEST_STORAGE && results.length > 0 && results[0] == android.content.pm.PackageManager.PERMISSION_GRANTED
            && afterPermission != null) {
            afterPermission.run();
        }
    }

    /** The Java Edition launchers that are installed on this phone. */
    private List<String[]> installedLaunchers() {
        List<String[]> found = new ArrayList<String[]>();
        for (String[] launcher : JAVA_LAUNCHERS) {
            if (getPackageManager().getLaunchIntentForPackage(launcher[1]) != null) {
                found.add(launcher);
            }
        }
        return found;
    }

    /**
     * Minecraft: Java Edition itself runs in a Java launcher for Android; Lullaby hands the pack over to it:
     * the pack goes to Downloads, the launcher opens, and its "import modpack" installs Minecraft, the loader and every mod.
     */
    private void play() {
        if (packPlan == null) {
            return;
        }
        if (GameEngine.get() != null && chatId != null) {
            // the full build: the pack of this chat is an installation, and it starts right here
            syncInstallationFromChat();
            Installation inst = installations.forChat(chatId);
            if (inst != null) {
                playInstallation(inst);
                return;
            }
        }
        File file;
        try {
            file = writePack();
        } catch (Exception e) {
            Toast.makeText(this, "Не удалось создать файл: " + e.getMessage(), Toast.LENGTH_LONG).show();
            return;
        }
        afterPermission = new Runnable() {
            public void run() {
                play();
            }
        };
        playFile(file, packPlan.minecraftVersion, packPlan.loader);
    }

    private File exportFile(Installation inst) throws Exception {
        String safe = inst.name.replace('—', '-').replaceAll("[^\\p{L}\\p{N} ._-]", "").replaceAll("\\s+", " ").trim();
        File file = new File(PackProvider.exportDir(this), (safe.isEmpty() ? "Vellora pack" : safe) + ".mrpack");
        Installer.writeMrpack(inst, installations, file);
        return file;
    }

    /** writes the .mrpack in the background (CurseForge files are downloaded into it), then does what is next */
    private void withPackFile(final Installation inst, final String doing, final Job<File> next) {
        final HomeScreen.Progress progress = new HomeScreen.Progress(this, doing);
        progress.log("Готовлю файл сборки «" + inst.name + "»...");
        background(new Job<File>() {
            public File run() throws Exception {
                return exportFile(inst);
            }

            public void done(File file, Exception error) {
                progress.dismiss();
                if (error != null) {
                    Toast.makeText(MainActivity.this, "Не удалось создать файл: " + error.getMessage(), Toast.LENGTH_LONG).show();
                    return;
                }
                next.done(file, null);
            }
        });
    }

    /** "Play" on the main screen or in the editor */
    void playInstallation(final Installation inst) {
        if (account != null && account.guest) {
            // the game checks the purchase through the Microsoft account: a guest builds, a signed-in player plays
            Sounds.play(Sounds.OPEN);
            new AlertDialog.Builder(this)
                .setTitle("Играть — после входа")
                .setMessage("Minecraft: Java Edition проверяет покупку через аккаунт Microsoft. Войди тем аккаунтом, на котором "
                            + "куплена игра, — сборка «" + inst.name + "» и всё, что ты собрал, останется.")
                .setPositiveButton("Войти через Microsoft", new android.content.DialogInterface.OnClickListener() {
                    public void onClick(android.content.DialogInterface d, int w) {
                        leaveGuest();
                    }
                })
                .setNegativeButton("Позже", null)
                .show();
            return;
        }
        Sounds.play(Sounds.LAUNCH);
        if (GameEngine.get() != null) {
            playHere(inst);
            return;
        }
        withPackFile(inst, "Готовлю «" + inst.name + "» к игре", new Job<File>() {
            public File run() {
                return null;
            }

            public void done(File file, Exception error) {
                afterPermission = new Runnable() {
                    public void run() {
                        playInstallation(inst);
                    }
                };
                playFile(file, inst.minecraftVersion, inst.loader);
            }
        });
    }

    /** the full build: Minecraft starts inside Lullaby on the built-in engine */
    private void playHere(final Installation inst) {
        final GameEngine engine = GameEngine.get();
        final HomeScreen.Progress progress = new HomeScreen.Progress(this, "Запускаю «" + inst.name + "»");
        progress.log("Готовлю " + inst.describe() + "...");
        engine.play(this, inst, account, progress, new GameEngine.Done() {
            public void finished(boolean started, String problem) {
                if (started) {
                    // the engine downloads Minecraft and opens the game; its progress keeps showing here.
                    // The play time is counted from now until the game's process is gone (checkPlaySession)
                    inst.lastPlayed = System.currentTimeMillis();
                    saveInstallation(inst);
                    getSharedPreferences("session", MODE_PRIVATE).edit().putString("id", inst.id).putLong("start", inst.lastPlayed).apply();
                    getSharedPreferences("lastgame", MODE_PRIVATE).edit().putString("id", inst.id).apply();
                    return;
                }
                progress.dismiss();
                if (problem != null && problem.startsWith(GameEngine.NEED_JAVA)) {
                    showJavaDialog(Integer.parseInt(problem.substring(GameEngine.NEED_JAVA.length())), inst);
                } else if (problem != null) {
                    new AlertDialog.Builder(MainActivity.this).setTitle(inst.name).setMessage(problem).setPositiveButton("OK", null).show();
                }
            }
        });
    }

    /** how much memory the game gets: automatic by the size of the pack, or 2-8 GB */
    void showMemory() {
        int phone = GameEngine.phoneMemory(this);
        final java.util.List<Integer> values = new java.util.ArrayList<Integer>();
        java.util.List<String> names = new java.util.ArrayList<String>();
        values.add(0);
        names.add("Авто — по размеру сборки (рекомендуется)");
        for (int gb : new int[] { 2, 3, 4, 5, 6, 8 }) {
            if (gb * 1024 <= phone * 75 / 100) {
                values.add(gb * 1024);
                names.add(gb + " ГБ" + (gb >= 6 ? " — для очень больших сборок" : gb <= 2 ? " — без модов и маленькие сборки" : ""));
            }
        }
        int current = GameEngine.chosenMemory(this);
        int checked = Math.max(0, values.indexOf(current));
        new AlertDialog.Builder(this)
            .setTitle("Память для игры (в телефоне " + Math.round(phone / 1024f) + " ГБ)")
            .setSingleChoiceItems(names.toArray(new String[0]), checked, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    GameEngine.chooseMemory(MainActivity.this, values.get(which));
                    Toast.makeText(MainActivity.this, "Будет при следующем «Играть»", Toast.LENGTH_SHORT).show();
                    d.dismiss();
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    /** which graphics the game uses (the engine's renderers, the ones this phone supports) */
    void showGraphics() {
        final java.util.List<String[]> choices = GameEngine.get().graphicsChoices(this);
        String current = GameEngine.chosenGraphics(this);
        String[] names = new String[choices.size()];
        int checked = 0;
        for (int i = 0; i < choices.size(); i++) {
            names[i] = choices.get(i)[1];
            if (choices.get(i)[0].equals(current)) {
                checked = i;
            }
        }
        new AlertDialog.Builder(this)
            .setTitle("Графика в игре")
            .setSingleChoiceItems(names, checked, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    GameEngine.chooseGraphics(MainActivity.this, choices.get(which)[0]);
                    Toast.makeText(MainActivity.this, "Будет при следующем «Играть»", Toast.LENGTH_SHORT).show();
                    d.dismiss();
                }
            })
            .setNegativeButton("Отмена", null)
            .show();
    }

    /** the few lines that tell why a start failed: graphics, Java, version and the errors */
    static String summary(String[] lines) {
        StringBuilder out = new StringBuilder("СВОДКА\n");
        String crashReport = null;
        java.util.List<String> errors = new java.util.ArrayList<String>();
        for (String line : lines) {
            String l = line.trim();
            if (l.startsWith("Added custom env: POJAV_RENDERER=")) {
                out.append("Графика: ").append(l.substring("Added custom env: POJAV_RENDERER=".length())).append('\n');
            } else if (l.startsWith("Info: Selected Minecraft version:") || l.startsWith("Info: Graphics device:")) {
                out.append(l.substring(6)).append('\n');
            } else if (l.contains("] [main/INFO]: Java is ")) {
                out.append(l.substring(l.indexOf("Java is "))).append('\n');
            } else if (l.contains("Crash report saved to:")) {
                crashReport = l.substring(l.indexOf("Crash report saved to:") + "Crash report saved to:".length()).replace("#@!@#", "").trim();
            } else if (l.contains("/ERROR]") || l.contains("/FATAL]") || l.startsWith("ERROR:") || l.contains("fatal error") || l.contains("SIGSEGV") || l.contains("SIGABRT")
                       || l.startsWith("# C ") || l.startsWith("# J ") || l.contains("Exception in thread") || l.contains("OutOfMemoryError")
                       || l.contains("zink") && l.toLowerCase(java.util.Locale.ROOT).contains("fail") || l.startsWith("Caused by:")) {
                errors.add(l);
            }
        }
        int from = Math.max(0, errors.size() - 12);
        if (!errors.isEmpty()) {
            out.append("Ошибки:\n");
            for (int i = from; i < errors.size(); i++) {
                out.append("• ").append(errors.get(i).length() > 160 ? errors.get(i).substring(0, 160) + "…" : errors.get(i)).append('\n');
            }
        }
        // Minecraft's own crash report says what broke and in which mod: its beginning goes into the summary
        if (crashReport != null) {
            try {
                String[] report = Http.read(new java.io.FileInputStream(crashReport)).split("\n");
                out.append("Отчёт о сбое:\n");
                int shown = 0;
                boolean started = false;
                for (String line : report) {
                    String l = line.replace("\t", "  ").trim();
                    if (!started && (l.startsWith("Description:") || l.startsWith("Time:"))) {
                        started = true;
                    }
                    if (started && !l.isEmpty() && !l.startsWith("// ") && shown < 22) {
                        out.append(l.length() > 170 ? l.substring(0, 170) + "…" : l).append('\n');
                        shown++;
                    }
                    if (l.contains("Suspected Mod") || l.contains("Mod File:")) {
                        out.append("→ ").append(l).append('\n');
                    }
                }
            } catch (Exception e) {
                out.append("Отчёт о сбое: ").append(crashReport).append('\n');
            }
        }
        return out.toString();
    }

    /** the end of the last game start's log: what to send when the game does not start */
    void showGameLog() {
        final File log = GameEngine.get().gameLog();
        String text;
        try {
            String all = log == null ? "" : Http.read(new java.io.FileInputStream(log));
            String[] lines = all.split("\n");
            StringBuilder tail = new StringBuilder();
            for (int i = Math.max(0, lines.length - 60); i < lines.length; i++) {
                tail.append(lines[i]).append('\n');
            }
            text = tail.length() == 0 ? "Журнал пуст." : summary(lines) + "\n——— конец журнала ———\n" + tail;
        } catch (Exception e) {
            text = "Журнала ещё нет: игра не запускалась.";
        }
        final String shown = text;
        TextView view = new TextView(this);
        view.setText(shown);
        view.setTextSize(11);
        view.setTextIsSelectable(true);
        view.setTypeface(Typeface.MONOSPACE);
        view.setPadding(Ui.dp(this, 14), Ui.dp(this, 8), Ui.dp(this, 14), Ui.dp(this, 8));
        android.widget.ScrollView scroll = new android.widget.ScrollView(this);
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        final AlertDialog[] opened = new AlertDialog[1];
        addCrashFixes(column, shown, opened);
        column.addView(view);
        scroll.addView(column);
        AlertDialog.Builder dialog = new AlertDialog.Builder(this).setTitle("Журнал игры (конец)").setView(scroll)
            .setPositiveButton("Скопировать", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    android.content.ClipboardManager clipboard = (android.content.ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
                    clipboard.setPrimaryClip(ClipData.newPlainText("журнал игры", shown));
                    Toast.makeText(MainActivity.this, "Скопировано — пришли мне", Toast.LENGTH_SHORT).show();
                }
            })
            .setNegativeButton("Закрыть", null);
        if (log != null && log.isFile()) {
            dialog.setNeutralButton("Поделиться файлом", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    try {
                        File copy = new File(PackProvider.exportDir(MainActivity.this), "latestlog.txt");
                        copy(log, new java.io.FileOutputStream(copy));
                        Uri uri = PackProvider.uriFor(copy);
                        Intent send = new Intent(Intent.ACTION_SEND);
                        send.setType("text/plain");
                        send.putExtra(Intent.EXTRA_STREAM, uri);
                        send.setClipData(ClipData.newRawUri("latestlog.txt", uri));
                        send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                        startActivity(Intent.createChooser(send, "Отправить журнал игры"));
                    } catch (Exception e) {
                        Toast.makeText(MainActivity.this, "Не удалось: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    }
                }
            });
        }
        opened[0] = dialog.show();
    }

    /**
     * Like Millida Launcher's crash fix: one button for what the crash points at. The mod the crash report names (by its
     * file or as "Suspected Mod") is switched off in the installation; "out of memory" gives the game 1 GB more.
     */
    private void addCrashFixes(LinearLayout column, String text, final AlertDialog[] dialog) {
        String id = getSharedPreferences("lastgame", MODE_PRIVATE).getString("id", null);
        final Installation inst = id == null ? null : installations.get(id);
        if (inst == null || !text.contains("Отчёт о сбое") && !text.contains("Ошибки")) {
            return;
        }
        String lower = text.toLowerCase(Locale.ROOT);
        List<View> fixes = new ArrayList<View>();
        if (lower.contains("outofmemoryerror") || lower.contains("java heap space")) {
            Button more = Ui.button(this, "Дать игре больше памяти (+1 ГБ)", true);
            more.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    int now = GameEngine.memoryFor(MainActivity.this, inst);
                    int next = Math.min(now + 1024, GameEngine.phoneMemory(MainActivity.this) * 75 / 100) / 256 * 256;
                    GameEngine.chooseMemory(MainActivity.this, next);
                    Toast.makeText(MainActivity.this, "Теперь игре " + next + " МБ памяти. Запусти ещё раз.", Toast.LENGTH_LONG).show();
                    if (dialog[0] != null) {
                        dialog[0].dismiss();
                    }
                }
            });
            fixes.add(more);
        }
        for (final Mod mod : inst.mods) {
            if (!mod.enabled || !Mod.MOD.equals(mod.kind)) {
                continue;
            }
            String file = new File(mod.targetPath()).getName().toLowerCase(Locale.ROOT);
            boolean named = file.length() > 4 && lower.contains(file);
            for (String line : text.split("\n")) {
                String l = line.toLowerCase(Locale.ROOT);
                if (l.contains("suspected mod") && !mod.title.isEmpty() && l.contains(mod.title.toLowerCase(Locale.ROOT))) {
                    named = true;
                }
            }
            if (!named) {
                continue;
            }
            Button off = Ui.button(this, "Отключить «" + mod.title + "» и попробовать снова", true);
            off.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    mod.enabled = false;
                    saveInstallation(inst);
                    Toast.makeText(MainActivity.this, "«" + mod.title + "» отключён в «" + inst.name + "». Его можно включить в «Моды и ресурсы».",
                                   Toast.LENGTH_LONG).show();
                    if (dialog[0] != null) {
                        dialog[0].dismiss();
                    }
                    if (home != null) {
                        home.refresh();
                    }
                }
            });
            fixes.add(off);
        }
        if (fixes.isEmpty()) {
            return;
        }
        TextView title = Ui.label(this, "Как починить", 14, 0xFFFF8090, true);
        title.setPadding(Ui.dp(this, 14), Ui.dp(this, 10), Ui.dp(this, 14), Ui.dp(this, 4));
        column.addView(title);
        for (View fix : fixes) {
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            p.setMargins(Ui.dp(this, 14), Ui.dp(this, 2), Ui.dp(this, 14), Ui.dp(this, 4));
            column.addView(fix, p);
        }
    }

    private Installation javaWaiting;  // played again once Java is there

    /** Java for Android is not inside Lullaby (it is big): taken from PojavLauncher on the phone or from a file */
    void showJavaDialog(final int major, final Installation inst) {
        final GameEngine engine = GameEngine.get();
        javaWaiting = inst;
        final List<String> sources = engine.javaSources(this);
        String installed = engine.installedJava();
        StringBuilder text = new StringBuilder();
        text.append(major > 0 ? "Для Minecraft " + (inst != null ? inst.minecraftVersion : "") + " нужна Java " + major + " для Android. "
                              : "Java для Android, на которой работает игра. ");
        text.append(installed.isEmpty() ? "Сейчас Java не установлена." : "Сейчас есть: " + installed + ".");
        text.append(major > 0 ? "\n\nОбычно Vellora скачивает Java сам при «Играть», но сейчас не получилось (нет интернета?). "
                              : "\n\nVellora сам скачивает нужную Java при «Играть» (8, 17, 21 или 25 — по версии Minecraft). ");
        text.append("Её можно также взять из PojavLauncher (или Amethyst), если он стоит на телефоне, или выбрать файл: "
                    + "APK PojavLauncher или архив Java (.tar.xz).");
        AlertDialog.Builder dialog = new AlertDialog.Builder(this).setTitle("Java для игры").setMessage(text.toString());
        if (!sources.isEmpty()) {
            dialog.setPositiveButton("Взять из " + sources.get(0), new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    importJava(sources.get(0), null);
                }
            });
        }
        dialog.setNeutralButton("Выбрать файл", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface d, int which) {
                Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                pick.addCategory(Intent.CATEGORY_OPENABLE);
                pick.setType("*/*");
                try {
                    startActivityForResult(pick, GameEngine.REQUEST_JAVA_FILE);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(MainActivity.this, "Нет приложения для выбора файлов", Toast.LENGTH_LONG).show();
                }
            }
        });
        dialog.setNegativeButton("Отмена", null).show();
    }

    private void importJava(final String appName, final Intent picked) {
        final GameEngine engine = GameEngine.get();
        final HomeScreen.Progress progress = new HomeScreen.Progress(this, "Устанавливаю Java");
        progress.log(appName != null ? "Беру Java из " + appName + "..." : "Распаковываю Java из файла...");
        background(new Job<String>() {
            public String run() throws Exception {
                return appName != null ? engine.importJavaFromApp(MainActivity.this, appName) : engine.importJavaFromFile(MainActivity.this, picked);
            }

            public void done(String result, Exception error) {
                progress.dismiss();
                if (error != null) {
                    new AlertDialog.Builder(MainActivity.this).setTitle("Java не установилась").setMessage(error.getMessage())
                        .setPositiveButton("OK", null).show();
                    return;
                }
                Toast.makeText(MainActivity.this, "Установлено: " + result, Toast.LENGTH_LONG).show();
                if (javaWaiting != null) {
                    Installation again = javaWaiting;
                    javaWaiting = null;
                    playInstallation(again);
                }
            }
        });
    }

    private static final int REQUEST_IMPORT_PACK = 42;

    /** "Из файла" in the modpack browser: a .mrpack or a CurseForge .zip from the phone */
    void pickPackFile() {
        Intent pick = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        pick.addCategory(Intent.CATEGORY_OPENABLE);
        pick.setType("*/*");
        try {
            startActivityForResult(pick, REQUEST_IMPORT_PACK);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, "Нет приложения для выбора файлов", Toast.LENGTH_LONG).show();
        }
    }

    private void importPackFile(final Uri uri) {
        final HomeScreen.Progress progress = new HomeScreen.Progress(this, "Импорт сборки");
        progress.log("Читаю файл...");
        background(new Job<Installation>() {
            public Installation run() throws Exception {
                File copy = new File(getCacheDir(), "pack-import.zip");
                java.io.InputStream in = getContentResolver().openInputStream(uri);
                java.io.FileOutputStream out = new java.io.FileOutputStream(copy);
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
                try {
                    Installation inst = Installer.importFile(copy, installations, progress);
                    if (inst.name.isEmpty()) {
                        inst.name = "Импортированная сборка";
                    }
                    installations.save(inst);
                    return inst;
                } finally {
                    copy.delete();
                }
            }

            public void done(Installation inst, Exception error) {
                progress.dismiss();
                if (error != null) {
                    new AlertDialog.Builder(MainActivity.this).setTitle("Не удалось импортировать сборку")
                        .setMessage(error instanceof Installer.InstallException ? error.getMessage() : String.valueOf(error.getMessage()))
                        .setPositiveButton("OK", null).show();
                    return;
                }
                Toast.makeText(MainActivity.this, "Сборка «" + inst.name + "» добавлена: " + inst.describe() + BrowserScreen.disabledNote(inst),
                               Toast.LENGTH_LONG).show();
                showHome(inst.id);
            }
        });
    }

    /** "Export" on the main screen: send or save the .mrpack */
    void exportInstallation(final Installation inst) {
        withPackFile(inst, "Экспорт «" + inst.name + "»", new Job<File>() {
            public File run() {
                return null;
            }

            public void done(File file, Exception error) {
                Uri uri = PackProvider.uriFor(file);
                Intent send = new Intent(Intent.ACTION_SEND);
                send.setType("application/x-modrinth-modpack+zip");
                send.putExtra(Intent.EXTRA_STREAM, uri);
                send.putExtra(Intent.EXTRA_SUBJECT, inst.name);
                send.setClipData(ClipData.newRawUri(inst.name, uri));
                send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                startActivity(Intent.createChooser(send, "Отправить или сохранить сборку"));
            }
        });
    }

    private void playFile(File file, String minecraftVersion, String loader) {
        if (android.os.Build.VERSION.SDK_INT < 29 && !hasStoragePermission()) {
            // asks for the permission; afterPermission runs once it is granted
            saveToDownloads(file);
            return;
        }
        final String saved = saveToDownloads(file);
        final List<String[]> launchers = installedLaunchers();

        StringBuilder steps = new StringBuilder();
        steps.append(saved != null ? "1. Сборка сохранена: <b>" + Util.escape(saved) + "</b>"
                                   : "1. Сохрани сборку кнопкой <b>Файл → Сохранить в файлы</b>");
        steps.append("<br><br>2. В лаунчере Minecraft открой <b>импорт сборки</b> (Modpack / .mrpack) и выбери этот файл. "
                     + "Лаунчер сам скачает Minecraft " + Util.escape(minecraftVersion) + ", " + Plan.loaderName(loader)
                     + " и все моды.");
        steps.append("<br><br>3. Войди аккаунтом Microsoft, на котором куплена Minecraft: Java Edition, и нажми <b>Играть</b>.");
        steps.append("<br><br>4. Удобные кнопки для игры (джойстик, ЛКМ, ПКМ, прыжок, инвентарь, камера, клавиатура): "
                     + "<b>Управление → Отправить в лаунчер</b> на главном экране Vellora.");
        if (launchers.isEmpty()) {
            steps.append("<br><br>На телефоне пока нет лаунчера Java Edition. Установи, например, PojavLauncher, Amethyst, "
                         + "Fold Craft Launcher или Zalith Launcher и нажми «Играть» ещё раз.");
        }
        steps.append("<br><br><i>Моды на телефоне работают медленнее, чем на компьютере: для слабого телефона лучше сборка "
                     + "с небольшим числом модов.</i>");

        AlertDialog.Builder dialog = new AlertDialog.Builder(this).setTitle("Как поиграть на телефоне").setMessage(Html.fromHtml(steps.toString()));
        if (launchers.isEmpty()) {
            dialog.setPositiveButton("Найти лаунчер", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    openStoreSearch();
                }
            });
        } else {
            final String[] launcher = launchers.get(0);
            dialog.setPositiveButton("Открыть " + launcher[0], new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface d, int which) {
                    openLauncher(launcher[1]);
                }
            });
            if (launchers.size() > 1) {
                final String[] second = launchers.get(1);
                dialog.setNeutralButton("Открыть " + second[0], new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface d, int which) {
                        openLauncher(second[1]);
                    }
                });
            }
        }
        dialog.setNegativeButton("Закрыть", null).show();
    }

    private void openLauncher(String packageName) {
        Intent launch = getPackageManager().getLaunchIntentForPackage(packageName);
        if (launch == null) {
            openStoreSearch();
            return;
        }
        launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(launch);
    }

    private void openStoreSearch() {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://search?q=Minecraft%20Java%20launcher%20PojavLauncher&c=apps")));
        } catch (ActivityNotFoundException e) {
            startActivity(new Intent(Intent.ACTION_VIEW,
                                     Uri.parse("https://play.google.com/store/search?q=Minecraft%20Java%20launcher%20PojavLauncher&c=apps")));
        }
    }

    private void noLauncher() {
        new AlertDialog.Builder(this)
            .setTitle("Нужен лаунчер Minecraft")
            .setMessage("Чтобы играть со сборкой на телефоне, установи лаунчер Minecraft: Java Edition для Android, который умеет "
                        + "импортировать сборки Modrinth (.mrpack). Потом нажми «Играть» ещё раз или сохрани файл через «Файл» и "
                        + "открой его в лаунчере.")
            .setPositiveButton("OK", null)
            .show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_SAVE_IMAGE) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null && pendingImage != null) {
                try {
                    java.io.OutputStream out = getContentResolver().openOutputStream(data.getData());
                    try {
                        pendingImage.compress(Bitmap.CompressFormat.PNG, 100, out);
                    } finally {
                        out.close();
                    }
                    Toast.makeText(this, "Сохранено", Toast.LENGTH_SHORT).show();
                } catch (Exception e) {
                    Toast.makeText(this, "Не удалось сохранить: " + e.getMessage(), Toast.LENGTH_LONG).show();
                }
            }
            pendingImage = null;
            return;
        }
        if (requestCode == REQUEST_SKIN) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null && wardrobe != null) {
                wardrobe.changeSkin(data.getData());
            }
            return;
        }
        if (requestCode == REQUEST_IMPORT_PACK) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null) {
                importPackFile(data.getData());
            }
            return;
        }
        if (requestCode == GameEngine.REQUEST_JAVA_FILE) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null) {
                importJava(null, data);
            }
            return;
        }
        if (requestCode != REQUEST_SAVE || resultCode != RESULT_OK || data == null || data.getData() == null || pendingExport == null) {
            return;
        }
        try {
            OutputStream out = getContentResolver().openOutputStream(data.getData());
            java.io.FileInputStream in = new java.io.FileInputStream(pendingExport);
            try {
                byte[] buffer = new byte[16384];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                }
            } finally {
                in.close();
                out.close();
            }
            Toast.makeText(this, "Сборка сохранена", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            Toast.makeText(this, "Не удалось сохранить: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}
