package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;

/**
 * The player in the lobby and the wardrobe, like the character of Millida Launcher: drawn from the skin with sharp
 * pixels, it breathes, its wings move, the pet hops beside it, and it plays emotes (wave, jump, dance). A tap turns it
 * around to show the back with the cape. What it wears comes from Cosmetics; `preview` overrides one slot (the wardrobe
 * shows an item before it is put on).
 */
final class CharacterView extends View {
    private static final float GRID_W = 32, GRID_H = 46;  // the figure (16x32) with room around for wings, hats and the pet
    private static final float FIGURE_X = 8, FIGURE_Y = 10;

    private final Paint pixels = new Paint();
    private final Paint glow = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final long start = SystemClock.uptimeMillis();
    private Bitmap skin;
    private Bitmap cape;      // the real Minecraft cape (64x32 texture), or null
    private boolean back;     // seen from behind
    private String emote = "";
    private long emoteStart;
    private String previewSlot, previewId;

    CharacterView(Context context) {
        super(context);
        pixels.setFilterBitmap(false);
        setOnTouchListener(new OnTouchListener() {
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_UP) {
                    back = !back;  // a tap turns the player around
                    invalidate();
                }
                return true;
            }
        });
    }

    void setSkin(Bitmap skin) {
        this.skin = skin;
        invalidate();
    }

    void setCape(Bitmap cape) {
        this.cape = cape;
        invalidate();
    }

    void setBack(boolean back) {
        this.back = back;
        invalidate();
    }

    /** try an item on without wearing it (null: show what is worn) */
    void preview(String slot, String id) {
        previewSlot = slot;
        previewId = id;
        invalidate();
    }

    void playEmote(String id) {
        emote = id == null ? "" : id;
        emoteStart = SystemClock.uptimeMillis();
        invalidate();
    }

    private String worn(String slot) {
        if (slot.equals(previewSlot) && previewId != null) {
            return previewId;
        }
        return Cosmetics.worn(getContext(), slot);
    }

    /** a part of the skin: from (sx, sy, w, h) of the texture to (x, y) of the figure */
    private void part(Canvas c, float ox, float oy, float u, int sx, int sy, int w, int h, float x, float y, boolean mirror) {
        if (skin == null || sx + w > skin.getWidth() || sy + h > skin.getHeight()) {
            return;
        }
        RectF dst = new RectF(ox + x * u, oy + y * u, ox + (x + w) * u, oy + (y + h) * u);
        if (mirror) {
            c.save();
            c.scale(-1, 1, dst.centerX(), dst.centerY());
            c.drawBitmap(skin, new Rect(sx, sy, sx + w, sy + h), dst, pixels);
            c.restore();
        } else {
            c.drawBitmap(skin, new Rect(sx, sy, sx + w, sy + h), dst, pixels);
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        float u = (float) Math.floor(Math.min(getWidth() / GRID_W, getHeight() / GRID_H));
        if (u < 1) {
            u = Math.min(getWidth() / GRID_W, getHeight() / GRID_H);
        }
        float time = (SystemClock.uptimeMillis() - start) / 1000f;
        float emoteTime = (SystemClock.uptimeMillis() - emoteStart) / 1000f;
        boolean emoting = !emote.isEmpty() && emoteTime < 2.4f;
        float breath = (float) Math.sin(time * 2) * 0.35f;
        float jump = emoting && emote.equals("jump") ? (float) Math.max(0, Math.sin(emoteTime * Math.PI / 0.6)) * 5 : 0;
        float sway = emoting && emote.equals("dance") ? (float) Math.sin(emoteTime * 9) * 1.5f : 0;
        float ox = (getWidth() - GRID_W * u) / 2 + (FIGURE_X + sway) * u;
        float oy = (getHeight() - GRID_H * u) / 2 + (FIGURE_Y - jump + breath) * u;

        // the red glow on the floor under the feet
        float floorY = (getHeight() - GRID_H * u) / 2 + (FIGURE_Y + 32) * u;
        float cx = (getWidth()) / 2f;
        glow.setShader(new RadialGradient(cx, floorY, 14 * u, new int[] { 0x66E8364F, 0x00E8364F }, null, Shader.TileMode.CLAMP));
        canvas.save();
        canvas.scale(1, 0.25f, cx, floorY);
        canvas.drawCircle(cx, floorY, 14 * u, glow);
        canvas.restore();

        Cosmetics.Pen pen = new Cosmetics.Pen(canvas, ox, oy, u);
        float flap = (float) (0.5 + 0.5 * Math.sin(time * (worn(Cosmetics.BACK).contains("wings") ? 2.2 : 0)));
        if (!back) {
            Cosmetics.drawBack(pen, worn(Cosmetics.BACK), flap, false);
        }
        drawFigure(canvas, pen, ox, oy, u, emoting, emoteTime);
        if (back) {
            if (cape != null && !worn(Cosmetics.BACK).equals("crimson_cape")) {
                // the real cape: its back face is at (1, 1) of the 64x32 texture, 10x16
                float s = cape.getWidth() / 64f;
                canvas.drawBitmap(cape, new Rect(Math.round(1 * s), Math.round(1 * s), Math.round(11 * s), Math.round(17 * s)),
                                  new RectF(ox + 3 * u, oy + 8 * u, ox + 13 * u, oy + 24 * u), pixels);
            }
            Cosmetics.drawBack(pen, worn(Cosmetics.BACK), flap, true);
        } else {
            Cosmetics.drawBody(pen, worn(Cosmetics.BODY));
            Cosmetics.drawHead(pen, worn(Cosmetics.HEAD), time);
        }
        Cosmetics.drawPet(pen, worn(Cosmetics.PET), time);
        postInvalidateDelayed(40);
    }

    private void drawFigure(Canvas c, Cosmetics.Pen pen, float ox, float oy, float u, boolean emoting, float emoteTime) {
        boolean wave = emoting && emote.equals("wave");
        if (skin == null) {
            // no skin yet: a dark figure with a crimson rim
            float[][] parts = { { 4, 0, 8, 8 }, { 4, 8, 8, 12 }, { 0, 8, 4, 12 }, { 12, wave ? -4 : 8, 4, 12 }, { 4, 20, 4, 12 }, { 8, 20, 4, 12 } };
            for (float[] r : parts) {
                pen.rect(r[0], r[1], r[2], r[3], 0xFF8A3442);
                pen.rect(r[0] + 0.5f, r[1] + 0.5f, r[2] - 1, r[3] - 1, 0xFF2B0E15);
            }
            if (!back) {
                pen.rect(6, 4, 1, 1, 0xFFFF4654);
                pen.rect(9, 4, 1, 1, 0xFFFF4654);
            }
            return;
        }
        boolean modern = skin.getHeight() >= 64;
        if (!back) {
            part(c, ox, oy, u, 4, 20, 4, 12, 4, 20, false);                                    // right leg
            if (modern) {
                part(c, ox, oy, u, 20, 52, 4, 12, 8, 20, false);                                // left leg
            } else {
                part(c, ox, oy, u, 4, 20, 4, 12, 8, 20, true);
            }
            part(c, ox, oy, u, 20, 20, 8, 12, 4, 8, false);                                    // body
            part(c, ox, oy, u, 44, 20, 4, 12, 0, 8, false);                                    // right arm
            float leftArmY = wave ? -4 + (float) Math.sin(emoteTime * 10) * 0.6f : 8;          // waving: the arm goes up
            if (modern) {
                part(c, ox, oy, u, 36, 52, 4, 12, 12, leftArmY, false);
            } else {
                part(c, ox, oy, u, 44, 20, 4, 12, 12, leftArmY, true);
            }
            part(c, ox, oy, u, 8, 8, 8, 8, 4, 0, false);                                       // head
            if (modern) {
                part(c, ox, oy, u, 4, 36, 4, 12, 4, 20, false);
                part(c, ox, oy, u, 4, 52, 4, 12, 8, 20, false);
                part(c, ox, oy, u, 20, 36, 8, 12, 4, 8, false);
                part(c, ox, oy, u, 44, 36, 4, 12, 0, 8, false);
                part(c, ox, oy, u, 52, 52, 4, 12, 12, leftArmY, false);
            }
            part(c, ox, oy, u, 40, 8, 8, 8, 4, 0, false);                                      // hat layer
        } else {
            // from behind the sides swap: the right arm and leg are on the right of the picture
            part(c, ox, oy, u, 12, 20, 4, 12, 8, 20, false);
            if (modern) {
                part(c, ox, oy, u, 28, 52, 4, 12, 4, 20, false);
            } else {
                part(c, ox, oy, u, 12, 20, 4, 12, 4, 20, true);
            }
            part(c, ox, oy, u, 32, 20, 8, 12, 4, 8, false);
            part(c, ox, oy, u, 52, 20, 4, 12, 12, 8, false);
            float leftArmY = wave ? -4 : 8;
            if (modern) {
                part(c, ox, oy, u, 44, 52, 4, 12, 0, leftArmY, false);
            } else {
                part(c, ox, oy, u, 52, 20, 4, 12, 0, leftArmY, true);
            }
            part(c, ox, oy, u, 24, 8, 8, 8, 4, 0, false);
            if (modern) {
                part(c, ox, oy, u, 32, 36, 8, 12, 4, 8, false);
                part(c, ox, oy, u, 52, 36, 4, 12, 12, 8, false);
                part(c, ox, oy, u, 60, 52, 4, 12, 0, leftArmY, false);
                part(c, ox, oy, u, 12, 36, 4, 12, 8, 20, false);
                part(c, ox, oy, u, 12, 52, 4, 12, 4, 20, false);
            }
            part(c, ox, oy, u, 56, 8, 8, 8, 4, 0, false);
        }
    }
}
