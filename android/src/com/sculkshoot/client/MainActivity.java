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

    // sculk colors of the desktop theme
    // night black and moon white; main buttons wear a see-through rainbow
    private static final int BG = 0xFF0A0B12, PANEL = 0xFF14151F, PANEL_LIGHT = 0xFF1F2130, ACCENT = 0xFFFFFFFF, ACCENT_DARK = 0xFF5D6276,
                             TEXT = 0xFFF4F5FB, HINT = 0xFFA3A7BB, USER_BUBBLE = 0xFF2C2F45, LINE = 0xFF3A3E52;
    private static final int[] RAINBOW_PASTEL = { 0xFFFFC4D0, 0xFFFFE0C0, 0xFFFFF4C2, 0xFFCDF3D6, 0xFFC4E4FF, 0xFFDCCFFF };

    private static final String[] TOPIC_REPLIES = { "Зомби-апокалипсис: города, зомби и оружие", "Магия и приключения с подземельями",
                                                    "Технологии и космос", "Только оптимизация для слабого ПК" };
    private static final String[] SETUP_REPLIES = { "NeoForge 1.21.1", "Fabric 1.20.1", "Forge 1.20.1", "Forge 1.12.2" };
    private static final String[] LOADER_REPLIES = { "NeoForge", "Fabric", "Forge", "Quilt" };
    private static final String[] VERSION_REPLIES = { "1.21.1", "1.20.1", "1.19.2", "1.16.5" };
    private static final String[] AFTER_REPLIES = { "Добавь магию", "Добавь шейдеры", "Начать заново" };

    private final Handler main = new Handler(Looper.getMainLooper());
    private ChatStore store;
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
        store = new ChatStore(getFilesDir());
        try {
            pixel = Typeface.createFromAsset(getAssets(), "Tiny5-Regular.ttf");
        } catch (RuntimeException e) {
            pixel = Typeface.DEFAULT_BOLD;
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        getWindow().setStatusBarColor(BG);
        getWindow().setNavigationBarColor(BG);
        setContentView(buildLayout());

        // continue the most recent chat, like a messenger
        List<JSONObject> chats = store.list();
        if (!chats.isEmpty()) {
            loadChat(chats.get(0).optString("id"));
        } else {
            newChat();
        }
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

    private GradientDrawable rounded(int color, float radius, int stroke) {
        GradientDrawable d = new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(radius));
        if (stroke != 0) {
            d.setStroke(dp(1), stroke);
        }
        return d;
    }

    private Button button(String text, boolean accent) {
        Button b = new Button(this);
        b.setText(text);
        b.setAllCaps(false);
        b.setTypeface(pixel);
        b.setTextSize(15);
        b.setTextColor(accent ? BG : TEXT);
        if (accent) {
            GradientDrawable rainbow = new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT, RAINBOW_PASTEL);
            rainbow.setCornerRadius(dp(10));
            rainbow.setStroke(dp(1), Color.WHITE);
            b.setBackground(rainbow);
        } else {
            b.setBackground(rounded(PANEL_LIGHT, 10, LINE));
        }
        b.setPadding(dp(14), dp(6), dp(14), dp(6));
        b.setMinHeight(dp(40));
        b.setMinimumHeight(dp(40));
        return b;
    }

    private View buildLayout() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new MoonSkyDrawable(getResources().getDisplayMetrics().density));

        // top bar: Moony, the name, chats and the invite code
        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(12), dp(8), dp(8), dp(8));
        bar.setBackgroundColor(0xCC0D0E16);
        ImageView warden = new ImageView(this);
        warden.setImageResource(R.drawable.moony);
        bar.addView(warden, new LinearLayout.LayoutParams(dp(40), dp(40)));
        LinearLayout titles = new LinearLayout(this);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.setPadding(dp(10), 0, 0, 0);
        TextView title = new TextView(this);
        title.setText("Лунни-сборщик");
        title.setTypeface(pixel);
        title.setTextSize(18);
        title.setTextColor(ACCENT);
        titles.addView(title);
        TextView subtitle = new TextView(this);
        subtitle.setText("Lullaby Launcher · моды с Modrinth");
        subtitle.setTextSize(11);
        subtitle.setTextColor(HINT);
        titles.addView(subtitle);
        bar.addView(titles, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
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
        root.addView(chatScroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        // the finished pack: name, mod list and export
        packBar = new LinearLayout(this);
        packBar.setOrientation(LinearLayout.VERTICAL);
        packBar.setPadding(dp(12), dp(8), dp(12), dp(8));
        packBar.setBackground(rounded(0xE614151F, 12, LINE));
        packTitle = new TextView(this);
        packTitle.setTextColor(TEXT);
        packTitle.setTextSize(13);
        packBar.addView(packTitle);
        LinearLayout packButtons = new LinearLayout(this);
        packButtons.setPadding(0, dp(6), 0, 0);
        Button list = button("Моды", false);
        list.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showModList();
            }
        });
        Button play = button("В лаунчер", true);
        play.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                exportPack(0);
            }
        });
        Button share = button("Файл", false);
        share.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showExportChoices();
            }
        });
        LinearLayout.LayoutParams third = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1);
        third.rightMargin = dp(6);
        packButtons.addView(list, third);
        packButtons.addView(play, new LinearLayout.LayoutParams(third));
        packButtons.addView(share, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        packBar.addView(packButtons);
        LinearLayout.LayoutParams packParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        packParams.setMargins(dp(8), dp(4), dp(8), dp(4));
        packBar.setVisibility(View.GONE);
        root.addView(packBar, packParams);

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
        input.setHint("Напиши Лунни...");
        input.setHintTextColor(HINT);
        input.setTextColor(TEXT);
        input.setTextSize(15);
        input.setBackground(rounded(0xE614151F, 20, LINE));
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
        int maxWidth = (int) (getResources().getDisplayMetrics().widthPixels * 0.82f);
        if (fromMoony) {
            ImageView avatar = new ImageView(this);
            avatar.setImageResource(R.drawable.moony);
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
        bubble.setBackground(rounded(0xF01F2130, 14, LINE));
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
        bubble.setBackground(rounded(0xF01F2130, 14, LINE));
        bubble.setPadding(dp(12), dp(9), dp(12), dp(9));
        TextView head = new TextView(this);
        head.setText("Лунни работает...");
        head.setTypeface(pixel);
        head.setTextColor(ACCENT);
        bubble.addView(head);
        typingStatus = new TextView(this);
        typingStatus.setText(status);
        typingStatus.setTextColor(HINT);
        typingStatus.setTextSize(13);
        typingStatus.setMaxWidth((int) (getResources().getDisplayMetrics().widthPixels * 0.7f));
        bubble.addView(typingStatus);
        ProgressBar progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        progress.setIndeterminate(true);
        bubble.addView(progress, new LinearLayout.LayoutParams(dp(220), dp(10)));
        typingRow = addRow(bubble, true);
    }

    private void hideTyping() {
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
                        + "Или нажми <b>В лаунчер</b>, чтобы поставить сборку.", AFTER_REPLIES);
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
                           + details + "<br>Нажми <b>В лаунчер</b>, чтобы открыть сборку в лаунчере Minecraft на телефоне, "
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
        say("Привет! Я Лунни, собираю сборки модов. <b>Про что будет твоя сборка?</b> Расскажи тему и что в ней должно быть, например: "
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
            Toast.makeText(this, "Сохранённых чатов пока нет. Напиши Лунни, про что будет сборка.", Toast.LENGTH_LONG).show();
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

    private void showInviteCode() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(20), dp(8), dp(20), 0);
        TextView hint = new TextView(this);
        hint.setText("Вставь код, который прислал друг из Lullaby Launcher на компьютере («Мой сервер»). Я покажу адрес сервера.");
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
            new AlertDialog.Builder(this).setMessage("Это не похоже на код приглашения Lullaby. Проверь, что код скопирован целиком.")
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
        text.setText(workLog.length() == 0 ? "Журнал пуст: Лунни ещё ничего не собирал в этом чате." : workLog.toString());
        text.setTextSize(12);
        text.setTextIsSelectable(true);
        text.setPadding(dp(20), dp(10), dp(20), dp(10));
        ScrollView scroll = new ScrollView(this);
        scroll.addView(text);
        new AlertDialog.Builder(this).setTitle("Журнал работы").setView(scroll).setPositiveButton("OK", null).show();
    }

    private void showAbout() {
        new AlertDialog.Builder(this)
            .setTitle("Lullaby Launcher для Android")
            .setMessage("Лунни собирает сборки модов с Modrinth: понимает тему, названия модов, «убери» и «побольше», "
                        + "сам добавляет оптимизацию и все нужные библиотеки.\n\n"
                        + "Сама Minecraft: Java Edition на телефоне запускается в лаунчере для Android, который умеет ставить сборки "
                        + "Modrinth (.mrpack). Кнопка «В лаунчер» открывает сборку в нём, «Файл» — сохраняет или отправляет .mrpack.\n\n"
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
            safe = "Lullaby pack";
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

    private void noLauncher() {
        new AlertDialog.Builder(this)
            .setTitle("Нужен лаунчер Minecraft")
            .setMessage("Чтобы играть со сборкой на телефоне, установи лаунчер Minecraft: Java Edition для Android, который умеет "
                        + "импортировать сборки Modrinth (.mrpack). Потом нажми «В лаунчер» ещё раз или сохрани файл через «Файл» и "
                        + "открой его в лаунчере.")
            .setPositiveButton("OK", null)
            .show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
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
