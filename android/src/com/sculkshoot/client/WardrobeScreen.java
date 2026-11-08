package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.net.Uri;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

/**
 * The wardrobe, laid out like Millida Launcher's: "‹ Лобби" and "Загрузить скин" on top, the player big on a lit stage
 * on the left (a tap turns it around), square tabs with pictures on the right (outfits, skins, capes, head, body, back,
 * pets, effects, emotes) and the items as tiles. Outfits are cards of four pieces with "2/4", like Millida's sets.
 * Every item is open. The skin and the capes are the real ones of the Minecraft account and are seen in the game; the
 * other items show in the launcher.
 */
final class WardrobeScreen {
    static final String SETS = "sets", SKINS = "skins", CAPES = "capes";
    private static final String[][] TABS = { { SETS, "Образы" }, { SKINS, "Скины" }, { CAPES, "Плащи" }, { Cosmetics.HEAD, "Голова" },
                                             { Cosmetics.BODY, "Тело" }, { Cosmetics.BACK, "Спина" }, { Cosmetics.PET, "Питомцы" },
                                             { Cosmetics.EFFECT, "Эффекты" }, { Cosmetics.EMOTE, "Эмоции" } };

    private final MainActivity activity;
    private final LinearLayout root;
    private final CharacterView character;
    private final FlowLayout grid;
    private final ScrollView scroll;
    private final LinearLayout header;  // the generator above the tiles on the skins and capes tabs
    private long seed = System.currentTimeMillis();
    private final TextView section;
    private final List<View> tabViews = new ArrayList<View>();
    private String tab = SETS;
    private boolean slim;  // the skin to upload: thin arms (Alex) or classic (Steve)

    WardrobeScreen(MainActivity activity) {
        this.activity = activity;
        float density = activity.getResources().getDisplayMetrics().density;
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(0xFF0B0305);

        LinearLayout bar = new LinearLayout(activity);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(10), dp(6), dp(10), dp(6));
        bar.setBackgroundColor(0xF0080204);
        Button back = Ui.button(activity, "‹ Лобби", true);
        back.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                WardrobeScreen.this.activity.showLobby();
            }
        });
        bar.addView(back);
        bar.addView(new View(activity), new LinearLayout.LayoutParams(0, 1, 1));
        Button undress = Ui.button(activity, "Снять всё", false);
        undress.setTextSize(13);
        undress.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                for (String slot : Cosmetics.WEARABLE) {
                    Cosmetics.wear(WardrobeScreen.this.activity, slot, "");
                }
                Sounds.play(Sounds.CLOSE);
                character.invalidate();
                showTab(tab);
            }
        });
        bar.addView(undress);
        Button upload = Ui.button(activity, "⬆ Загрузить скин", true);
        upload.setTextSize(13);
        upload.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showTab(SKINS);
                pickSkin();
            }
        });
        LinearLayout.LayoutParams up = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        up.leftMargin = dp(6);
        bar.addView(upload, up);
        root.addView(bar);

        LinearLayout body = new LinearLayout(activity);
        body.setPadding(dp(10), dp(10), dp(6), dp(10));
        root.addView(body, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        // left: the player on a stage lit from the middle, with rays, like Millida's wardrobe
        FrameLayout stage = new FrameLayout(activity);
        stage.setBackground(new StageDrawable(density));
        character = new CharacterView(activity);
        stage.addView(character, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        TextView hint = Ui.label(activity, "Крути пальцем · нажми — анимация", 10, 0xB3FFD6DC, false);
        hint.setGravity(Gravity.CENTER);
        stage.addView(hint, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM));
        body.addView(stage, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 4));

        // right: the square tabs with pictures and the items
        LinearLayout right = new LinearLayout(activity);
        right.setOrientation(LinearLayout.VERTICAL);
        right.setPadding(dp(10), 0, 0, 0);
        HorizontalScrollView tabsScroll = new HorizontalScrollView(activity);
        tabsScroll.setHorizontalScrollBarEnabled(false);
        LinearLayout tabs = new LinearLayout(activity);
        for (final String[] t : TABS) {
            View b = tabButton(t[0], t[1]);
            b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    showTab(t[0]);
                }
            });
            tabViews.add(b);
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(dp(66), dp(62));
            p.rightMargin = dp(5);
            tabs.addView(b, p);
        }
        tabsScroll.addView(tabs);
        right.addView(tabsScroll);
        section = Ui.label(activity, "", 15, Ui.TEXT, true);
        section.setPadding(dp(2), dp(8), 0, dp(6));
        right.addView(section);
        scroll = new ScrollView(activity);
        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        header = new LinearLayout(activity);
        header.setOrientation(LinearLayout.VERTICAL);
        content.addView(header, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        grid = new FlowLayout(activity, dp(8));
        grid.setPadding(0, 0, 0, dp(10));
        content.addView(grid, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        scroll.addView(content);
        right.addView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        body.addView(right, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 7));

        loadSkinAndCape(activity, character);
        showTab(tab);
    }

    View view() {
        return root;
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    /** a square tab like Millida's: a picture above the name */
    private View tabButton(String id, String name) {
        LinearLayout b = new LinearLayout(activity);
        b.setOrientation(LinearLayout.VERTICAL);
        b.setGravity(Gravity.CENTER_HORIZONTAL);
        b.setPadding(0, dp(5), 0, dp(4));
        View icon = tabIcon(id);
        b.addView(icon, new LinearLayout.LayoutParams(dp(30), dp(28)));
        TextView label = Ui.label(activity, name, 10, Ui.TEXT, true);
        label.setGravity(Gravity.CENTER);
        b.addView(label);
        b.setTag(id);
        b.setClickable(true);
        b.setContentDescription(name);
        return b;
    }

    private View tabIcon(String id) {
        if (Cosmetics.HEAD.equals(id)) {
            return new ItemIcon(activity, Cosmetics.find(activity, "top_hat"));
        }
        if (Cosmetics.BODY.equals(id)) {
            return new ItemIcon(activity, Cosmetics.find(activity, "heart_necklace"));
        }
        if (Cosmetics.BACK.equals(id)) {
            return new ItemIcon(activity, Cosmetics.find(activity, "bat_wings"));
        }
        if (Cosmetics.PET.equals(id)) {
            return new ItemIcon(activity, Cosmetics.find(activity, "cat_pet"));
        }
        if (Cosmetics.EFFECT.equals(id)) {
            return new ItemIcon(activity, Cosmetics.find(activity, "sparkles"));
        }
        if (SKINS.equals(id)) {
            return new PixelIcon(activity, new String[] { "..wwww..", "..wwww..", "..wwww..", "........", "wwwwwwww", "w.wwww.w", "w.wwww.w", "..wwww..",
                                                          "..w..w..", "..w..w.." });
        }
        if (CAPES.equals(id)) {
            return new PixelIcon(activity, new String[] { "wwwwwwww", ".wwwwww.", ".wwwwww.", ".wwwwww.", ".wwwwww.", "wwwwwwww", "wwwwwwww", "ww.ww.ww" });
        }
        if (Cosmetics.EMOTE.equals(id)) {
            return new PixelIcon(activity, new String[] { "..wwwww..", ".w.....w.", "w..w.w..w", "w.......w", "w.w...w.w", "w..www..w", ".w.....w.", "..wwwww.." });
        }
        // the outfits: a hanger
        return new PixelIcon(activity, new String[] { ".....ww.....", "....w..w....", ".......w....", "......w.....", ".....ww.....", "...ww..ww...",
                                                      ".ww......ww.", "wwwwwwwwwwww" });
    }

    /** the account's skin and active cape on a character */
    static void loadSkinAndCape(final MainActivity activity, final CharacterView view) {
        loadSkinAndCape(activity, view, null);
    }

    /**
     * The skin and the cape: at once from the phone (SkinCache), then fresh from Mojang over https and kept again;
     * `onSkin` hears about every skin shown (the account chip draws its face from it).
     */
    static void loadSkinAndCape(final MainActivity activity, final CharacterView view, final Icons.Got onSkin) {
        final Account account = activity.account();
        if (account == null || !account.signedIn()) {
            return;
        }
        view.setSlim("SLIM".equalsIgnoreCase(account.skinVariant));
        Bitmap cached = SkinCache.skin(activity, account);
        if (cached != null) {
            view.setSkin(cached);
            if (onSkin != null) {
                onSkin.got(cached);
            }
        }
        if (!account.skinUrl.isEmpty()) {
            Icons.loadBitmap(account.skinUrl, new Icons.Got() {
                public void got(Bitmap bitmap) {
                    if (bitmap == null || bitmap.getWidth() < 64) {
                        return;  // no internet: the kept skin stays
                    }
                    view.setSkin(bitmap);
                    SkinCache.saveSkin(activity, account, bitmap);
                    if (onSkin != null) {
                        onSkin.got(bitmap);
                    }
                }
            });
        }
        Bitmap generated = SkinCache.generatedCape(activity);
        if (generated != null) {
            view.setCape(generated);  // a cape from the generator, chosen in the wardrobe
            return;
        }
        Bitmap cachedCape = SkinCache.cape(activity, account);
        if (cachedCape != null) {
            view.setCape(cachedCape);
        }
        if (!account.ownsJava) {
            return;
        }
        activity.background(new MainActivity.Job<String>() {
            public String run() throws Exception {
                for (Account.Cape cape : account.capes()) {
                    if (cape.active) {
                        return cape.url;
                    }
                }
                return "";
            }

            public void done(String url, Exception error) {
                if (error != null || url == null) {
                    return;  // no internet: the kept cape stays
                }
                if (url.isEmpty()) {
                    view.setCape(null);
                    SkinCache.saveCape(activity, account, null);
                    return;
                }
                Icons.loadBitmap(url, new Icons.Got() {
                    public void got(Bitmap bitmap) {
                        if (bitmap != null) {
                            view.setCape(bitmap);
                            SkinCache.saveCape(activity, account, bitmap);
                        }
                    }
                });
            }
        });
    }

    private static int indexOf(String id) {
        for (int i = 0; i < TABS.length; i++) {
            if (TABS[i][0].equals(id)) {
                return i;
            }
        }
        return 0;
    }

    /** the player pops a little when something is put on, with a sparkle sound */
    private void pop(boolean on) {
        Sounds.play(on ? Sounds.WEAR : Sounds.CLICK);
        if (!on) {
            return;
        }
        character.animate().cancel();
        character.setScaleX(0.94f);
        character.setScaleY(0.94f);
        character.animate().scaleX(1f).scaleY(1f).setDuration(260).setInterpolator(new android.view.animation.OvershootInterpolator(3f)).start();
    }

    void showTab(String id) {
        boolean changed = !id.equals(tab);
        final int direction = indexOf(id) >= indexOf(tab) ? 1 : -1;
        tab = id;
        float density = activity.getResources().getDisplayMetrics().density;
        for (View b : tabViews) {
            boolean on = id.equals(b.getTag());
            b.setBackground(on ? PixelDrawable.accent(density) : PixelDrawable.plain(density));
        }
        grid.removeAllViews();
        header.removeAllViews();
        character.setBack(Cosmetics.BACK.equals(id) || CAPES.equals(id));
        if (SETS.equals(id)) {
            section.setText("Наборы — все открыты");
            for (final Cosmetics.Outfit outfit : Cosmetics.outfits(activity)) {
                addOutfit(outfit);
            }
        } else if (SKINS.equals(id)) {
            section.setText("Скин аккаунта — его видно в игре всем");
            showSkins();
        } else if (CAPES.equals(id)) {
            section.setText("Плащи Minecraft — их видно в игре");
            showCapes();
        } else {
            final String slot = id;
            List<Cosmetics.Item> list = Cosmetics.inSlot(activity, slot);
            section.setText(id.equals(Cosmetics.EMOTE) ? "Эмоции — нажми, чтобы персонаж сыграл" : titleOf(id) + " · " + list.size());
            for (final Cosmetics.Item item : list) {
                boolean on = item.id.equals(Cosmetics.worn(activity, slot));
                View picture;
                if (slot.equals(Cosmetics.EMOTE)) {
                    ItemPicture figure = new ItemPicture(activity, "", "", "", "");
                    figure.emote = item.id;
                    picture = figure;
                } else {
                    picture = new ItemIcon(activity, item);
                }
                addTile(picture, item.title, item.rarity, on && !slot.equals(Cosmetics.EMOTE), new Runnable() {
                    public void run() {
                        if (slot.equals(Cosmetics.EMOTE)) {
                            Sounds.play(Sounds.TOGGLE);
                            character.setBack(false);
                            character.playEmote(item.id);
                            return;
                        }
                        boolean wearing = item.id.equals(Cosmetics.worn(activity, slot));
                        Cosmetics.wear(activity, slot, wearing ? "" : item.id);
                        pop(!wearing);
                        character.preview(null, null);
                        character.invalidate();
                        showTab(slot);
                    }
                });
            }
        }
        if (changed) {
            // like Millida's tabs: the content slides in from the side of the new tab, with a blip
            Sounds.play(Sounds.NAV);
            scroll.scrollTo(0, 0);
            scroll.animate().cancel();
            scroll.setAlpha(0f);
            scroll.setTranslationX(dp(26) * direction);
            scroll.animate().alpha(1f).translationX(0).setDuration(240).setInterpolator(new android.view.animation.DecelerateInterpolator(1.6f)).start();
            section.setAlpha(0f);
            section.animate().alpha(1f).setDuration(200).start();
            // the tiles come in one after another, like Millida's
            for (int i = 0; i < grid.getChildCount(); i++) {
                View tile = grid.getChildAt(i);
                tile.setAlpha(0f);
                tile.setTranslationY(dp(10));
                tile.animate().alpha(1f).translationY(0).setStartDelay(i * 18L).setDuration(180).start();
            }
        }
    }

    private static String titleOf(String slot) {
        for (String[] t : TABS) {
            if (t[0].equals(slot)) {
                return t[1];
            }
        }
        return slot;
    }

    /** the cell colour of a rarity, darkened, like the tinted cells of Millida's sets */
    private static int cellColor(String rarity) {
        int c = Cosmetics.rarityColor(rarity);
        int r = ((c >> 16) & 0xFF) / 5, g = ((c >> 8) & 0xFF) / 5, b = (c & 0xFF) / 5;
        return 0xFF000000 | ((r + 0x16) << 16) | ((g + 0x06) << 8) | (b + 0x0A);
    }

    /** an outfit as a card of its pieces (two by two), the name and how many are worn, like Millida's sets */
    private void addOutfit(final Cosmetics.Outfit outfit) {
        float density = activity.getResources().getDisplayMetrics().density;
        String[][] pieces = { { Cosmetics.HEAD, outfit.head }, { Cosmetics.BACK, outfit.back }, { Cosmetics.BODY, outfit.body },
                              { Cosmetics.PET, outfit.pet }, { Cosmetics.EFFECT, outfit.effect } };
        List<Cosmetics.Item> items = new ArrayList<Cosmetics.Item>();
        int worn = 0;
        for (String[] piece : pieces) {
            Cosmetics.Item item = Cosmetics.find(activity, piece[1]);
            if (item != null) {
                items.add(item);
                worn += piece[1].equals(Cosmetics.worn(activity, piece[0])) ? 1 : 0;
            }
        }
        boolean all = worn == items.size() && !items.isEmpty();
        LinearLayout card = new LinearLayout(activity);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER_HORIZONTAL);
        card.setPadding(dp(6), dp(6), dp(6), dp(8));
        card.setBackground(PixelDrawable.panel(all ? 0xFF4A1420 : 0xFF1C080D, all ? 0xFFFF5A6E : 0xFF3A121B, density));
        LinearLayout rows = new LinearLayout(activity);
        rows.setOrientation(LinearLayout.VERTICAL);
        for (int row = 0; row < 2; row++) {
            LinearLayout line = new LinearLayout(activity);
            for (int col = 0; col < 2; col++) {
                int i = row * 2 + col;
                View cell = i < items.size() ? new ItemIcon(activity, items.get(i)) : new View(activity);
                cell.setBackground(PixelDrawable.panel(i < items.size() ? cellColor(items.get(i).rarity) : 0xFF14060A, 0xFF0B0305, density));
                cell.setPadding(dp(4), dp(4), dp(4), dp(4));
                LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(dp(56), dp(50));
                p.setMargins(dp(2), dp(2), dp(2), dp(2));
                line.addView(cell, p);
            }
            rows.addView(line);
        }
        card.addView(rows);
        TextView name = Ui.label(activity, outfit.title, 11, Ui.TEXT, true);
        name.setGravity(Gravity.CENTER);
        name.setMaxWidth(dp(120));
        name.setPadding(0, dp(4), 0, 0);
        card.addView(name);
        TextView count = Ui.label(activity, (all ? "Весь " : "") + worn + "/" + items.size(), 10, all ? 0xFFFF8090 : Ui.HINT, false);
        count.setGravity(Gravity.CENTER);
        card.addView(count);
        card.setClickable(true);
        card.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Cosmetics.wearOutfit(activity, outfit);
                character.setBack(false);
                character.invalidate();
                pop(true);
                showTab(SETS);
            }
        });
        grid.addView(card, new ViewGroup.LayoutParams(dp(132), ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    private void addTile(View picture, String name, String rarity, boolean on, final Runnable action) {
        float density = activity.getResources().getDisplayMetrics().density;
        LinearLayout tile = new LinearLayout(activity);
        tile.setOrientation(LinearLayout.VERTICAL);
        tile.setGravity(Gravity.CENTER_HORIZONTAL);
        tile.setPadding(dp(4), dp(4), dp(4), dp(7));
        // the frame shows the rarity, like Millida's; a worn item glows crimson
        tile.setBackground(PixelDrawable.panel(on ? 0xFF4A1420 : 0xFF1C080D, on ? 0xFFFF5A6E : Cosmetics.rarityColor(rarity), density));
        picture.setBackground(PixelDrawable.panel(cellColor(rarity), 0xFF0B0305, density));
        picture.setPadding(dp(6), dp(6), dp(6), dp(6));
        tile.addView(picture, new LinearLayout.LayoutParams(dp(92), dp(74)));
        TextView label = Ui.label(activity, name, 11, Ui.TEXT, true);
        label.setGravity(Gravity.CENTER);
        label.setMaxWidth(dp(96));
        label.setPadding(0, dp(3), 0, 0);
        tile.addView(label);
        tile.addView(Ui.label(activity, on ? "надето" : Cosmetics.rarityName(rarity), 9, on ? 0xFFFF8090 : Cosmetics.rarityColor(rarity), false));
        tile.setClickable(true);
        tile.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                action.run();
            }
        });
        grid.addView(tile, new ViewGroup.LayoutParams(dp(102), ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    /* ------------------------------------------------------------ the skin of the account */

    private void showSkins() {
        header.addView(generator(false), fullWidth());
        final Account account = activity.account();
        if (account == null || !account.ownsJava) {
            section.setText("Генератор работает всегда; надеть скин можно на аккаунте с Minecraft: Java Edition");
            return;
        }
        float density = activity.getResources().getDisplayMetrics().density;
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(12), dp(10), dp(12), dp(12));
        panel.setBackground(PixelDrawable.panel(0xFF1C080D, 0xFF5A1E2A, density));
        panel.addView(Ui.label(activity, "Свой скин", 15, Ui.TEXT, true));
        TextView text = Ui.label(activity, "PNG 64×64 из файла: его сразу видно в лобби, а в игре — всем игрокам. Выбери, какие руки у скина.", 12, Ui.HINT,
                                 false);
        text.setPadding(0, dp(2), 0, dp(8));
        panel.addView(text);
        LinearLayout arms = new LinearLayout(activity);
        final Button classic = Ui.button(activity, "Широкие руки (Steve)", !slim);
        final Button thin = Ui.button(activity, "Тонкие руки (Alex)", slim);
        classic.setTextSize(12);
        thin.setTextSize(12);
        classic.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                slim = false;
                Ui.styleButton(activity, classic, true);
                Ui.styleButton(activity, thin, false);
            }
        });
        thin.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                slim = true;
                Ui.styleButton(activity, classic, false);
                Ui.styleButton(activity, thin, true);
            }
        });
        arms.addView(classic, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        LinearLayout.LayoutParams tp = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1);
        tp.leftMargin = dp(6);
        arms.addView(thin, tp);
        panel.addView(arms);
        Button pick = Ui.button(activity, "⬆ Выбрать PNG и загрузить", true);
        pick.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                pickSkin();
            }
        });
        LinearLayout.LayoutParams pp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        pp.topMargin = dp(8);
        panel.addView(pick, pp);
        Button reset = Ui.button(activity, "Вернуть стандартный скин", false);
        reset.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                changeSkin(null);
            }
        });
        panel.addView(reset, new LinearLayout.LayoutParams(pp));
        Button nick = Ui.button(activity, "Аккаунт и ник", false);
        nick.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.showAccount();
            }
        });
        panel.addView(nick, new LinearLayout.LayoutParams(pp));
        grid.addView(panel, new ViewGroup.LayoutParams(dp(330), ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    private void pickSkin() {
        activity.pickSkinFile(this);
    }

    /** the PNG the player chose (null: back to the standard skin) */
    void changeSkin(final Uri uri) {
        final Account account = activity.account();
        if (account == null || !account.ownsJava) {
            Toast.makeText(activity, "Нужен аккаунт с Minecraft: Java Edition", Toast.LENGTH_LONG).show();
            return;
        }
        final boolean thin = slim;
        Toast.makeText(activity, uri == null ? "Возвращаю стандартный скин..." : "Загружаю скин...", Toast.LENGTH_SHORT).show();
        activity.background(new MainActivity.Job<Boolean>() {
            public Boolean run() throws Exception {
                if (uri == null) {
                    account.resetSkin();
                    return true;
                }
                java.io.InputStream in = activity.getContentResolver().openInputStream(uri);
                byte[] png;
                try {
                    java.io.ByteArrayOutputStream out = new java.io.ByteArrayOutputStream();
                    byte[] buffer = new byte[16384];
                    int n;
                    while ((n = in.read(buffer)) > 0 && out.size() < 1 << 20) {
                        out.write(buffer, 0, n);
                    }
                    png = out.toByteArray();
                } finally {
                    in.close();
                }
                Bitmap check = android.graphics.BitmapFactory.decodeByteArray(png, 0, png.length);
                if (check == null || check.getWidth() != 64 || (check.getHeight() != 64 && check.getHeight() != 32)) {
                    throw new Account.LoginException("Это не скин Minecraft: нужна картинка PNG 64×64 (или 64×32)");
                }
                account.uploadSkin(png, thin);
                return true;
            }

            public void done(Boolean ok, Exception error) {
                if (error != null) {
                    Toast.makeText(activity, error.getMessage(), Toast.LENGTH_LONG).show();
                    return;
                }
                account.save(activity);
                Toast.makeText(activity, uri == null ? "Стандартный скин возвращён" : "Скин загружен — его видно в игре", Toast.LENGTH_LONG).show();
                loadSkinAndCape(activity, character);
                activity.accountChanged();
            }
        });
    }


    /* ------------------------------------------------------------ Arkan's skin and cape generator */

    private LinearLayout.LayoutParams fullWidth() {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.bottomMargin = dp(10);
        return p;
    }

    private static final String[] SKIN_IDEAS = { "🛡 Рыцарь в золотых доспехах", "🥷 Ниндзя с красной повязкой", "🧙 Маг в фиолетовой мантии", "🏴 Пират",
                                                 "👑 Принцесса", "🚀 Космонавт", "😈 Демон", "🤖 Кибер неон", "🧟 Зомби", "💀 Скелет", "🧝 Эльф",
                                                 "🪓 Викинг", "🖤 Чёрное худи с черепом", "🍷 Бордовый костюм" };
    private static final String[] CAPE_IDEAS = { "🌌 Галактика со звёздами", "🔥 Огненный", "❄ Ледяной", "👑 Золотой с короной", "💗 Розовый с сердцем",
                                                 "🌈 Радуга", "🍷 Бордовый Vellora", "🌲 Лесной", "⚡ Синий с молнией", "🖤 Чёрно-красные полосы" };

    /** the generator: a description, ideas, four variants; a tap tries one on, then it can be worn or saved */
    private View generator(final boolean capes) {
        float density = activity.getResources().getDisplayMetrics().density;
        final LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(12), dp(10), dp(12), dp(12));
        panel.setBackground(PixelDrawable.panel(0xFF241018, 0xFFB3263A, density));
        LinearLayout titleRow = new LinearLayout(activity);
        titleRow.setGravity(Gravity.CENTER_VERTICAL);
        WizardView wizard = new WizardView(activity);
        titleRow.addView(wizard, new LinearLayout.LayoutParams(dp(40), dp(40)));
        LinearLayout titles = new LinearLayout(activity);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.setPadding(dp(8), 0, 0, 0);
        titles.addView(Ui.label(activity, capes ? "ИИ-генератор плащей" : "ИИ-генератор скинов", 15, Ui.TEXT, true));
        titles.addView(Ui.label(activity, capes ? "Опиши плащ — Аркан нарисует 4 варианта" : "Опиши персонажа — Аркан нарисует 4 скина", 11, Ui.HINT, false));
        titleRow.addView(titles, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        panel.addView(titleRow);

        LinearLayout inputRow = new LinearLayout(activity);
        inputRow.setGravity(Gravity.CENTER_VERTICAL);
        inputRow.setPadding(0, dp(8), 0, dp(4));
        final android.widget.EditText prompt = new android.widget.EditText(activity);
        prompt.setHint(capes ? "Например: фиолетовая галактика со звёздами" : "Например: рыжая девушка-маг в синей мантии");
        prompt.setHintTextColor(Ui.HINT);
        prompt.setTextColor(Ui.TEXT);
        prompt.setTextSize(13);
        prompt.setSingleLine(true);
        prompt.setBackground(Ui.rounded(activity, 0xFF0E0407, 0, 0xFF5A1E2A));
        prompt.setPadding(dp(10), dp(7), dp(10), dp(7));
        inputRow.addView(prompt, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button go = Ui.button(activity, "🔮 Создать", true);
        go.setTextSize(13);
        LinearLayout.LayoutParams gp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        gp.leftMargin = dp(6);
        inputRow.addView(go, gp);
        panel.addView(inputRow);

        FlowLayout ideas = new FlowLayout(activity, dp(5));
        for (final String idea : capes ? CAPE_IDEAS : SKIN_IDEAS) {
            Button b = Ui.button(activity, idea, false);
            b.setTextSize(11);
            b.setMinHeight(dp(30));
            b.setMinimumHeight(dp(30));
            b.setPadding(dp(8), dp(3), dp(8), dp(6));
            b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    prompt.setText(idea.substring(idea.indexOf(' ') + 1));
                    Sounds.play(Sounds.CLICK);
                }
            });
            ideas.addView(b);
        }
        panel.addView(ideas);

        final FlowLayout variants = new FlowLayout(activity, dp(8));
        variants.setPadding(0, dp(10), 0, 0);
        panel.addView(variants);
        final LinearLayout actions = new LinearLayout(activity);
        actions.setPadding(0, dp(8), 0, 0);
        actions.setVisibility(View.GONE);
        panel.addView(actions);

        go.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                final String text = prompt.getText().toString().trim();
                seed++;
                Sounds.play(Sounds.OPEN);
                android.view.inputmethod.InputMethodManager keyboard =
                    (android.view.inputmethod.InputMethodManager) activity.getSystemService(android.content.Context.INPUT_METHOD_SERVICE);
                if (keyboard != null) {
                    keyboard.hideSoftInputFromWindow(prompt.getWindowToken(), 0);
                }
                final long mySeed = seed;
                activity.background(new MainActivity.Job<Object>() {
                    public Object run() {
                        return capes ? SkinGenerator.capes(text, 4, mySeed) : SkinGenerator.skins(text, 4, mySeed);
                    }

                    @SuppressWarnings("unchecked")
                    public void done(Object result, Exception error) {
                        if (error != null || result == null) {
                            Toast.makeText(activity, "Не получилось: " + (error == null ? "" : error.getMessage()), Toast.LENGTH_LONG).show();
                            return;
                        }
                        variants.removeAllViews();
                        actions.setVisibility(View.GONE);
                        if (capes) {
                            for (Bitmap cape : (List<Bitmap>) result) {
                                variants.addView(capeVariant(cape, actions));
                            }
                        } else {
                            for (SkinGenerator.Skin skin : (List<SkinGenerator.Skin>) result) {
                                variants.addView(skinVariant(skin, actions));
                            }
                        }
                        for (int i = 0; i < variants.getChildCount(); i++) {
                            View tile = variants.getChildAt(i);
                            tile.setAlpha(0f);
                            tile.setScaleX(0.85f);
                            tile.setScaleY(0.85f);
                            tile.animate().alpha(1f).scaleX(1f).scaleY(1f).setStartDelay(i * 70L).setDuration(220).start();
                        }
                    }
                });
            }
        });
        return panel;
    }

    private View skinVariant(final SkinGenerator.Skin skin, final LinearLayout actions) {
        float density = activity.getResources().getDisplayMetrics().density;
        LinearLayout tile = new LinearLayout(activity);
        tile.setOrientation(LinearLayout.VERTICAL);
        tile.setGravity(Gravity.CENTER_HORIZONTAL);
        tile.setPadding(dp(4), dp(4), dp(4), dp(6));
        tile.setBackground(PixelDrawable.panel(0xFF1C080D, 0xFF8A3442, density));
        CharacterView picture = new CharacterView(activity);
        picture.setTurnable(false);
        picture.setAnimated(false);
        picture.setShowItems(false);
        picture.setSlim(skin.slim);
        picture.setSkin(skin.bitmap);
        picture.turnTo(28);
        tile.addView(picture, new LinearLayout.LayoutParams(dp(92), dp(124)));
        TextView name = Ui.label(activity, skin.title, 10, Ui.TEXT, true);
        name.setGravity(Gravity.CENTER);
        tile.addView(name);
        tile.setClickable(true);
        tile.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // try it on the big player
                character.setSlim(skin.slim);
                character.setSkin(skin.bitmap);
                character.setBack(false);
                pop(true);
                showSkinActions(actions, skin);
            }
        });
        tile.setLayoutParams(new ViewGroup.LayoutParams(dp(104), ViewGroup.LayoutParams.WRAP_CONTENT));
        return tile;
    }

    private void showSkinActions(LinearLayout actions, final SkinGenerator.Skin skin) {
        actions.removeAllViews();
        actions.setVisibility(View.VISIBLE);
        Button wear = Ui.button(activity, "⬆ Надеть на аккаунт", true);
        wear.setTextSize(12);
        wear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                uploadGenerated(skin);
            }
        });
        actions.addView(wear, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button save = Ui.button(activity, "💾 PNG", false);
        save.setTextSize(12);
        save.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.saveImage(skin.bitmap, "vellora-skin.png");
            }
        });
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        sp.leftMargin = dp(6);
        actions.addView(save, sp);
        Button mine = Ui.button(activity, "↺ Мой", false);
        mine.setTextSize(12);
        mine.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                loadSkinAndCape(activity, character);
                Sounds.play(Sounds.CLOSE);
            }
        });
        actions.addView(mine, new LinearLayout.LayoutParams(sp));
    }

    /** puts a generated skin on the Minecraft account: everyone sees it in the game */
    private void uploadGenerated(final SkinGenerator.Skin skin) {
        final Account account = activity.account();
        if (account == null || !account.ownsJava) {
            Toast.makeText(activity, "Надеть можно на аккаунт с Minecraft: Java Edition. Сохрани PNG — его можно поставить позже.", Toast.LENGTH_LONG).show();
            return;
        }
        Toast.makeText(activity, "Надеваю скин...", Toast.LENGTH_SHORT).show();
        activity.background(new MainActivity.Job<Boolean>() {
            public Boolean run() throws Exception {
                java.io.ByteArrayOutputStream png = new java.io.ByteArrayOutputStream();
                skin.bitmap.compress(Bitmap.CompressFormat.PNG, 100, png);
                account.uploadSkin(png.toByteArray(), skin.slim);
                return true;
            }

            public void done(Boolean ok, Exception error) {
                if (error != null) {
                    Toast.makeText(activity, error.getMessage(), Toast.LENGTH_LONG).show();
                    return;
                }
                account.save(activity);
                SkinCache.saveSkin(activity, account, skin.bitmap);
                Sounds.play(Sounds.WEAR);
                Toast.makeText(activity, "Скин надет — его видно в игре и в лобби", Toast.LENGTH_LONG).show();
                activity.accountChanged();
            }
        });
    }

    private View capeVariant(final Bitmap cape, final LinearLayout actions) {
        float density = activity.getResources().getDisplayMetrics().density;
        LinearLayout tile = new LinearLayout(activity);
        tile.setOrientation(LinearLayout.VERTICAL);
        tile.setGravity(Gravity.CENTER_HORIZONTAL);
        tile.setPadding(dp(4), dp(6), dp(4), dp(6));
        tile.setBackground(PixelDrawable.panel(0xFF1C080D, 0xFF8A3442, density));
        tile.addView(new CapePicture(activity, cape), new LinearLayout.LayoutParams(dp(70), dp(100)));
        tile.setClickable(true);
        tile.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                character.setCape(cape);
                character.setBack(true);
                pop(true);
                showCapeActions(actions, cape);
            }
        });
        tile.setLayoutParams(new ViewGroup.LayoutParams(dp(82), ViewGroup.LayoutParams.WRAP_CONTENT));
        return tile;
    }

    private void showCapeActions(LinearLayout actions, final Bitmap cape) {
        actions.removeAllViews();
        actions.setVisibility(View.VISIBLE);
        Button wear = Ui.button(activity, "Носить в Vellora", true);
        wear.setTextSize(12);
        wear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                SkinCache.useGeneratedCape(activity, cape);
                Sounds.play(Sounds.WEAR);
                Toast.makeText(activity, "Плащ надет в Vellora: он виден в лобби и гардеробе. В самой игре Minecraft показывает только официальные плащи.",
                               Toast.LENGTH_LONG).show();
                activity.accountChanged();
            }
        });
        actions.addView(wear, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button save = Ui.button(activity, "💾 PNG", false);
        save.setTextSize(12);
        save.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                activity.saveImage(cape, "vellora-cape.png");
            }
        });
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        sp.leftMargin = dp(6);
        actions.addView(save, sp);
        Button off = Ui.button(activity, "Снять", false);
        off.setTextSize(12);
        off.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                SkinCache.useGeneratedCape(activity, null);
                character.setCape(null);
                loadSkinAndCape(activity, character);
                activity.accountChanged();
                Sounds.play(Sounds.CLOSE);
            }
        });
        actions.addView(off, new LinearLayout.LayoutParams(sp));
    }

    /* ------------------------------------------------------------ capes */

    private void showCapes() {
        header.addView(generator(true), fullWidth());
        final Account account = activity.account();
        if (account == null || !account.ownsJava) {
            section.setText("Плащи есть только у аккаунта с Minecraft: Java Edition");
            return;
        }
        activity.background(new MainActivity.Job<List<Account.Cape>>() {
            public List<Account.Cape> run() throws Exception {
                return account.capes();
            }

            public void done(List<Account.Cape> capes, Exception error) {
                if (!CAPES.equals(tab)) {
                    return;
                }
                if (error != null) {
                    section.setText("Не удалось загрузить плащи: " + error.getMessage());
                    return;
                }
                if (capes.isEmpty()) {
                    section.setText("У этого аккаунта пока нет плащей (их дают за события Minecraft и покупки)");
                    return;
                }
                boolean anyActive = false;
                for (Account.Cape cape : capes) {
                    anyActive |= cape.active;
                }
                addTile(new CapePicture(activity, null), "Без плаща", "common", !anyActive, new Runnable() {
                    public void run() {
                        setCape(null, null);
                    }
                });
                for (final Account.Cape cape : capes) {
                    final CapePicture picture = new CapePicture(activity, null);
                    Icons.loadBitmap(cape.url, new Icons.Got() {
                        public void got(Bitmap bitmap) {
                            picture.cape = bitmap;
                            picture.invalidate();
                        }
                    });
                    addTile(picture, cape.alias.isEmpty() ? "Плащ" : cape.alias, "legendary", cape.active, new Runnable() {
                        public void run() {
                            setCape(cape.id, cape.url);
                        }
                    });
                }
            }
        });
    }

    private void setCape(final String id, final String url) {
        final Account account = activity.account();
        activity.background(new MainActivity.Job<Boolean>() {
            public Boolean run() throws Exception {
                account.wearCape(id);
                return true;
            }

            public void done(Boolean ok, Exception error) {
                if (error != null) {
                    Toast.makeText(activity, error.getMessage(), Toast.LENGTH_LONG).show();
                    return;
                }
                Toast.makeText(activity, id == null ? "Плащ снят" : "Плащ надет — его видно в игре", Toast.LENGTH_SHORT).show();
                SkinCache.useGeneratedCape(activity, null);  // a real cape replaces the generated one
                activity.accountChanged();
                pop(id != null);
                if (url == null) {
                    character.setCape(null);
                } else {
                    Icons.loadBitmap(url, new Icons.Got() {
                        public void got(Bitmap bitmap) {
                            character.setCape(bitmap);
                        }
                    });
                }
                showTab(CAPES);
            }
        });
    }

    /* ------------------------------------------------------------ pictures */

    /** the stage behind the player: a crimson light in the middle and hard rays, like the stage of Millida's wardrobe */
    static final class StageDrawable extends android.graphics.drawable.Drawable {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final PixelDrawable frame;
        private final float inset;

        StageDrawable(float density) {
            frame = PixelDrawable.panel(0xFF2A0A12, 0xFF060103, density);
            inset = 2 * Math.max(1, Math.round(2 * density));
        }

        @Override
        public void draw(Canvas canvas) {
            Rect b = getBounds();
            frame.setBounds(b);
            frame.draw(canvas);
            canvas.save();
            canvas.clipRect(b.left + inset, b.top + inset, b.right - inset, b.bottom - inset);
            float cx = b.exactCenterX(), cy = b.top + b.height() * 0.45f, r = Math.max(b.width(), b.height());
            paint.setShader(new RadialGradient(cx, cy, r * 0.6f, new int[] { 0xFF8A2036, 0xFF4A1020, 0xFF1C060B }, new float[] { 0f, 0.45f, 1f },
                                               Shader.TileMode.CLAMP));
            canvas.drawRect(b, paint);
            paint.setShader(null);
            paint.setColor(0x22FFB0BC);
            Path path = new Path();
            for (int i = 0; i < 18; i++) {
                double a0 = Math.toRadians(i * 20), a1 = Math.toRadians(i * 20 + 8);
                path.reset();
                path.moveTo(cx, cy);
                path.lineTo(cx + (float) Math.cos(a0) * r, cy + (float) Math.sin(a0) * r);
                path.lineTo(cx + (float) Math.cos(a1) * r, cy + (float) Math.sin(a1) * r);
                path.close();
                canvas.drawPath(path, paint);
            }
            canvas.restore();
        }

        @Override
        public void setAlpha(int alpha) {}

        @Override
        public void setColorFilter(android.graphics.ColorFilter filter) {}

        @Override
        public int getOpacity() {
            return android.graphics.PixelFormat.OPAQUE;
        }
    }

    /** one item alone, as big as it fits, moving like on the player (wings flap, pets hop); effects show their particles */
    static final class ItemIcon extends View {
        final Cosmetics.Item item;
        private final long start = android.os.SystemClock.uptimeMillis();

        ItemIcon(Context context, Cosmetics.Item item) {
            super(context);
            this.item = item;
        }

        @Override
        protected void onDraw(Canvas canvas) {
            if (item == null) {
                return;
            }
            float time = (android.os.SystemClock.uptimeMillis() - start) / 1000f;
            float w = getWidth() - getPaddingLeft() - getPaddingRight(), h = getHeight() - getPaddingTop() - getPaddingBottom();
            if (item.particleColors != null) {
                // an effect: its particles around an empty place where the player stands
                float u = Math.min(w / 28f, h / 44f);
                float ox = getPaddingLeft() + (w - 28 * u) / 2 + 6 * u, oy = getPaddingTop() + (h - 44 * u) / 2 + 8 * u;
                Cosmetics.Pen pen = new Cosmetics.Pen(canvas, ox, oy, u);
                pen.rect(4, 0, 8, 8, 0x332B0E15);
                pen.rect(4, 8, 8, 24, 0x332B0E15);
                Cosmetics.drawParticles(pen, item, time);
                postInvalidateDelayed(60);
                return;
            }
            boolean behind = false;
            float[] box = Cosmetics.bounds(item, false);
            if (box == null) {
                box = Cosmetics.bounds(item, true);
                behind = true;
            }
            if (box == null) {
                return;
            }
            float bw = box[2] - box[0], bh = box[3] - box[1];
            float u = Math.min(w / (bw + 1), h / (bh + 1));
            float ox = getPaddingLeft() + (w - bw * u) / 2 - box[0] * u;
            float oy = getPaddingTop() + (h - bh * u) / 2 - box[1] * u;
            Cosmetics.draw(new Cosmetics.Pen(canvas, ox, oy, u), item, behind, time);
            for (Cosmetics.Layer layer : item.layers) {
                if (!layer.anim.isEmpty()) {
                    postInvalidateDelayed(60);
                    break;
                }
            }
        }
    }

    /** a small one-colour pixel picture (the tab icons) */
    static final class PixelIcon extends View {
        private final String[] rows;
        private final Paint paint = new Paint();

        PixelIcon(Context context, String[] rows) {
            super(context);
            this.rows = rows;
            paint.setColor(0xFFFBEFF1);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            int cols = 0;
            for (String row : rows) {
                cols = Math.max(cols, row.length());
            }
            float u = (float) Math.floor(Math.min(getWidth() / (float) (cols + 1), getHeight() / (float) (rows.length + 1)));
            u = Math.max(1, u);
            float ox = (getWidth() - cols * u) / 2, oy = (getHeight() - rows.length * u) / 2;
            for (int j = 0; j < rows.length; j++) {
                for (int i = 0; i < rows[j].length(); i++) {
                    if (rows[j].charAt(i) != '.') {
                        canvas.drawRect(ox + i * u, oy + j * u, ox + (i + 1) * u, oy + (j + 1) * u, paint);
                    }
                }
            }
        }
    }

    /** a small picture of an item on a dark figure (or a whole outfit); it moves a little, like the big one */
    static final class ItemPicture extends View {
        final String head, body, back, pet;
        String emote = "", effect = "";
        boolean closeUp;  // in a lobby tile: the head and shoulders, big
        private final long start = android.os.SystemClock.uptimeMillis();

        ItemPicture(Context context, String head, String body, String back, String pet) {
            super(context);
            this.head = head;
            this.body = body;
            this.back = back;
            this.pet = pet;
        }

        @Override
        protected void onDraw(Canvas canvas) {
            Context context = getContext();
            float time = (android.os.SystemClock.uptimeMillis() - start) / 1000f;
            float w = getWidth() - getPaddingLeft() - getPaddingRight(), h = getHeight() - getPaddingTop() - getPaddingBottom();
            float u = closeUp ? Math.min(w / 34f, h / 24f) : Math.min(w / 32f, h / 40f);
            float ox = getPaddingLeft() + (closeUp ? (w - 16 * u) / 2 : (w - 32 * u) / 2 + 8 * u);
            float oy = getPaddingTop() + (closeUp ? h * 0.08f + 7 * u : (h - 40 * u) / 2 + 8 * u);
            Cosmetics.Pen pen = new Cosmetics.Pen(canvas, ox, oy, u);
            Cosmetics.draw(pen, Cosmetics.find(context, back), false, time);
            boolean waving = emote.equals("wave") || emote.equals("cheer");
            float lift = emote.equals("jump") ? -3 : emote.equals("bow") ? 1 : 0;
            float[][] parts = { { 4, 0, 8, 8 }, { 4, 8, 8, 12 }, { 0, emote.equals("cheer") ? -4 : 8, 4, 12 }, { 12, waving ? -4 : 8, 4, 12 },
                                { 4, 20, 4, 12 }, { 8, 20, 4, 12 } };
            for (float[] r : parts) {
                pen.rect(r[0], r[1] + lift, r[2], r[3], 0xFF5A1E2A);
                pen.rect(r[0] + 0.6f, r[1] + 0.6f + lift, r[2] - 1.2f, r[3] - 1.2f, 0xFF2B0E15);
            }
            pen.rect(6, 4 + lift, 1, 1, 0xFFFF4654);
            pen.rect(9, 4 + lift, 1, 1, 0xFFFF4654);
            Cosmetics.draw(pen, Cosmetics.find(context, body), false, time);
            Cosmetics.draw(pen, Cosmetics.find(context, head), false, time);
            Cosmetics.draw(pen, Cosmetics.find(context, pet), false, time);
            Cosmetics.drawParticles(pen, Cosmetics.find(context, effect), time);
            if (!effect.isEmpty() || isAnimated()) {
                postInvalidateDelayed(60);
            }
        }

        private boolean isAnimated() {
            return !pet.isEmpty() || back.contains("wings") || "halo".equals(head) || "jetpack".equals(back);
        }
    }

    /** a cape seen from behind (its back face), or an empty back */
    static final class CapePicture extends View {
        Bitmap cape;
        private final Paint pixels = new Paint();

        CapePicture(Context context, Bitmap cape) {
            super(context);
            this.cape = cape;
            pixels.setFilterBitmap(false);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            float u = Math.min(getWidth() / 14f, getHeight() / 20f);
            float ox = (getWidth() - 10 * u) / 2, oy = (getHeight() - 16 * u) / 2;
            if (cape == null) {
                Cosmetics.Pen pen = new Cosmetics.Pen(canvas, ox, oy, u);
                pen.rect(0, 0, 10, 16, 0xFF5A1E2A);
                pen.rect(0.5f, 0.5f, 9, 15, 0xFF1C080D);
                pen.rect(2, 7.5f, 6, 1, 0xFF8A3442);
                return;
            }
            float s = cape.getWidth() / 64f;
            canvas.drawBitmap(cape, new Rect(Math.round(s), Math.round(s), Math.round(11 * s), Math.round(17 * s)),
                              new RectF(ox, oy, ox + 10 * u, oy + 16 * u), pixels);
        }
    }
}
