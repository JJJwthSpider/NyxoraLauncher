package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

/**
 * The player in 3D, like the character of Millida Launcher's lobby: the Minecraft model (head, body, arms, legs, with the
 * second layer of the skin and the real cape) drawn from the skin by a small software renderer with a depth buffer, so
 * it stays pixel-sharp on every phone. It breathes, looks around, swings its arms; a finger turns it around, a tap plays
 * an animation (wave, jump, dance, spin, bow, cheer). The accessories of the wardrobe are built in 3D from their pixels
 * (hats and crowns around the head, plates on the clothes, wings and capes on the back, the pet beside the player) and
 * the effects fly around it in 3D.
 */
final class CharacterView extends View {
    private static final int HEAD = 0, BODY = 1, RARM = 2, LARM = 3, RLEG = 4, LLEG = 5, WORLD = 6, RWING = 7, LWING = 8, CAPE = 9, PARTS = 10;
    private static final String[] EMOTES = { "wave", "jump", "dance", "spin", "bow", "cheer", "clap", "salute", "shrug", "point", "dab", "flip", "think", "punch" };
    private static final float BASE_YAW = (float) Math.toRadians(24);

    // the textures as pixels
    private Bitmap skinBitmap;
    private int[] skin;
    private int skinW, skinH;
    private int[] cape;
    private int capeW, capeH;
    private boolean slim;
    private static int[] shadowSkin;  // when there is no skin yet: a dark figure with a crimson rim and red eyes

    // what is shown
    private boolean turnable = true;
    private boolean closeUp;
    private boolean animated = true;
    private boolean showItems = true;
    private boolean back;
    private String nametag = "";
    private String previewSlot, previewId;
    private String emote = "";
    private long emoteStart;
    private final long start = SystemClock.uptimeMillis();

    // turning with a finger
    private float yaw = BASE_YAW, yawSpeed, pitch = (float) Math.toRadians(-10);
    private float downX, downY, lastX;
    private long downTime, lastTouch;
    private boolean dragging;
    private final int slop;
    private final Random random = new Random();

    // the frame: drawn into pixels with a depth buffer, then scaled up sharp
    private int bufW, bufH;
    private int[] color;
    private float[] depth;
    private Bitmap frame;
    private float scale, centerX, centerY;
    private final float[][] mats = new float[PARTS][12];
    private final Paint pixels = new Paint();
    private final Paint glow = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint tagText = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint tagBack = new Paint();
    private static final Map<String, List<Box>> MESHES = new HashMap<String, List<Box>>();

    /** one box of an accessory: the part it moves with, its corners and its colour */
    private static final class Box {
        final int part, color;
        final float x0, y0, z0, x1, y1, z1;

        Box(int part, float x0, float y0, float z0, float x1, float y1, float z1, int color) {
            this.part = part;
            this.x0 = x0;
            this.y0 = y0;
            this.z0 = z0;
            this.x1 = x1;
            this.y1 = y1;
            this.z1 = z1;
            this.color = color;
        }
    }

    CharacterView(Context context) {
        super(context);
        pixels.setFilterBitmap(false);
        slop = ViewConfiguration.get(context).getScaledTouchSlop();
        tagText.setColor(0xFFFFFFFF);
        tagText.setTextAlign(Paint.Align.CENTER);
        tagBack.setColor(0x8C000000);
        setClickable(true);
    }

    /* ------------------------------------------------------------ what to show */

    void setTurnable(boolean turnable) {
        this.turnable = turnable;
        setClickable(turnable);
    }

    void setCloseUp(boolean closeUp) {
        this.closeUp = closeUp;
        invalidate();
    }

    /** false: drawn once, still (the small pictures of the skin generator) */
    void setAnimated(boolean animated) {
        this.animated = animated;
        invalidate();
    }

    /** false: only the skin, without the wardrobe items (the generator's variants) */
    void setShowItems(boolean show) {
        showItems = show;
        invalidate();
    }

    void setSlim(boolean slim) {
        this.slim = slim;
        invalidate();
    }

    void setSkin(Bitmap bitmap) {
        skinBitmap = bitmap;
        if (bitmap == null || bitmap.getWidth() < 64 || bitmap.getHeight() < 32) {
            skin = null;
        } else {
            Bitmap b = bitmap;
            if (b.getWidth() != 64) {  // HD skins: the same layout, scaled to 64
                b = Bitmap.createScaledBitmap(bitmap, 64, b.getHeight() * 64 / b.getWidth(), false);
            }
            skinW = b.getWidth();
            skinH = b.getHeight();
            skin = new int[skinW * skinH];
            b.getPixels(skin, 0, skinW, 0, 0, skinW, skinH);
        }
        invalidate();
    }

    Bitmap skin() {
        return skinBitmap;
    }

    void setCape(Bitmap bitmap) {
        if (bitmap == null) {
            cape = null;
        } else {
            capeW = bitmap.getWidth();
            capeH = bitmap.getHeight();
            cape = new int[capeW * capeH];
            bitmap.getPixels(cape, 0, capeW, 0, 0, capeW, capeH);
        }
        invalidate();
    }

    void setBack(boolean back) {
        this.back = back;
        lastTouch = 0;  // turn there now
        invalidate();
    }

    /** turns the player to this angle (degrees, 0: facing us) and keeps it there for a while */
    void turnTo(float degrees) {
        yaw = (float) Math.toRadians(degrees);
        yawSpeed = 0;
        lastTouch = SystemClock.uptimeMillis();
        invalidate();
    }

    void setNametag(String name) {
        nametag = name == null ? "" : name;
        tagText.setTypeface(Ui.pixel(getContext()));
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

    /* ------------------------------------------------------------ the finger */

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!turnable) {
            return false;  // inside a tile: the tile gets the tap
        }
        switch (event.getActionMasked()) {
        case MotionEvent.ACTION_DOWN:
            downX = lastX = event.getX();
            downY = event.getY();
            downTime = SystemClock.uptimeMillis();
            dragging = false;
            yawSpeed = 0;
            getParent().requestDisallowInterceptTouchEvent(true);
            return true;
        case MotionEvent.ACTION_MOVE:
            if (!dragging && Math.abs(event.getX() - downX) + Math.abs(event.getY() - downY) > slop) {
                dragging = true;
            }
            if (dragging) {
                float dx = event.getX() - lastX;
                yaw += dx * 0.012f;
                yawSpeed = dx * 0.012f;
                pitch = Math.max(-0.5f, Math.min(0.25f, pitch + (event.getY() - downY) * 0.00015f));
                lastX = event.getX();
                lastTouch = SystemClock.uptimeMillis();
                invalidate();
            }
            return true;
        case MotionEvent.ACTION_UP:
            lastTouch = SystemClock.uptimeMillis();
            if (!dragging && SystemClock.uptimeMillis() - downTime < 400) {
                // a tap: an animation, never the same twice in a row
                String next;
                do {
                    next = EMOTES[random.nextInt(EMOTES.length)];
                } while (next.equals(emote) && EMOTES.length > 1);
                playEmote(next);
                Sounds.play(Sounds.TOGGLE);
                performClick();
            }
            dragging = false;
            return true;
        case MotionEvent.ACTION_CANCEL:
            dragging = false;
            return true;
        default:
            return true;
        }
    }

    @Override
    public boolean performClick() {
        return super.performClick();
    }

    /* ------------------------------------------------------------ 3x4 matrices */

    private static void identity(float[] m) {
        Arrays.fill(m, 0);
        m[0] = m[5] = m[10] = 1;
    }

    /** out = a * b (b applied first) */
    private static void mul(float[] a, float[] b, float[] out) {
        float[] r = new float[12];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                float v = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
                if (j == 3) {
                    v += a[i * 4 + 3];
                }
                r[i * 4 + j] = v;
            }
        }
        System.arraycopy(r, 0, out, 0, 12);
    }

    /** a rotation about a pivot: first z, then x, then y */
    private static float[] rot(float px, float py, float pz, float rx, float ry, float rz) {
        float cx = (float) Math.cos(rx), sx = (float) Math.sin(rx);
        float cy = (float) Math.cos(ry), sy = (float) Math.sin(ry);
        float cz = (float) Math.cos(rz), sz = (float) Math.sin(rz);
        float[] z = { cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0 };
        float[] x = { 1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0 };
        float[] y = { cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0 };
        float[] r = new float[12];
        mul(x, z, r);
        mul(y, r, r);
        // T(p) * R * T(-p)
        r[3] = px - (r[0] * px + r[1] * py + r[2] * pz);
        r[7] = py - (r[4] * px + r[5] * py + r[6] * pz);
        r[11] = pz - (r[8] * px + r[9] * py + r[10] * pz);
        return r;
    }

    private static float[] translate(float x, float y, float z) {
        return new float[] { 1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z };
    }

    /* ------------------------------------------------------------ drawing */

    @Override
    protected void onDraw(Canvas canvas) {
        int w = getWidth(), h = getHeight();
        if (w <= 0 || h <= 0) {
            return;
        }
        int k = Math.max(1, (int) Math.ceil(h / 360f));
        int bw = Math.max(1, w / k), bh = Math.max(1, h / k);
        if (bw != bufW || bh != bufH || frame == null) {
            bufW = bw;
            bufH = bh;
            color = new int[bw * bh];
            depth = new float[bw * bh];
            frame = Bitmap.createBitmap(bw, bh, Bitmap.Config.ARGB_8888);
        }
        long now = SystemClock.uptimeMillis();
        float time = (now - start) / 1000f;
        float emoteTime = (now - emoteStart) / 1000f;
        boolean emoting = !emote.isEmpty() && emoteTime < 2.4f;

        // turning: the fling slows down, then the player turns back to face us (or shows the back in the wardrobe)
        if (!dragging) {
            yaw += yawSpeed;
            yawSpeed *= 0.92f;
            if (Math.abs(yawSpeed) < 0.002f) {
                yawSpeed = 0;
            }
            if (yawSpeed == 0 && now - lastTouch > 2500) {
                float target = BASE_YAW + (back ? (float) Math.PI : 0);
                float diff = (float) Math.atan2(Math.sin(target - yaw), Math.cos(target - yaw));
                yaw += diff * 0.06f;
                pitch += ((float) Math.toRadians(-10) - pitch) * 0.06f;
            }
        }

        // the pose
        float breath = (float) Math.sin(time * 2) * 0.25f;
        float lift = 0, spin = 0, bow = 0, sway = 0, flipAngle = 0;
        float rArmX = (float) Math.sin(time * 1.3) * 0.07f, lArmX = -rArmX;
        float rArmZ = -0.05f - (float) Math.sin(time * 2) * 0.02f, lArmZ = -rArmZ;
        float rLegX = 0, lLegX = 0;
        float headYaw = (float) Math.sin(time * 0.6) * 0.22f, headPitch = (float) Math.sin(time * 0.45) * 0.07f;
        if (!animated) {
            breath = 0;
            rArmX = lArmX = 0;
            headYaw = headPitch = 0;
        }
        if (emoting) {
            float p = emoteTime / 2.4f;
            float fade = Math.min(1, Math.min(emoteTime * 4, (2.4f - emoteTime) * 4));
            if (emote.equals("wave")) {
                rArmZ = mix(rArmZ, -2.55f + (float) Math.sin(emoteTime * 12) * 0.28f, fade);
                headYaw *= 1 - fade;
            } else if (emote.equals("cheer")) {
                float wob = (float) Math.sin(emoteTime * 14) * 0.2f;
                rArmZ = mix(rArmZ, -2.7f + wob, fade);
                lArmZ = mix(lArmZ, 2.7f - wob, fade);
                lift = (float) Math.abs(Math.sin(emoteTime * 7)) * 1.2f * fade;
            } else if (emote.equals("jump")) {
                float j = (float) Math.max(0, Math.sin(emoteTime * Math.PI / 0.6));
                lift = j * 7;
                rArmZ = mix(rArmZ, -0.9f, j);
                lArmZ = mix(lArmZ, 0.9f, j);
                rLegX = -j * 0.5f;
                lLegX = j * 0.3f;
            } else if (emote.equals("dance")) {
                float b = (float) Math.sin(emoteTime * 9);
                sway = b * 0.35f * fade;
                rArmX = mix(rArmX, -1.6f + b * 0.9f, fade);
                lArmX = mix(lArmX, -1.6f - b * 0.9f, fade);
                rArmZ = mix(rArmZ, -0.4f, fade);
                lArmZ = mix(lArmZ, 0.4f, fade);
                rLegX = b * 0.35f * fade;
                lLegX = -b * 0.35f * fade;
                lift = (float) Math.abs(Math.cos(emoteTime * 9)) * 1.0f * fade;
            } else if (emote.equals("spin")) {
                float e = Math.min(1, p * 1.6f);
                spin = (float) (Math.PI * 2 * (e * e * (3 - 2 * e)));
                rArmZ = mix(rArmZ, -1.2f, fade);
                lArmZ = mix(lArmZ, 1.2f, fade);
            } else if (emote.equals("bow")) {
                bow = (float) Math.sin(Math.min(1, p * 1.4f) * Math.PI) * 0.6f;
                rArmX = mix(rArmX, -0.4f, fade);
                lArmX = mix(lArmX, 0.3f, fade);
            } else if (emote.equals("clap")) {
                float c = (float) Math.abs(Math.sin(emoteTime * 9));
                rArmX = mix(rArmX, -1.25f, fade);
                lArmX = mix(lArmX, -1.25f, fade);
                rArmZ = mix(rArmZ, 0.15f + c * 0.35f, fade);
                lArmZ = mix(lArmZ, -0.15f - c * 0.35f, fade);
            } else if (emote.equals("salute")) {
                rArmZ = mix(rArmZ, -2.35f, fade);
                rArmX = mix(rArmX, -0.55f, fade);
                headPitch = mix(headPitch, -0.08f, fade);
                headYaw *= 1 - fade;
            } else if (emote.equals("shrug")) {
                float u = (float) Math.abs(Math.sin(emoteTime * 3));
                rArmZ = mix(rArmZ, -0.8f - u * 0.2f, fade);
                lArmZ = mix(lArmZ, 0.8f + u * 0.2f, fade);
                rArmX = mix(rArmX, -0.6f, fade);
                lArmX = mix(lArmX, -0.6f, fade);
                headPitch = mix(headPitch, 0.12f, fade);
                headYaw = mix(headYaw, (float) Math.sin(emoteTime * 3) * 0.25f, fade);
            } else if (emote.equals("point")) {
                rArmX = mix(rArmX, -1.6f, fade);
                rArmZ = mix(rArmZ, 0.05f, fade);
                headYaw *= 1 - fade;
            } else if (emote.equals("dab")) {
                rArmZ = mix(rArmZ, -2.3f, fade);
                rArmX = mix(rArmX, -0.3f, fade);
                lArmX = mix(lArmX, -1.3f, fade);
                lArmZ = mix(lArmZ, -0.7f, fade);
                headPitch = mix(headPitch, 0.45f, fade);
                headYaw = mix(headYaw, 0.45f, fade);
            } else if (emote.equals("flip")) {
                float e = Math.min(1, p * 1.5f);
                float arc = (float) Math.sin(e * Math.PI);
                lift = arc * 9;
                flipAngle = -(float) (Math.PI * 2 * (e * e * (3 - 2 * e)));
                rArmZ = mix(rArmZ, -2.6f, arc);
                lArmZ = mix(lArmZ, 2.6f, arc);
            } else if (emote.equals("think")) {
                rArmX = mix(rArmX, -2.0f, fade);
                rArmZ = mix(rArmZ, 0.55f, fade);
                headPitch = mix(headPitch, 0.18f, fade);
                headYaw = mix(headYaw, -0.2f, fade);
            } else if (emote.equals("punch")) {
                float a = (float) Math.max(0, Math.sin(emoteTime * 10)), b = (float) Math.max(0, -Math.sin(emoteTime * 10));
                rArmX = mix(rArmX, -0.6f - a * 1.0f, fade);
                lArmX = mix(lArmX, -0.6f - b * 1.0f, fade);
                sway = (a - b) * 0.15f * fade;
            }
        }
        if (animated && !emoting && !dragging) {
            // the legs shift a little under the breathing body
            rLegX = (float) Math.sin(time * 1.3) * 0.03f;
            lLegX = -rLegX;
        }

        // the camera: the whole model turns, then the view looks a little from above
        float totalYaw = yaw + spin + sway * 0.4f;
        float[] camera = rot(0, 0, 0, pitch, 0, 0);
        mul(camera, rot(0, 0, 0, 0, totalYaw, 0), camera);
        mul(camera, translate(0, lift + breath * 0.3f, 0), camera);
        if (flipAngle != 0) {
            mul(camera, rot(0, 16, 0, flipAngle, 0, 0), camera);  // a backflip about the middle of the body
        }
        float[] upper = rot(0, 12, 0, bow, sway * 0.6f, 0);
        float[] upperCam = new float[12];
        mul(camera, upper, upperCam);
        float armW = slim ? 3 : 4;
        mul(upperCam, rot(0, 24, 0, headPitch + bow * 0.5f, headYaw, 0), mats[HEAD]);
        System.arraycopy(upperCam, 0, mats[BODY], 0, 12);
        mul(upperCam, rot(-4 - armW / 2, 22, 0, rArmX, 0, rArmZ), mats[RARM]);
        mul(upperCam, rot(4 + armW / 2, 22, 0, lArmX, 0, lArmZ), mats[LARM]);
        mul(camera, rot(-2, 12, 0, rLegX, 0, 0), mats[RLEG]);
        mul(camera, rot(2, 12, 0, lLegX, 0, 0), mats[LLEG]);
        System.arraycopy(camera, 0, mats[WORLD], 0, 12);
        float flap = animated ? (float) (0.35 + 0.35 * Math.sin(time * 2.4)) : 0.4f;
        mul(upperCam, rot(-1, 20, -2.6f, 0, -flap, 0), mats[RWING]);
        mul(upperCam, rot(1, 20, -2.6f, 0, flap, 0), mats[LWING]);
        float capeTilt = 0.12f + (animated ? (float) Math.sin(time * 1.7) * 0.04f : 0) + (lift > 0 ? lift * 0.03f : 0) + Math.abs(yawSpeed) * 2;
        mul(upperCam, rot(0, 24, -2.9f, capeTilt, 0, 0), mats[CAPE]);

        // the view: everything fits, or the head and shoulders fill a tile
        if (closeUp) {
            scale = Math.min(bufW / 24f, bufH / 19f);
            centerX = bufW / 2f;
            centerY = bufH * 0.55f + 25 * scale;
        } else {
            scale = Math.min(bufW / 34f, bufH / 44f);
            centerX = bufW / 2f;
            centerY = bufH * 0.5f + 15 * scale;
        }
        Arrays.fill(color, 0);
        Arrays.fill(depth, -1e9f);

        // the player
        int[] tex = skin != null ? skin : shadow();
        int tw = skin != null ? skinW : 64, th = skin != null ? skinH : 64;
        boolean modern = th >= 64;
        drawBox(mats[HEAD], -4, 24, -4, 8, 8, 8, 0, 0, tex, tw, th, 0);
        drawBox(mats[BODY], -4, 12, -2, 8, 12, 4, 16, 16, tex, tw, th, 0);
        drawBox(mats[RARM], -4 - armW, 12, -2, armW, 12, 4, 40, 16, tex, tw, th, 0);
        drawBox(mats[RLEG], -4, 0, -2, 4, 12, 4, 0, 16, tex, tw, th, 0);
        if (modern) {
            drawBox(mats[LARM], 4, 12, -2, armW, 12, 4, 32, 48, tex, tw, th, 0);
            drawBox(mats[LLEG], 0, 0, -2, 4, 12, 4, 16, 48, tex, tw, th, 0);
            // the second layer, a little bigger
            drawBox(mats[HEAD], -4, 24, -4, 8, 8, 8, 32, 0, tex, tw, th, 0.5f);
            drawBox(mats[BODY], -4, 12, -2, 8, 12, 4, 16, 32, tex, tw, th, 0.25f);
            drawBox(mats[RARM], -4 - armW, 12, -2, armW, 12, 4, 40, 32, tex, tw, th, 0.25f);
            drawBox(mats[LARM], 4, 12, -2, armW, 12, 4, 48, 48, tex, tw, th, 0.25f);
            drawBox(mats[RLEG], -4, 0, -2, 4, 12, 4, 0, 32, tex, tw, th, 0.25f);
            drawBox(mats[LLEG], 0, 0, -2, 4, 12, 4, 0, 48, tex, tw, th, 0.25f);
        } else {
            // old 64x32 skins: the left arm and leg are the right ones mirrored
            drawBoxMirrored(mats[LARM], 4, 12, -2, armW, 12, 4, 40, 16, tex, tw, th);
            drawBoxMirrored(mats[LLEG], 0, 0, -2, 4, 12, 4, 0, 16, tex, tw, th);
            drawBox(mats[HEAD], -4, 24, -4, 8, 8, 8, 32, 0, tex, tw, th, 0.5f);
        }

        // the cape: the real one of the account, or one from the generator (an item cape replaces it)
        String backItem = showItems ? worn(Cosmetics.BACK) : "";
        if (cape != null && !backItem.contains("cape") && !"backpack".equals(backItem) && !"jetpack".equals(backItem)) {
            float[] capeMat = new float[12];
            mul(mats[CAPE], rot(0, 24, -2.9f, 0, (float) Math.PI, 0), capeMat);
            // a box of 10x16x1 hanging from the shoulders; turned around, its outer side faces back
            int cw = capeW, ch = capeH;
            int[] capeTex = cape;
            if (capeW != 64) {  // HD capes are drawn at their own size: scale the coordinates
                capeTex = scaledCape(64);
                cw = 64;
                ch = 32;
            }
            drawBox(capeMat, -5, 8, -3.4f, 10, 16, 1, 0, 0, capeTex, cw, ch, 0);
        }

        // the accessories in 3D, and the effect around
        Context context = getContext();
        if (showItems) {
            drawItem(Cosmetics.find(context, backItem), Cosmetics.BACK, time);
            drawItem(Cosmetics.find(context, worn(Cosmetics.BODY)), Cosmetics.BODY, time);
            drawItem(Cosmetics.find(context, worn(Cosmetics.HEAD)), Cosmetics.HEAD, time);
            drawItem(Cosmetics.find(context, worn(Cosmetics.PET)), Cosmetics.PET, time);
            drawItem(Cosmetics.find(context, worn(Cosmetics.HAND)), Cosmetics.HAND, time);
            drawParticles(Cosmetics.find(context, worn(Cosmetics.EFFECT)), time);
        }

        // the crimson light on the floor, then the frame, sharp
        float[] foot = project(mats[WORLD], 0, 0, 0);
        float k2 = (float) w / bufW;
        float fx = foot[0] * k2, fy = foot[1] * k2 + (lift > 0 ? lift * scale * k2 : 0);
        float radius = 16 * scale * k2;
        glow.setShader(new RadialGradient(fx, fy, radius, new int[] { 0x70E8364F, 0x00E8364F }, null, Shader.TileMode.CLAMP));
        canvas.save();
        canvas.scale(1, 0.25f, fx, fy);
        canvas.drawCircle(fx, fy, radius, glow);
        canvas.restore();
        frame.setPixels(color, 0, bufW, 0, 0, bufW, bufH);
        canvas.drawBitmap(frame, new Rect(0, 0, bufW, bufH), new Rect(0, 0, bufW * k, bufH * k), pixels);

        if (!nametag.isEmpty() && !closeUp) {
            float[] top = project(mats[HEAD], 0, 36, 0);
            tagText.setTextSize(Math.max(10 * getResources().getDisplayMetrics().density, 2.6f * scale * k2));
            float tx = top[0] * k2, ty = top[1] * k2;
            float half = tagText.measureText(nametag) / 2 + scale * k2;
            Paint.FontMetrics fm = tagText.getFontMetrics();
            canvas.drawRect(tx - half, ty + fm.ascent - scale * k2 * 0.4f, tx + half, ty + fm.descent + scale * k2 * 0.4f, tagBack);
            canvas.drawText(nametag, tx, ty, tagText);
        }
        if (animated) {
            postInvalidateDelayed(33);
        }
    }

    private static float mix(float a, float b, float t) {
        return a + (b - a) * t;
    }

    private float[] project(float[] m, float x, float y, float z) {
        float X = m[0] * x + m[1] * y + m[2] * z + m[3];
        float Y = m[4] * x + m[5] * y + m[6] * z + m[7];
        float Z = m[8] * x + m[9] * y + m[10] * z + m[11];
        return new float[] { centerX + X * scale, centerY - Y * scale, Z };
    }

    private int[] scaledCape(int width) {
        Bitmap b = Bitmap.createBitmap(cape, capeW, capeH, Bitmap.Config.ARGB_8888);
        Bitmap s = Bitmap.createScaledBitmap(b, width, width / 2, false);
        int[] out = new int[width * width / 2];
        s.getPixels(out, 0, width, 0, 0, width, width / 2);
        return out;
    }

    /** a dark figure for when there is no skin: crimson rim, red eyes */
    private static int[] shadow() {
        if (shadowSkin != null) {
            return shadowSkin;
        }
        int[] t = new int[64 * 64];
        int[][] boxes = { { 0, 0, 8, 8, 8 }, { 16, 16, 8, 12, 4 }, { 40, 16, 4, 12, 4 }, { 0, 16, 4, 12, 4 }, { 32, 48, 4, 12, 4 }, { 16, 48, 4, 12, 4 } };
        for (int[] b : boxes) {
            int u = b[0], v = b[1], w = b[2], h = b[3], d = b[4];
            for (int y = v; y < v + d + h; y++) {
                for (int x = u; x < u + 2 * (d + w); x++) {
                    if (y < v + d && (x < u + d || x >= u + d + 2 * w)) {
                        continue;
                    }
                    boolean edge = y == v || y == v + d + h - 1 || x == u || x == u + 2 * (d + w) - 1 || x == u + d || x == u + d + w - 1;
                    t[y * 64 + x] = edge ? 0xFF8A3442 : 0xFF2B0E15;
                }
            }
        }
        t[12 * 64 + 10] = t[12 * 64 + 13] = 0xFFFF4654;  // the eyes
        shadowSkin = t;
        return t;
    }

    /* ------------------------------------------------------------ boxes */

    /**
     * A box of the Minecraft model with the usual texture layout at (u, v): top and bottom on the first row, then right,
     * front, left and back. `inflate` makes the second layer stand off the first.
     */
    private void drawBox(float[] m, float x, float y, float z, float w, float h, float d, int u, int v, int[] tex, int tw, int th, float inflate) {
        float x0 = x - inflate, x1 = x + w + inflate, y0 = y - inflate, y1 = y + h + inflate, z0 = z - inflate, z1 = z + d + inflate;
        int iw = Math.round(w), ih = Math.round(h), id = Math.round(d);
        // front, back, right (x0), left (x1), top, bottom: three corners each (top left, top right, bottom left)
        face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, tex, tw, th, u + id, v + id, iw, ih, 0, false);
        face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, tex, tw, th, u + 2 * id + iw, v + id, iw, ih, 0, false);
        face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, tex, tw, th, u, v + id, id, ih, 0, false);
        face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, tex, tw, th, u + id + iw, v + id, id, ih, 0, false);
        face(m, x0, y1, z0, x1, y1, z0, x0, y1, z1, tex, tw, th, u + id, v, iw, id, 0, false);
        face(m, x0, y0, z1, x1, y0, z1, x0, y0, z0, tex, tw, th, u + id + iw, v, iw, id, 0, false);
    }

    /** the left arm or leg of an old skin: the right one's texture, mirrored */
    private void drawBoxMirrored(float[] m, float x, float y, float z, float w, float h, float d, int u, int v, int[] tex, int tw, int th) {
        float x0 = x, x1 = x + w, y0 = y, y1 = y + h, z0 = z, z1 = z + d;
        int iw = Math.round(w), ih = Math.round(h), id = Math.round(d);
        face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, tex, tw, th, u + id, v + id, iw, ih, 0, true);
        face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, tex, tw, th, u + 2 * id + iw, v + id, iw, ih, 0, true);
        face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, tex, tw, th, u, v + id, id, ih, 0, true);
        face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, tex, tw, th, u + id + iw, v + id, id, ih, 0, true);
        face(m, x0, y1, z0, x1, y1, z0, x0, y1, z1, tex, tw, th, u + id, v, iw, id, 0, true);
        face(m, x0, y0, z1, x1, y0, z1, x0, y0, z0, tex, tw, th, u + id + iw, v, iw, id, 0, true);
    }

    /**
     * One face, a parallelogram from three corners (top left, top right, bottom left) in the part's space, with a
     * texture rectangle or one colour; it is shaded by how it faces the light and drawn with the depth test.
     */
    private void face(float[] m, float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, int[] tex, int tw, int th,
                      int su, int sv, int sw, int sh, int solid, boolean mirror) {
        float AX = m[0] * ax + m[1] * ay + m[2] * az + m[3], AY = m[4] * ax + m[5] * ay + m[6] * az + m[7], AZ = m[8] * ax + m[9] * ay + m[10] * az + m[11];
        float BX = m[0] * bx + m[1] * by + m[2] * bz + m[3], BY = m[4] * bx + m[5] * by + m[6] * bz + m[7], BZ = m[8] * bx + m[9] * by + m[10] * bz + m[11];
        float CX = m[0] * cx + m[1] * cy + m[2] * cz + m[3], CY = m[4] * cx + m[5] * cy + m[6] * cz + m[7], CZ = m[8] * cx + m[9] * cy + m[10] * cz + m[11];
        // the normal: (bottom left - top left) x (top right - top left); seen only from the front
        float ex = BX - AX, ey = BY - AY, ez = BZ - AZ, fx = CX - AX, fy = CY - AY, fz = CZ - AZ;
        float nx = fy * ez - fz * ey, ny = fz * ex - fx * ez, nz = fx * ey - fy * ex;
        float len = (float) Math.sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-6f || nz / len <= 0.001f) {
            return;  // it faces away from us
        }
        // light from the upper left, in front: the top bright, the bottom dark, like Minecraft
        float light = (nx * -0.35f + ny * 0.8f + nz * 0.5f) / len / 0.995f;
        float bright = 0.56f + 0.46f * Math.max(0, light);
        if (ny / len < -0.5f) {
            bright *= 0.85f;
        }
        int b256 = Math.round(Math.min(1.08f, bright) * 256);
        float sAx = centerX + AX * scale, sAy = centerY - AY * scale;
        float e1x = ex * scale, e1y = -ey * scale, e2x = fx * scale, e2y = -fy * scale;
        float det = e1x * e2y - e1y * e2x;
        if (Math.abs(det) < 1e-4f) {
            return;
        }
        float minX = Math.min(Math.min(sAx, sAx + e1x), Math.min(sAx + e2x, sAx + e1x + e2x));
        float maxX = Math.max(Math.max(sAx, sAx + e1x), Math.max(sAx + e2x, sAx + e1x + e2x));
        float minY = Math.min(Math.min(sAy, sAy + e1y), Math.min(sAy + e2y, sAy + e1y + e2y));
        float maxY = Math.max(Math.max(sAy, sAy + e1y), Math.max(sAy + e2y, sAy + e1y + e2y));
        int x0 = Math.max(0, (int) Math.floor(minX)), x1 = Math.min(bufW - 1, (int) Math.ceil(maxX));
        int y0 = Math.max(0, (int) Math.floor(minY)), y1 = Math.min(bufH - 1, (int) Math.ceil(maxY));
        float dsdx = e2y / det, dsdy = -e2x / det, dtdx = -e1y / det, dtdy = e1x / det;
        float dz1 = BZ - AZ, dz2 = CZ - AZ;
        float eps = 0.02f;
        for (int py = y0; py <= y1; py++) {
            float ry = py + 0.5f - sAy;
            float rx = x0 + 0.5f - sAx;
            float s = rx * dsdx + ry * dsdy, t = rx * dtdx + ry * dtdy;
            int row = py * bufW;
            for (int px = x0; px <= x1; px++, s += dsdx, t += dtdx) {
                if (s < -eps || s >= 1 + eps || t < -eps || t >= 1 + eps) {
                    continue;
                }
                float z = AZ + s * dz1 + t * dz2;
                int i = row + px;
                if (z <= depth[i]) {
                    continue;
                }
                int c;
                if (tex != null) {
                    int tu = (int) (Math.max(0, Math.min(0.999f, s)) * sw);
                    int tv = (int) (Math.max(0, Math.min(0.999f, t)) * sh);
                    if (mirror) {
                        tu = sw - 1 - tu;
                    }
                    int ux = su + tu, vy = sv + tv;
                    if (ux < 0 || vy < 0 || ux >= tw || vy >= th) {
                        continue;
                    }
                    c = tex[vy * tw + ux];
                } else {
                    c = solid;
                }
                if ((c >>> 24) < 128) {
                    continue;
                }
                int r = Math.min(255, (((c >> 16) & 0xFF) * b256) >> 8);
                int g = Math.min(255, (((c >> 8) & 0xFF) * b256) >> 8);
                int bl = Math.min(255, ((c & 0xFF) * b256) >> 8);
                color[i] = 0xFF000000 | (r << 16) | (g << 8) | bl;
                depth[i] = z;
            }
        }
    }

    /** a solid box (an accessory voxel run) */
    private void solidBox(float[] m, float x0, float y0, float z0, float x1, float y1, float z1, int c) {
        face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, null, 0, 0, 0, 0, 1, 1, c, false);
        face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, null, 0, 0, 0, 0, 1, 1, c, false);
        face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, null, 0, 0, 0, 0, 1, 1, c, false);
        face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, null, 0, 0, 0, 0, 1, 1, c, false);
        face(m, x0, y1, z0, x1, y1, z0, x0, y1, z1, null, 0, 0, 0, 0, 1, 1, c, false);
        face(m, x0, y0, z1, x1, y0, z1, x0, y0, z0, null, 0, 0, 0, 0, 1, 1, c, false);
    }

    /* ------------------------------------------------------------ accessories in 3D */

    private void drawItem(Cosmetics.Item item, String slot, float time) {
        if (item == null) {
            return;
        }
        List<Box> mesh = MESHES.get(item.id);
        if (mesh == null) {
            mesh = build(item, slot);
            MESHES.put(item.id, mesh);
        }
        float bob = 0;
        String anim = item.layers.isEmpty() ? "" : item.layers.get(0).anim;
        if (animated && "bob".equals(anim)) {
            bob = (float) Math.sin(time * 2) * 0.5f;
        } else if (animated && "hop".equals(anim)) {
            bob = (float) Math.abs(Math.sin(time * 3)) * 2;
        } else if (animated && "float".equals(anim)) {
            bob = (float) Math.sin(time * 2) * 1.2f;
        }
        float[] shifted = new float[12];
        for (Box b : mesh) {
            float[] m = mats[b.part];
            if (bob != 0) {
                mul(m, translate(0, bob, 0), shifted);
                m = shifted;
            }
            solidBox(m, b.x0, b.y0, b.z0, b.x1, b.y1, b.z1, b.color);
        }
    }

    /**
     * The look for the game (the Nyxora Cosmetics mod; the PC launcher writes the same): the accessories of the head, body
     * and back slots as boxes on the parts of the Minecraft player model (pets, things in the hand and effects stay in
     * Nyxora), the skin and the cape, written into `dir` as look.json, skin.png and cape.png. The launcher's model has y
     * up, the feet at 0 and the face to +z; Minecraft's has y down from the neck and the face to -z, each box in the pixels
     * of its part, from the part's pivot.
     */
    static void exportLook(Context context, java.io.File dir, Bitmap skin, boolean slim, Bitmap cape, String head, String body, String back)
        throws java.io.IOException, org.json.JSONException {
        if (!dir.isDirectory() && !dir.mkdirs()) {
            throw new java.io.IOException("cannot make " + dir);
        }
        new java.io.File(dir, "skin.png").delete();
        new java.io.File(dir, "cape.png").delete();
        org.json.JSONObject look = new org.json.JSONObject();
        look.put("version", 1);
        if (skin != null) {
            Bitmap full = skin;
            if (skin.getWidth() == 64 && skin.getHeight() == 32) {
                // the old 64x32 layout: the 64x64 one the player model reads, the left arm and leg from the right ones
                full = Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888);
                Canvas canvas = new Canvas(full);
                canvas.drawBitmap(skin, 0, 0, null);
                canvas.drawBitmap(Bitmap.createBitmap(skin, 0, 16, 16, 16), 16, 48, null);
                canvas.drawBitmap(Bitmap.createBitmap(skin, 40, 16, 16, 16), 32, 48, null);
            }
            savePng(full, new java.io.File(dir, "skin.png"));
            look.put("skin", "skin.png");
            look.put("slim", slim);
        }
        if (cape != null) {
            savePng(cape, new java.io.File(dir, "cape.png"));
            look.put("cape", "cape.png");
        }
        String[] names = { "head", "body", "rightArm", "leftArm", "rightLeg", "leftLeg" };
        float armY = slim ? 2.5f : 2f;
        float[][] pivots = { { 0, 0, 0 }, { 0, 0, 0 }, { -5, armY, 0 }, { 5, armY, 0 }, { -1.9f, 12, 0 }, { 1.9f, 12, 0 } };
        org.json.JSONArray[] boxes = new org.json.JSONArray[6];
        for (int i = 0; i < 6; i++) {
            boxes[i] = new org.json.JSONArray();
        }
        String[][] worn = { { Cosmetics.HEAD, head }, { Cosmetics.BODY, body }, { Cosmetics.BACK, back } };
        for (String[] w : worn) {
            Cosmetics.Item item = w[1] == null || w[1].isEmpty() ? null : Cosmetics.find(context, w[1]);
            if (item == null) {
                continue;
            }
            for (Box b : build(item, w[0])) {
                int part;
                switch (b.part) {
                    case HEAD: part = 0; break;
                    case BODY: case RWING: case LWING: case CAPE: part = 1; break;  // wings and capes of the back hang on the body
                    case RARM: part = 2; break;
                    case LARM: part = 3; break;
                    case RLEG: part = 4; break;
                    case LLEG: part = 5; break;
                    default: continue;
                }
                float[] p = pivots[part];
                org.json.JSONArray a = new org.json.JSONArray();
                a.put(b.x0 - p[0]).put(24 - b.y1 - p[1]).put(-b.z1 - p[2]).put(b.x1 - p[0]).put(24 - b.y0 - p[1]).put(-b.z0 - p[2]).put(b.color & 0xFFFFFFFFL);
                boxes[part].put(a);
            }
        }
        org.json.JSONObject parts = new org.json.JSONObject();
        for (int i = 0; i < 6; i++) {
            if (boxes[i].length() > 0) {
                parts.put(names[i], boxes[i]);
            }
        }
        look.put("parts", parts);
        java.io.FileOutputStream out = new java.io.FileOutputStream(new java.io.File(dir, "look.json"));
        try {
            out.write(look.toString().getBytes("UTF-8"));
        } finally {
            out.close();
        }
    }

    private static void savePng(Bitmap bitmap, java.io.File file) throws java.io.IOException {
        java.io.FileOutputStream out = new java.io.FileOutputStream(file);
        try {
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
        } finally {
            out.close();
        }
    }

    /**
     * Builds an item's 3D shape from its pixels (the front picture, and the back one where there is one). Each pixel
     * becomes a box, runs of the same colour are joined; where it goes depends on the slot: around the head (wide rows
     * all the way round, narrow ones like ears thin), as plates on the clothes, on the back (wings turn at the spine,
     * capes hang), or beside the player (pets).
     */
    private static List<Box> build(Cosmetics.Item item, String slot) {
        List<Box> out = new ArrayList<Box>();
        // crowns, halos, tiaras and wreaths go round the head
        boolean ring = item.id.contains("crown") || item.id.contains("halo") || item.id.contains("tiara") || item.id.contains("wreath");
        boolean hasFront = false, hasBack = false;
        for (Cosmetics.Layer l : item.layers) {
            hasFront |= !"back".equals(l.view);
            hasBack |= "back".equals(l.view);
        }
        for (Cosmetics.Layer layer : item.layers) {
            boolean backView = "back".equals(layer.view);
            float px = 1f / layer.res;  // one row character, in skin pixels
            for (int j = 0; j < layer.rows.length; j++) {
                String row = layer.rows[j];
                String relief = layer.relief != null && j < layer.relief.length ? layer.relief[j] : "";
                int i = 0;
                while (i < row.length()) {
                    char ch = row.charAt(i);
                    if (ch == '.' || ch == ' ' || ch >= 128 || layer.colors[ch] == 0) {
                        i++;
                        continue;
                    }
                    // a run of one colour and one relief; and how wide the whole filled stretch of this row is
                    int bump = bumpAt(relief, i);
                    int end = i;
                    while (end < row.length() && row.charAt(end) == ch && bumpAt(relief, end) == bump) {
                        end++;
                    }
                    int segStart = i, segEnd = end;
                    while (segStart > 0 && isFilled(layer, row.charAt(segStart - 1))) {
                        segStart--;
                    }
                    while (segEnd < row.length() && isFilled(layer, row.charAt(segEnd))) {
                        segEnd++;
                    }
                    float gx0 = layer.x + i * px, gx1 = layer.x + end * px, gy = layer.y + j * px;
                    int c = layer.colors[ch];
                    // thicker in the middle of the shape than at its edges: rounded, not a flat plate
                    float bulge = bump * 0.12f;
                    place(out, item, slot, backView, hasFront, hasBack, ring, gx0, gx1, gy, (segEnd - segStart) * px, segStart == i, segEnd == end, c, px, bulge);
                    i = end;
                }
            }
        }
        return out;
    }

    /** how far a character of a finished layer is from the edge of its shape (0..4; 0 without relief) */
    private static int bumpAt(String relief, int i) {
        if (i >= relief.length()) {
            return 0;
        }
        char d = relief.charAt(i);
        return d >= '0' && d <= '9' ? d - '0' : 0;
    }

    private static boolean isFilled(Cosmetics.Layer layer, char ch) {
        return ch != '.' && ch != ' ' && ch < 128 && layer.colors[ch] != 0;
    }

    private static void add(List<Box> out, int part, float x0, float y0, float z0, float x1, float y1, float z1, int c) {
        out.add(new Box(part, x0, y0, z0, x1, y1, z1, c));
    }

    /** where one run of pixels goes in 3D (grid: the 16x32 figure of the pictures, its top left at 0,0) */
    private static void place(List<Box> out, Cosmetics.Item item, String slot, boolean backView, boolean hasFront, boolean hasBack, boolean ring,
                              float gx0, float gx1, float gy, float segWidth, boolean segLeft, boolean segRight, int c, float px, float bulge) {
        // the model: x from the player's right (-8) to the left (+8), y up from the feet (32 at the top of the head)
        float x0 = gx0 - 8, x1 = gx1 - 8;
        float frontGx = (gx0 + gx1) / 2;  // to tell which part the pixels are on
        if (backView) {  // seen from behind the sides swap
            x0 = 8 - gx1;
            x1 = 8 - gx0;
            frontGx = 16 - frontGx;
        }
        float y1 = 32 - gy, y0 = y1 - px;
        if (Cosmetics.HEAD.equals(slot)) {
            boolean onFace = frontGx >= 4 && frontGx < 12 && gy >= 0 && gy < 8;
            if (onFace || gy >= 8) {
                if (backView) {
                    add(out, HEAD, x0, y0, -4.65f - bulge, x1, y1, -4.05f, c);
                } else {
                    add(out, HEAD, x0, y0, 4.05f, x1, y1, 4.65f + bulge, c);  // glasses, a mask: on the face
                }
            } else if (backView) {
                return;  // the front picture already went all the way round
            } else if (ring) {  // crowns and halos: everything on a ring round the head, the points of a crown too
                add(out, HEAD, x0, y0, -4.7f, x1, y1, -3.7f, c);
                add(out, HEAD, x0, y0, 3.7f, x1, y1, 4.7f, c);
                if (segWidth >= 6 && segLeft) {
                    add(out, HEAD, x0, y0, -4.7f, x0 + 1, y1, 4.7f, c);
                }
                if (segWidth >= 6 && segRight) {
                    add(out, HEAD, x1 - 1, y0, -4.7f, x1, y1, 4.7f, c);
                }
            } else {
                // hats, helmets and horns: as deep as they are wide (a cone stays round), a brim all the way round; ears flat
                boolean ears = item.id.endsWith("_ears");
                float half = ears ? 0.7f + bulge / 2 : Math.min(4.7f, Math.max(1.2f, segWidth / 2) + bulge / 2);
                add(out, HEAD, x0, y0, -half, x1, y1, half, c);
            }
        } else if (Cosmetics.BODY.equals(slot)) {
            int part = BODY;
            float zf = 2.05f, zb = 2.65f;
            if (gy < 8) {
                part = HEAD;
                zf = 4.05f;
                zb = 4.65f;
            } else if (gy >= 20) {
                part = frontGx < 8 ? RLEG : LLEG;
            } else if (frontGx < 4) {
                part = RARM;
            } else if (frontGx >= 12) {
                part = LARM;
            }
            if (backView) {
                add(out, part, x0, y0, -zb - bulge, x1, y1, -zf, c);
            } else {
                add(out, part, x0, y0, zf, x1, y1, zb + bulge, c);
            }
        } else if (Cosmetics.BACK.equals(slot)) {
            boolean wings = item.id.contains("wings");
            boolean pack = item.id.contains("pack");
            boolean capeItem = item.id.contains("cape");
            if (wings) {
                if (backView && hasFront) {
                    return;
                }
                float mid = (x0 + x1) / 2;
                add(out, mid < 0 ? RWING : LWING, x0, y0, -3.2f - bulge, x1, y1, -2.5f, c);
            } else if (pack) {
                if (!backView && hasBack && frontGx >= 4 && frontGx < 12) {
                    return;  // the back picture shows it better
                }
                add(out, BODY, x0, y0, -5.2f - bulge, x1, y1, -2.05f, c);
            } else if (capeItem) {
                if (!backView && hasBack) {
                    return;
                }
                add(out, CAPE, x0, y0, -3.1f - bulge / 2, x1, y1, -2.5f, c);
            } else {
                if (!backView && hasBack && frontGx >= 4 && frontGx < 12) {
                    return;
                }
                add(out, BODY, x0, y0, -3.2f - bulge, x1, y1, -2.3f, c);
            }
        } else if (Cosmetics.HAND.equals(slot)) {
            if (backView) {
                return;
            }
            add(out, RARM, x0, y0, 2.1f - bulge / 2, x1, y1, 2.9f + bulge / 2, c);  // held in the right hand, in front of it: it moves with the arm
        } else if (Cosmetics.PET.equals(slot)) {
            if (backView) {
                return;
            }
            add(out, WORLD, x0 + 1, y0, 1.5f - bulge / 2, x1 + 1, y1, 4.5f + bulge / 2, c);  // beside the player, a little in front
        }
    }

    /* ------------------------------------------------------------ effects in 3D */

    private static float hash(int a, int b) {
        int h = a * 374761393 + b * 668265263 + 0x2545F491;
        h = (h ^ (h >>> 13)) * 1274126177;
        return ((h ^ (h >>> 16)) & 0x7fffffff) % 10000 / 10000f;
    }

    private void drawParticles(Cosmetics.Item item, float time) {
        if (item == null || item.particleColors == null) {
            return;
        }
        float[] m = mats[WORLD];
        for (int i = 0; i < item.particleCount; i++) {
            int col = item.particleColors[i % item.particleColors.length];
            float speed = 4 + hash(i, 1) * 5, phase = hash(i, 2) * 40;
            float x, y, z, alpha;
            if ("orbit".equals(item.motion)) {
                double angle = time * 1.4 + i * Math.PI * 2 / item.particleCount;
                x = (float) Math.cos(angle) * 11;
                z = (float) Math.sin(angle) * 11;
                y = 16 + (float) Math.sin(time * 2 + i) * 7;
                alpha = 1;
            } else if ("fall".equals(item.motion)) {
                x = -14 + hash(i, 3) * 28;
                z = -10 + hash(i, 6) * 20;
                y = 40 - (time * speed + phase) % 44;
                alpha = 0.85f;
            } else if ("spark".equals(item.motion)) {
                x = -12 + hash(i, 3) * 24;
                z = -8 + hash(i, 6) * 16;
                y = 2 + hash(i, 4) * 36;
                alpha = (float) Math.max(0, Math.sin(time * (2 + hash(i, 5) * 3) + phase));
            } else {  // rise
                x = -10 + hash(i, 3) * 20;
                z = -7 + hash(i, 6) * 14;
                float travel = (time * speed + phase) % 40;
                y = travel - 2;
                alpha = Math.min(1, travel / 6) * Math.max(0, 1 - travel / 40);
            }
            if (alpha <= 0.05f) {
                continue;
            }
            float[] p = project(m, x, y, z);
            int size = Math.max(1, Math.round(scale * ("heart".equals(item.shape) ? 0.45f : "note".equals(item.shape) || "star".equals(item.shape) ? 1.2f : 0.8f)));
            if ("heart".equals(item.shape)) {
                int[][] heart = { { 0, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 }, { 1, 2 } };
                for (int[] h : heart) {
                    dot(p[0] + (h[0] - 1) * size, p[1] + (h[1] - 1) * size, p[2], size, col, alpha);
                }
            } else if ("drop".equals(item.shape)) {
                dot(p[0], p[1], p[2], size, col, alpha);
                dot(p[0], p[1] + size, p[2], size, col, alpha);
            } else if ("note".equals(item.shape)) {
                int q = Math.max(1, size / 2);
                for (int k = 0; k < 4; k++) {
                    dot(p[0] + q, p[1] - k * q, p[2], q, col, alpha);  // the stem
                }
                dot(p[0] + 2 * q, p[1] - 3 * q, p[2], q, col, alpha);
                dot(p[0], p[1], p[2], q * 2, col, alpha);             // the head
            } else if ("star".equals(item.shape)) {
                int q = Math.max(1, size / 2);
                dot(p[0], p[1], p[2], q, col, alpha);
                dot(p[0] - q, p[1], p[2], q, col, alpha);
                dot(p[0] + q, p[1], p[2], q, col, alpha);
                dot(p[0], p[1] - q, p[2], q, col, alpha);
                dot(p[0], p[1] + q, p[2], q, col, alpha);
            } else {
                dot(p[0], p[1], p[2], size, col, alpha);
            }
        }
    }

    private void dot(float sx, float sy, float z, int size, int col, float alpha) {
        int a = Math.round(((col >>> 24) & 0xFF) * Math.min(1, alpha));
        int x0 = Math.round(sx - size / 2f), y0 = Math.round(sy - size / 2f);
        for (int y = y0; y < y0 + size; y++) {
            if (y < 0 || y >= bufH) {
                continue;
            }
            for (int x = x0; x < x0 + size; x++) {
                if (x < 0 || x >= bufW) {
                    continue;
                }
                int i = y * bufW + x;
                if (z < depth[i]) {
                    continue;  // behind the player
                }
                int under = color[i];
                int ua = under >>> 24;
                int r = blend((under >> 16) & 0xFF, (col >> 16) & 0xFF, a, ua);
                int g = blend((under >> 8) & 0xFF, (col >> 8) & 0xFF, a, ua);
                int b = blend(under & 0xFF, col & 0xFF, a, ua);
                color[i] = (Math.max(ua, a) << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    private static int blend(int under, int over, int a, int ua) {
        if (ua == 0) {
            return over;
        }
        return (under * (255 - a) + over * a) / 255;
    }
}
