package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.View;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Arkan, Nyxora's AI pack builder, as a 3D mob in the style of a Minecraft mod: a person in a spacesuit made of boxes
 * with a pixel texture (assets/arkan_model.json and arkan.png, made by art/arkan3d.py), drawn here with the same
 * software renderer as the player (CharacterView): a white helmet with a wide glass in the icon's blue and violet, an
 * antenna whose light blinks, a white suit with a lit screen on the chest, a life-support pack, one hand raised with
 * Nyxora's cube (a tesseract) turning in four dimensions over it. He floats over a turning ring of light, turns a
 * little and looks around; while he works the cube spins faster and the sparks fly. In a small place (a chat avatar)
 * only his head and shoulders are shown.
 */
final class WizardView extends View {
    private static final int HEAD = 0, BODY = 1, RARM = 2, LARM = 3, SKIRT = 4;
    private static final String[] PARTS = { "head", "body", "rarm", "larm" };

    /** the model, loaded once */
    private static final class Model {
        int tw, th;
        int[] tex;
        float[][] pivots = new float[4][];
        float[][] hands = new float[4][];
        int magic = RARM;  // the raised hand under the tesseract
        float[][] eyes;
        final List<float[]> glows = new ArrayList<float[]>();  // part, x, y, z, radius: soft lights over the model
        final List<float[]> boxes = new ArrayList<float[]>();  // part, x, y, z, w, h, d, u, v, inflate
    }

    private static Model model;

    private final Paint pixels = new Paint();
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint dot = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint glow = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint spark = new Paint();
    private final float[] tx = new float[16], ty = new float[16];
    private final long start = SystemClock.uptimeMillis();
    private boolean busy;

    private int bufW, bufH;
    private int[] color;
    private float[] depth;
    private Bitmap frame;
    private float scale, centerX, centerY;
    private boolean blink;
    private final float[] tesseract = new float[3];  // where the tesseract is, before the projection: it lights the cloth
    private float tesseractLight;

    WizardView(Context context) {
        super(context);
        load(context);
        pixels.setFilterBitmap(false);  // sharp pixels, like Minecraft
        line.setColor(0xFF9A62FF);
        line.setStrokeCap(Paint.Cap.ROUND);
        dot.setColor(0xFFE0D0FF);
    }

    private Boolean closeUpForced;

    /** only the head and shoulders (by default in places smaller than 80dp) */
    void setCloseUp(boolean closeUp) {
        closeUpForced = closeUp;
        invalidate();
    }

    /** Arkan is building: the cube turns faster, the sparks fly */
    void setBusy(boolean busy) {
        this.busy = busy;
        invalidate();
    }

    private static synchronized void load(Context context) {
        if (model != null) {
            return;
        }
        Model m = new Model();
        try {
            InputStream in = context.getAssets().open("arkan_model.json");
            java.io.ByteArrayOutputStream bytes = new java.io.ByteArrayOutputStream();
            byte[] chunk = new byte[8192];
            for (int n; (n = in.read(chunk)) > 0;) {
                bytes.write(chunk, 0, n);
            }
            in.close();
            JSONObject json = new JSONObject(new String(bytes.toByteArray(), "UTF-8"));
            JSONObject pivots = json.getJSONObject("pivots");
            for (int i = 0; i < PARTS.length; i++) {
                m.pivots[i] = floats(pivots.getJSONArray(PARTS[i]));
            }
            m.hands[RARM] = floats(json.getJSONObject("hands").getJSONArray("rarm"));
            m.hands[LARM] = floats(json.getJSONObject("hands").getJSONArray("larm"));
            m.magic = "larm".equals(json.optString("magicHand", "rarm")) ? LARM : RARM;
            JSONArray glows = json.optJSONArray("glows");
            for (int i = 0; glows != null && i < glows.length(); i++) {
                JSONObject g = glows.getJSONObject(i);
                float[] at = floats(g.getJSONArray("at"));
                m.glows.add(new float[] { Arrays.asList(PARTS).indexOf(g.getString("part")), at[0], at[1], at[2], (float) g.optDouble("radius", 2) });
            }
            JSONArray eyes = json.getJSONArray("eyes");
            m.eyes = new float[eyes.length()][];
            for (int i = 0; i < eyes.length(); i++) {
                m.eyes[i] = floats(eyes.getJSONArray(i));
            }
            JSONArray boxes = json.getJSONArray("boxes");
            for (int i = 0; i < boxes.length(); i++) {
                JSONObject b = boxes.getJSONObject(i);
                int part = Arrays.asList(PARTS).indexOf(b.getString("part"));
                if (part < 0) {
                    continue;
                }
                float[] from = floats(b.getJSONArray("from")), size = floats(b.getJSONArray("size")), uv = floats(b.getJSONArray("uv"));
                if (part == BODY && from[1] < 1) {
                    part = SKIRT;  // the legs sway a little on their own, as he floats
                }
                m.boxes.add(new float[] { part, from[0], from[1], from[2], size[0], size[1], size[2], uv[0], uv[1], (float) b.optDouble("inflate", 0) });
            }
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            options.inPremultiplied = false;  // the alpha 254 of the glowing pixels must stay as it is
            InputStream png = context.getAssets().open("arkan.png");
            Bitmap texture = BitmapFactory.decodeStream(png, null, options);
            png.close();
            m.tw = texture.getWidth();
            m.th = texture.getHeight();
            m.tex = new int[m.tw * m.th];
            texture.getPixels(m.tex, 0, m.tw, 0, 0, m.tw, m.th);
        } catch (Exception e) {
            android.util.Log.w("Nyxora", "Arkan's model did not load: " + e);
            m.boxes.clear();  // no model: the view stays empty rather than crashing
        }
        model = m;
    }

    private static float[] floats(JSONArray a) throws Exception {
        float[] out = new float[a.length()];
        for (int i = 0; i < out.length; i++) {
            out[i] = (float) a.getDouble(i);
        }
        return out;
    }

    /* ------------------------------------------------------------ 3x4 matrices, as in CharacterView */

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
        r[3] = px - (r[0] * px + r[1] * py + r[2] * pz);
        r[7] = py - (r[4] * px + r[5] * py + r[6] * pz);
        r[11] = pz - (r[8] * px + r[9] * py + r[10] * pz);
        return r;
    }

    private static float[] translate(float x, float y, float z) {
        return new float[] { 1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z };
    }

    private static float[] apply(float[] m, float[] p) {
        return new float[] { m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
                             m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11] };
    }

    /** a point of the model on the screen (in buffer pixels) and its depth */
    private float[] project(float[] m, float[] p) {
        float[] q = apply(m, p);
        return new float[] { centerX + q[0] * scale, centerY - q[1] * scale, q[2] };
    }

    /* ------------------------------------------------------------ drawing */

    @Override
    protected void onDraw(Canvas canvas) {
        int w = getWidth(), h = getHeight();
        if (w <= 0 || h <= 0 || model == null || model.boxes.isEmpty()) {
            return;
        }
        float density = getResources().getDisplayMetrics().density;
        boolean closeUp = closeUpForced != null ? closeUpForced : Math.min(w, h) < 80 * density;  // a chat avatar: the head and shoulders
        int k = Math.max(1, (int) Math.ceil(h / 240f));
        int bw = Math.max(1, w / k), bh = Math.max(1, h / k);
        if (bw != bufW || bh != bufH || frame == null) {
            bufW = bw;
            bufH = bh;
            color = new int[bw * bh];
            depth = new float[bw * bh];
            frame = Bitmap.createBitmap(bw, bh, Bitmap.Config.ARGB_8888);
        }
        float time = (SystemClock.uptimeMillis() - start) / 1000f;
        float speed = busy ? 2.2f : 1f;

        // the pose: floating, turning a little towards the raised hand, looking around, the hand up with the tesseract
        Model m = model;
        int magic = m.magic, other = magic == RARM ? LARM : RARM;
        float side = m.pivots[magic][0] > 0 ? 1 : -1;  // +1: the raised hand is on the right of the picture
        float bob = 2.2f + (float) Math.sin(time * 1.6) * 0.7f;
        // turned a little towards the raised hand; in a close-up he looks at you
        float yaw = closeUp ? -side * 0.12f + (float) Math.sin(time * 0.45) * 0.12f : -side * 0.38f + (float) Math.sin(time * 0.45) * 0.32f;
        float headYaw = (float) Math.sin(time * 0.7) * (closeUp ? 0.15f : 0.28f), headPitch = (float) Math.sin(time * 0.5) * 0.08f - 0.04f;
        // the hand out to his side, the palm up, the tesseract over it (the face stays in sight)
        float mPitch = -0.55f + (float) Math.sin(time * 1.6 * speed) * (busy ? 0.18f : 0.05f);
        float mRoll = side * (1.2f + (float) Math.sin(time * 1.1 * speed) * (busy ? 0.15f : 0.05f));
        // the other hand hangs by his side
        float oPitch = -0.12f + (float) Math.sin(time * 1.3) * 0.05f, oRoll = -side * (0.08f + (busy ? (float) Math.sin(time * 5) * 0.05f : 0));
        float skirtSway = (float) Math.sin(time * 1.6 + 1) * 0.045f;
        blink = (time % 4.7f) > 4.55f;

        float[] camera = rot(0, 0, 0, -0.16f, 0, 0);
        mul(camera, rot(0, 0, 0, 0, yaw, 0), camera);
        float[] ground = camera.clone();
        mul(camera, translate(0, bob, 0), camera);
        float[][] mats = new float[5][12];
        System.arraycopy(camera, 0, mats[BODY], 0, 12);
        mul(camera, rot(m.pivots[HEAD][0], m.pivots[HEAD][1], m.pivots[HEAD][2], headPitch, headYaw, 0), mats[HEAD]);
        float[] magicArm = rot(m.pivots[magic][0], m.pivots[magic][1], m.pivots[magic][2], mPitch, 0, mRoll);
        mul(camera, magicArm, mats[magic]);
        mul(camera, rot(m.pivots[other][0], m.pivots[other][1], m.pivots[other][2], oPitch, 0, oRoll), mats[other]);
        mul(camera, rot(0, 12, 0, skirtSway, 0, 0), mats[SKIRT]);
        // the cube over the raised palm; its violet light falls on the suit near it
        float pulse = 0.75f + 0.25f * (float) Math.sin(time * (busy ? 7 : 3));
        float[] palm = apply(magicArm, m.hands[magic]);
        palm[1] += 6.5f;
        System.arraycopy(apply(camera, palm), 0, tesseract, 0, 3);
        tesseractLight = (busy ? 1f : 0.8f) * (0.85f + 0.15f * pulse);

        if (closeUp) {
            scale = Math.min(bufW / 20f, bufH / 19f);
            centerX = bufW / 2f;
            centerY = bufH * 0.5f + (28 + bob) * scale;
        } else {
            scale = Math.min(bufW / 38f, bufH / 46f);
            centerX = bufW * (0.5f - side * 0.05f);  // room for the raised hand and its tesseract
            centerY = bufH * 0.5f + 20 * scale;
        }
        Arrays.fill(color, 0);
        Arrays.fill(depth, -1e9f);

        // the hover ring under him: dots of light turning, violet outside, blue inside
        if (!closeUp) {
            float turn = time * 0.4f * speed;
            for (int i = 0; i < 40; i++) {
                double a = turn + i * Math.PI * 2 / 40;
                runeDot(ground, (float) Math.cos(a) * 11, 0, (float) Math.sin(a) * 11, i % 5 == 0 ? 0xFFAB81FF : 0xFF8154F0);
            }
            for (int i = 0; i < 12; i++) {
                double a = -turn * 1.5 + i * Math.PI * 2 / 12;
                runeDot(ground, (float) Math.cos(a) * 7, 0, (float) Math.sin(a) * 7, 0xFF7CD0FF);
            }
        }

        // the model
        for (float[] b : m.boxes) {
            drawBox(mats[(int) b[0]], b[1], b[2], b[3], b[4], b[5], b[6], (int) b[7], (int) b[8], b[9], b[0] == HEAD);
        }

        // the light under him, then the frame, sharp
        float k2 = k;  // buffer pixels -> view pixels, the same as the frame below
        if (!closeUp) {
            float[] foot = project(ground, new float[] { 0, 0, 0 });
            float fx = foot[0] * k2, fy = foot[1] * k2, radius = 15 * scale * k2;
            glow.setShader(new RadialGradient(fx, fy, radius, new int[] { 0x808154F0, 0x008154F0 }, null, Shader.TileMode.CLAMP));
            canvas.save();
            canvas.scale(1, 0.3f, fx, fy);
            canvas.drawCircle(fx, fy, radius, glow);
            canvas.restore();
        }
        frame.setPixels(color, 0, bufW, 0, 0, bufW, bufH);
        canvas.drawBitmap(frame, new Rect(0, 0, bufW, bufH), new Rect(0, 0, bufW * k, bufH * k), pixels);

        // the glow of the antenna light
        if (!blink) {
            float shine = 0.7f + 0.3f * (float) Math.sin(time * (busy ? 6 : 2.2));
            for (float[] e : m.eyes) {
                float[] p = project(mats[HEAD], e);
                float r = scale * k2 * 2.2f;
                glow.setShader(new RadialGradient(p[0] * k2, p[1] * k2, r, new int[] { ((int) (140 * shine) << 24) | 0x8348FF, 0x008348FF }, null,
                                                  Shader.TileMode.CLAMP));
                canvas.drawCircle(p[0] * k2, p[1] * k2, r, glow);
            }
        }
        // the screen on his chest glows
        for (float[] g : m.glows) {
            float[] at = project(mats[(int) g[0]], new float[] { g[1], g[2], g[3] });
            float gr = scale * k2 * g[4] * (busy ? 1.4f : 1f) * (0.85f + 0.15f * (float) Math.sin(time * 3));
            glow.setShader(new RadialGradient(at[0] * k2, at[1] * k2, gr, new int[] { 0x708348FF, 0x008348FF }, null, Shader.TileMode.CLAMP));
            canvas.drawCircle(at[0] * k2, at[1] * k2, gr, glow);
        }

        // the tesseract over the raised hand, turning in four dimensions
        float cx = (centerX + tesseract[0] * scale) * k2;
        float cy = (centerY - tesseract[1] * scale) * k2 + (float) Math.sin(time * 2) * scale * k2 * 0.4f;
        float glowRadius = scale * k2 * 7 * (busy ? 1.3f : 1f);
        glow.setShader(new RadialGradient(cx, cy, glowRadius, new int[] { ((int) (110 * pulse) << 24) | 0x8648FF, 0x008648FF }, null,
                                          Shader.TileMode.CLAMP));
        canvas.drawCircle(cx, cy, glowRadius, glow);
        float size = scale * k2 * 15;
        float turn = time * (busy ? 1.6f : 0.7f);
        for (int i = 0; i < 16; i++) {
            float x = (i & 1) != 0 ? 1 : -1, y = (i & 2) != 0 ? 1 : -1, z = (i & 4) != 0 ? 1 : -1, ww = (i & 8) != 0 ? 1 : -1;
            double a = turn, b = turn * 0.7;
            float x1 = (float) (x * Math.cos(a) - ww * Math.sin(a)), w1 = (float) (x * Math.sin(a) + ww * Math.cos(a));
            float y1 = (float) (y * Math.cos(b) - z * Math.sin(b)), z1 = (float) (y * Math.sin(b) + z * Math.cos(b));
            float x2 = (float) (x1 * Math.cos(0.5) - z1 * Math.sin(0.5)), z2 = (float) (x1 * Math.sin(0.5) + z1 * Math.cos(0.5));
            float k4 = 1 / (3 - w1);
            float k3 = 1 / (4 - z2 * k4);
            tx[i] = cx + x2 * k4 * k3 * size;
            ty[i] = cy + y1 * k4 * k3 * size;
        }
        line.setStrokeWidth(Math.max(1, scale * k2 * 0.45f));
        line.setAlpha((int) (200 + 55 * pulse));
        for (int i = 0; i < 16; i++) {
            for (int j2 = 0; j2 < 4; j2++) {
                int j = i ^ (1 << j2);
                if (i < j) {
                    canvas.drawLine(tx[i], ty[i], tx[j], ty[j], line);
                }
            }
        }
        float r = Math.max(1, scale * k2 * 0.5f);
        for (int i = 0; i < 16; i++) {
            canvas.drawCircle(tx[i], ty[i], r, dot);
        }
        // sparks rise from the cube and fade: violet and blue
        int count = busy ? 12 : 7;
        float period = busy ? 1.4f : 2.4f;
        for (int i = 0; i < count; i++) {
            float t = ((time + i * period / count) % period) / period;
            float sx = cx + (float) Math.sin(i * 12.9 + t * 3) * scale * k2 * 4;
            float sy = cy - scale * k2 * 2 - t * scale * k2 * 11;
            spark.setColor(i % 3 == 0 ? 0xFF7CD0FF : 0xFF9A62FF);
            spark.setAlpha((int) (255 * (1 - t)));
            float s = scale * k2 * (t < 0.5f ? 0.8f : 0.55f);
            canvas.drawRect(sx, sy, sx + s, sy + s, spark);
        }
        postInvalidateDelayed(33);
    }

    /** one dot of the hover ring, with the depth test (his legs hide the back of the ring) */
    private void runeDot(float[] m, float x, float y, float z, int c) {
        float[] p = project(m, new float[] { x, y, z });
        int size = Math.max(1, Math.round(scale * 0.7f));
        int x0 = Math.round(p[0] - size / 2f), y0 = Math.round(p[1] - size / 2f);
        for (int j = 0; j < size; j++) {
            for (int i = 0; i < size; i++) {
                int sx = x0 + i, sy = y0 + j;
                if (sx < 0 || sy < 0 || sx >= bufW || sy >= bufH) {
                    continue;
                }
                int idx = sy * bufW + sx;
                if (p[2] > depth[idx]) {
                    color[idx] = c;
                    depth[idx] = p[2];
                }
            }
        }
    }

    /** a box with the Minecraft texture layout at (u, v): top and bottom on the first row, then right, front, left, back */
    private void drawBox(float[] m, float x, float y, float z, float w, float h, float d, int u, int v, float inflate, boolean head) {
        float x0 = x - inflate, x1 = x + w + inflate, y0 = y - inflate, y1 = y + h + inflate, z0 = z - inflate, z1 = z + d + inflate;
        int iw = Math.round(w), ih = Math.round(h), id = Math.round(d);
        face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, u + id, v + id, iw, ih, head);
        face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, u + 2 * id + iw, v + id, iw, ih, head);
        face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, u, v + id, id, ih, head);
        face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, u + id + iw, v + id, id, ih, head);
        face(m, x0, y1, z0, x1, y1, z0, x0, y1, z1, u + id, v, iw, id, head);
        face(m, x0, y0, z1, x1, y0, z1, x0, y0, z0, u + id + iw, v, iw, id, head);
    }

    /**
     * one face, a textured parallelogram, shaded like Minecraft and lit violet by the cube when it is near and turned to
     * it; the pixels with alpha 254 glow (not shaded), those of the head (the antenna light) go dark when he "blinks"
     */
    private void face(float[] m, float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, int su, int sv, int sw, int sh,
                      boolean head) {
        float AX = m[0] * ax + m[1] * ay + m[2] * az + m[3], AY = m[4] * ax + m[5] * ay + m[6] * az + m[7], AZ = m[8] * ax + m[9] * ay + m[10] * az + m[11];
        float BX = m[0] * bx + m[1] * by + m[2] * bz + m[3], BY = m[4] * bx + m[5] * by + m[6] * bz + m[7], BZ = m[8] * bx + m[9] * by + m[10] * bz + m[11];
        float CX = m[0] * cx + m[1] * cy + m[2] * cz + m[3], CY = m[4] * cx + m[5] * cy + m[6] * cz + m[7], CZ = m[8] * cx + m[9] * cy + m[10] * cz + m[11];
        float ex = BX - AX, ey = BY - AY, ez = BZ - AZ, fx = CX - AX, fy = CY - AY, fz = CZ - AZ;
        float nx = fy * ez - fz * ey, ny = fz * ex - fx * ez, nz = fx * ey - fy * ex;
        float len = (float) Math.sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-6f || nz / len <= 0.001f) {
            return;
        }
        float light = (nx * -0.35f + ny * 0.8f + nz * 0.5f) / len / 0.995f;
        float bright = 0.56f + 0.46f * Math.max(0, light);
        if (ny / len < -0.5f) {
            bright *= 0.85f;
        }
        // the violet light of the cube: more blue than red
        float dx = tesseract[0] - (AX + (ex + fx) / 2), dy = tesseract[1] - (AY + (ey + fy) / 2), dz = tesseract[2] - (AZ + (ez + fz) / 2);
        float dist = (float) Math.sqrt(dx * dx + dy * dy + dz * dz) + 1e-3f;
        float lit = Math.max(0, (nx * dx + ny * dy + nz * dz) / (len * dist)) * Math.max(0, Math.min(1, 1.1f - dist / 13)) * tesseractLight;
        int b256 = Math.round(Math.min(1.08f, bright) * 256);
        int r256 = Math.round(Math.min(1.3f, bright + 0.35f * lit) * 256);
        int v256 = Math.round(Math.min(1.4f, bright + 0.7f * lit) * 256);
        int add = Math.round(lit * 22);
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
        int[] tex = model.tex;
        int tw = model.tw, th = model.th;
        for (int py = y0; py <= y1; py++) {
            float ry = py + 0.5f - sAy;
            float rx = x0 + 0.5f - sAx;
            float s = rx * dsdx + ry * dsdy, t = rx * dtdx + ry * dtdy;
            int row = py * bufW;
            for (int px = x0; px <= x1; px++, s += dsdx, t += dtdx) {
                if (s < -0.02f || s >= 1.02f || t < -0.02f || t >= 1.02f) {
                    continue;
                }
                float z = AZ + s * dz1 + t * dz2;
                int i = row + px;
                if (z <= depth[i]) {
                    continue;
                }
                int ux = su + (int) (Math.max(0, Math.min(0.999f, s)) * sw), vy = sv + (int) (Math.max(0, Math.min(0.999f, t)) * sh);
                if (ux < 0 || vy < 0 || ux >= tw || vy >= th) {
                    continue;
                }
                int c = tex[vy * tw + ux];
                int alpha = c >>> 24;
                if (alpha < 128) {
                    continue;
                }
                if (alpha == 254) {
                    // a glowing pixel: the antenna light (dark for a moment now and then), the screen
                    color[i] = blink && head ? 0xFF040512 : 0xFF000000 | (c & 0xFFFFFF);
                } else {
                    int r = Math.min(255, ((((c >> 16) & 0xFF) * r256) >> 8) + add / 2);
                    int g = Math.min(255, ((((c >> 8) & 0xFF) * b256) >> 8) + add / 8);
                    int bl = Math.min(255, (((c & 0xFF) * v256) >> 8) + add);
                    color[i] = 0xFF000000 | (r << 16) | (g << 8) | bl;
                }
                depth[i] = z;
            }
        }
    }
}
