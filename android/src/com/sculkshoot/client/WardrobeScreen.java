package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

/**
 * The wardrobe, like Millida Launcher's: the player big on the left (a tap turns it around), the tabs on the right
 * (outfits, capes, head, body, back, pets, emotes) and the items as tiles. A tap puts an item on or takes it off; the
 * capes are the real Minecraft capes of the account and are seen in the game.
 */
final class WardrobeScreen {
    private static final String[][] TABS = { { "sets", "Образы" }, { "capes", "Плащи" }, { Cosmetics.HEAD, "Голова" }, { Cosmetics.BODY, "Тело" },
                                             { Cosmetics.BACK, "Спина" }, { Cosmetics.PET, "Питомцы" }, { Cosmetics.EMOTE, "Эмоции" } };

    private final MainActivity activity;
    private final LinearLayout root;
    private final CharacterView character;
    private final GridLayout grid;
    private final TextView section;
    private final List<Button> tabButtons = new ArrayList<Button>();
    private String tab = "sets";

    WardrobeScreen(MainActivity activity) {
        this.activity = activity;
        root = new LinearLayout(activity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackground(new RainDrawable(activity.getResources().getDisplayMetrics().density));

        LinearLayout bar = new LinearLayout(activity);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(10), dp(6), dp(10), dp(6));
        bar.setBackgroundColor(0xE60B0305);
        Button back = Ui.button(activity, "← Лобби", false);
        back.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                WardrobeScreen.this.activity.showHome();
            }
        });
        bar.addView(back);
        TextView title = Ui.label(activity, "Гардероб", 18, Ui.TEXT, true);
        title.setPadding(dp(14), 0, 0, 0);
        bar.addView(title, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        Button undress = Ui.button(activity, "Снять всё", false);
        undress.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                for (String slot : new String[] { Cosmetics.HEAD, Cosmetics.BODY, Cosmetics.BACK, Cosmetics.PET }) {
                    Cosmetics.wear(WardrobeScreen.this.activity, slot, "");
                }
                character.invalidate();
                showTab(tab);
            }
        });
        bar.addView(undress);
        root.addView(bar);

        LinearLayout body = new LinearLayout(activity);
        body.setPadding(dp(10), dp(10), dp(10), dp(10));
        root.addView(body, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        // left: the player on a crimson-lit stage
        LinearLayout stage = new LinearLayout(activity);
        stage.setOrientation(LinearLayout.VERTICAL);
        stage.setGravity(Gravity.CENTER_HORIZONTAL);
        stage.setBackground(Ui.rounded(activity, 0xE6140509, 0, 0xFF5A1E2A));
        stage.setPadding(dp(8), dp(8), dp(8), dp(10));
        character = new CharacterView(activity);
        stage.addView(character, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        TextView hint = Ui.label(activity, "Нажми на персонажа, чтобы повернуть", 11, Ui.HINT, false);
        hint.setGravity(Gravity.CENTER);
        stage.addView(hint);
        body.addView(stage, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 4));

        // right: the tabs and the items
        LinearLayout right = new LinearLayout(activity);
        right.setOrientation(LinearLayout.VERTICAL);
        right.setPadding(dp(10), 0, 0, 0);
        HorizontalScrollView tabsScroll = new HorizontalScrollView(activity);
        tabsScroll.setHorizontalScrollBarEnabled(false);
        LinearLayout tabs = new LinearLayout(activity);
        for (final String[] t : TABS) {
            Button b = Ui.button(activity, t[1], t[0].equals(tab));
            b.setTextSize(13);
            b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    showTab(t[0]);
                }
            });
            b.setTag(t[0]);
            tabButtons.add(b);
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            p.rightMargin = dp(6);
            tabs.addView(b, p);
        }
        tabsScroll.addView(tabs);
        right.addView(tabsScroll);
        section = Ui.label(activity, "", 16, Ui.TEXT, true);
        section.setPadding(dp(2), dp(10), 0, dp(6));
        right.addView(section);
        ScrollView scroll = new ScrollView(activity);
        grid = new GridLayout(activity);
        grid.setColumnCount(4);
        scroll.addView(grid);
        right.addView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        body.addView(right, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 6));

        loadSkinAndCape(activity, character);
        showTab(tab);
    }

    View view() {
        return root;
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    /** the account's skin and active cape on a character */
    static void loadSkinAndCape(final MainActivity activity, final CharacterView view) {
        final Account account = activity.account();
        if (account == null || account.skinUrl.isEmpty()) {
            return;
        }
        Icons.loadBitmap(account.skinUrl, new Icons.Got() {
            public void got(Bitmap bitmap) {
                view.setSkin(bitmap);
            }
        });
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
                if (url != null && !url.isEmpty()) {
                    Icons.loadBitmap(url, new Icons.Got() {
                        public void got(Bitmap bitmap) {
                            view.setCape(bitmap);
                        }
                    });
                }
            }
        });
    }

    private void showTab(String id) {
        tab = id;
        for (Button b : tabButtons) {
            Ui.styleButton(activity, b, id.equals(b.getTag()));
        }
        grid.removeAllViews();
        character.setBack(Cosmetics.BACK.equals(id) || "capes".equals(id));
        if ("sets".equals(id)) {
            section.setText("Образы");
            for (final String[] set : Cosmetics.SETS) {
                boolean on = set[1].equals(Cosmetics.worn(activity, Cosmetics.HEAD)) && set[2].equals(Cosmetics.worn(activity, Cosmetics.BACK));
                addTile(new ItemPicture(activity, set[1], set[3], set[2], set[4]), set[0], on, new Runnable() {
                    public void run() {
                        Cosmetics.wearSet(activity, set);
                        character.setBack(false);
                        showTab("sets");
                    }
                });
            }
        } else if ("capes".equals(id)) {
            section.setText("Плащи Minecraft — их видно в игре");
            showCapes();
        } else {
            section.setText(id.equals(Cosmetics.EMOTE) ? "Эмоции — нажми, чтобы персонаж сыграл" : titleOf(id));
            final String slot = id;
            for (final Cosmetics.Item item : Cosmetics.inSlot(slot)) {
                boolean on = item.id.equals(Cosmetics.worn(activity, slot));
                ItemPicture picture = new ItemPicture(activity, slot.equals(Cosmetics.HEAD) ? item.id : "", slot.equals(Cosmetics.BODY) ? item.id : "",
                                                      slot.equals(Cosmetics.BACK) ? item.id : "", slot.equals(Cosmetics.PET) ? item.id : "");
                picture.emote = slot.equals(Cosmetics.EMOTE) ? item.id : "";
                addTile(picture, item.title, on && !slot.equals(Cosmetics.EMOTE), new Runnable() {
                    public void run() {
                        if (slot.equals(Cosmetics.EMOTE)) {
                            character.setBack(false);
                            character.playEmote(item.id);
                            return;
                        }
                        boolean wearing = item.id.equals(Cosmetics.worn(activity, slot));
                        Cosmetics.wear(activity, slot, wearing ? "" : item.id);
                        character.invalidate();
                        showTab(slot);
                    }
                });
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

    private void addTile(View picture, String name, boolean on, final Runnable action) {
        LinearLayout tile = new LinearLayout(activity);
        tile.setOrientation(LinearLayout.VERTICAL);
        tile.setGravity(Gravity.CENTER_HORIZONTAL);
        tile.setPadding(dp(4), dp(4), dp(4), dp(8));
        tile.setBackground(on ? Ui.rainbowFrame(activity, 0) : PixelDrawable.plain(activity.getResources().getDisplayMetrics().density));
        tile.addView(picture, new LinearLayout.LayoutParams(dp(96), dp(84)));
        TextView label = Ui.label(activity, name, 11, Ui.TEXT, true);
        label.setGravity(Gravity.CENTER);
        label.setMaxWidth(dp(100));
        tile.addView(label);
        if (on) {
            tile.addView(Ui.label(activity, "надето", 9, 0xFFFF8090, false));
        }
        tile.setClickable(true);
        tile.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                action.run();
            }
        });
        GridLayout.LayoutParams p = new GridLayout.LayoutParams();
        p.setMargins(dp(4), dp(4), dp(4), dp(4));
        grid.addView(tile, p);
    }

    private void showCapes() {
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
                if (!"capes".equals(tab)) {
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
                addTile(new CapePicture(activity, null), "Без плаща", !anyActive, new Runnable() {
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
                    addTile(picture, cape.alias.isEmpty() ? "Плащ" : cape.alias, cape.active, new Runnable() {
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
                if (url == null) {
                    character.setCape(null);
                } else {
                    Icons.loadBitmap(url, new Icons.Got() {
                        public void got(Bitmap bitmap) {
                            character.setCape(bitmap);
                        }
                    });
                }
                showTab("capes");
            }
        });
    }

    /** a small still picture of an item on a dark figure (or a whole outfit) */
    static final class ItemPicture extends View {
        final String head, body, back, pet;
        String emote = "";

        ItemPicture(Context context, String head, String body, String back, String pet) {
            super(context);
            this.head = head;
            this.body = body;
            this.back = back;
            this.pet = pet;
        }

        @Override
        protected void onDraw(Canvas canvas) {
            float u = Math.min(getWidth() / 32f, getHeight() / 38f);
            float ox = (getWidth() - 32 * u) / 2 + 8 * u;
            float oy = (getHeight() - 38 * u) / 2 + 6 * u;
            Cosmetics.Pen pen = new Cosmetics.Pen(canvas, ox, oy, u);
            Cosmetics.drawBack(pen, back, 0.5f, false);
            boolean waving = emote.equals("wave");
            float lift = emote.equals("jump") ? -3 : 0;
            float[][] parts = { { 4, 0, 8, 8 }, { 4, 8, 8, 12 }, { 0, 8, 4, 12 }, { 12, waving ? -4 : 8, 4, 12 }, { 4, 20, 4, 12 }, { 8, 20, 4, 12 } };
            for (float[] r : parts) {
                pen.rect(r[0], r[1] + lift, r[2], r[3], 0xFF5A1E2A);
                pen.rect(r[0] + 0.6f, r[1] + 0.6f + lift, r[2] - 1.2f, r[3] - 1.2f, 0xFF2B0E15);
            }
            pen.rect(6, 4 + lift, 1, 1, 0xFFFF4654);
            pen.rect(9, 4 + lift, 1, 1, 0xFFFF4654);
            Cosmetics.drawBody(pen, body);
            Cosmetics.drawHead(pen, head, 0);
            Cosmetics.drawPet(pen, pet, 0);
        }
    }

    /** a cape seen from behind (its back face), or an empty back */
    static final class CapePicture extends View {
        Bitmap cape;
        private final android.graphics.Paint pixels = new android.graphics.Paint();

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
