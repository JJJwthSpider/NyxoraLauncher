package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * A tile of the lobby, drawn the way Millida Launcher draws its tiles, in Nyxora's burgundy: a dark outline with cut
 * corners, a plate in the colour of the section, hard rays behind the picture, the picture standing on the tile (it may
 * rise above the top edge), and the name on a dense band over the bottom. A "!" or a ribbon can call for attention.
 */
final class HubTile extends FrameLayout {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final int plate, outline, band, ray;
    private final float step;
    private final float over;  // how far the picture rises above the tile
    private final TextView title;
    private final TextView subtitle;
    private TextView ribbon;
    private TextView alert;
    private boolean pressed;

    HubTile(Context context, int plate, View art, String name, String sub) {
        super(context);
        this.plate = plate;
        this.outline = mix(plate, 0xFF000000, 0.7f);
        this.band = mix(plate, 0xFF000000, 0.55f);
        this.ray = (0x55 << 24) | (mix(plate, 0xFFFFFFFF, 0.45f) & 0xFFFFFF);
        float density = context.getResources().getDisplayMetrics().density;
        this.step = Math.max(1, Math.round(2 * density));
        this.over = 14 * density;
        setWillNotDraw(false);
        setClipChildren(false);
        setClipToPadding(false);

        // the picture: pressed to the bottom, a little bigger than the tile, so heads come out over the top edge
        LayoutParams artParams = new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        artParams.topMargin = -Math.round(over);
        artParams.bottomMargin = Math.round(20 * density);
        addView(art, artParams);

        LinearLayout strip = new LinearLayout(context);
        strip.setOrientation(LinearLayout.VERTICAL);
        strip.setPadding(Math.round(8 * density), Math.round(3 * density), Math.round(8 * density), Math.round(5 * density));
        title = Ui.label(context, name, 13, Ui.TEXT, true);
        title.setSingleLine(true);
        title.setEllipsize(android.text.TextUtils.TruncateAt.END);
        strip.addView(title);
        subtitle = Ui.label(context, sub == null ? "" : sub, 10, 0xFFFFC2CA, false);
        subtitle.setSingleLine(true);
        subtitle.setVisibility(sub == null || sub.isEmpty() ? GONE : VISIBLE);
        strip.addView(subtitle);
        LayoutParams stripParams = new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM);
        stripParams.setMargins(Math.round(step * 2), 0, Math.round(step * 2), Math.round(step * 2));
        strip.setBackgroundColor(band);
        addView(strip, stripParams);
        setClickable(true);
        setContentDescription(name);
    }

    /** a small ribbon in the top right corner, like Millida's "Бесплатно" */
    void setRibbon(String text) {
        if (ribbon == null) {
            ribbon = Ui.label(getContext(), "", 9, 0xFFFFFFFF, true);
            ribbon.setPadding(Ui.dp(getContext(), 6), Ui.dp(getContext(), 1), Ui.dp(getContext(), 6), Ui.dp(getContext(), 2));
            ribbon.setBackgroundColor(0xFFE8364F);
            LayoutParams p = new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.TOP | Gravity.END);
            p.setMargins(0, Ui.dp(getContext(), 6), Ui.dp(getContext(), 6), 0);
            addView(ribbon, p);
        }
        ribbon.setText(text);
        ribbon.setVisibility(text == null || text.isEmpty() ? GONE : VISIBLE);
    }

    /** the "!" in the corner: something new here */
    void setAlert(boolean on) {
        if (alert == null) {
            alert = Ui.label(getContext(), "!", 13, 0xFFFFFFFF, true);
            alert.setGravity(Gravity.CENTER);
            alert.setBackground(PixelDrawable.panel(0xFFE8364F, 0xFF1E0408, getResources().getDisplayMetrics().density));
            alert.setPadding(0, 0, 0, 0);
            alert.setIncludeFontPadding(false);
            LayoutParams p = new LayoutParams(Ui.dp(getContext(), 22), Ui.dp(getContext(), 22), Gravity.TOP | Gravity.START);
            p.setMargins(-Ui.dp(getContext(), 6), -Ui.dp(getContext(), 6), 0, 0);
            addView(alert, p);
        }
        alert.setVisibility(on ? VISIBLE : GONE);
    }

    void setSubtitle(String text) {
        subtitle.setText(text);
        subtitle.setVisibility(text == null || text.isEmpty() ? GONE : VISIBLE);
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        boolean now = isPressed();
        if (now != pressed) {
            pressed = now;
            invalidate();
            // like Millida's tiles: the picture jumps a little
            View art = getChildAt(0);
            art.animate().translationY(now ? -Ui.dp(getContext(), 4) : 0).setDuration(90).start();
        }
    }

    /** the picture may rise over the top edge, but stays inside the tile at the sides and the bottom */
    @Override
    protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
        if (child != getChildAt(0)) {
            return super.drawChild(canvas, child, drawingTime);
        }
        canvas.save();
        canvas.clipRect(2 * step, -over * 2, getWidth() - 2 * step, getHeight() - 2 * step);
        boolean more = super.drawChild(canvas, child, drawingTime);
        canvas.restore();
        return more;
    }

    private static int mix(int a, int b, float t) {
        int ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
        int br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
        return 0xFF000000 | (Math.round(ar + (br - ar) * t) << 16) | (Math.round(ag + (bg - ag) * t) << 8) | Math.round(ab + (bb - ab) * t);
    }

    private void notched(Canvas canvas, float l, float t, float r, float b, float cut, int color) {
        paint.setColor(color);
        canvas.drawRect(l + cut, t, r - cut, b, paint);
        canvas.drawRect(l, t + cut, r, b - cut, paint);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        float w = getWidth(), h = getHeight(), s = step;
        notched(canvas, 0, 0, w, h, 2 * s, outline);
        notched(canvas, s, s, w - s, h - s, s, pressed ? mix(plate, 0xFFFFFFFF, 0.12f) : plate);
        // the light top edge and the dark bottom edge, like a block
        paint.setColor(mix(plate, 0xFFFFFFFF, 0.3f));
        canvas.drawRect(2 * s, s, w - 2 * s, 2 * s, paint);
        // hard rays from a point near the top right, cut to the plate
        canvas.save();
        canvas.clipRect(2 * s, 2 * s, w - 2 * s, h - 2 * s);
        float cx = w * 0.62f, cy = h * 0.3f, radius = Math.max(w, h) * 1.3f;
        paint.setColor(ray);
        Path path = new Path();
        for (int i = 0; i < 16; i++) {
            double a0 = Math.toRadians(4 + i * 22.5), a1 = Math.toRadians(4 + i * 22.5 + 9);
            path.reset();
            path.moveTo(cx, cy);
            path.lineTo(cx + (float) Math.cos(a0) * radius, cy + (float) Math.sin(a0) * radius);
            path.lineTo(cx + (float) Math.cos(a1) * radius, cy + (float) Math.sin(a1) * radius);
            path.close();
            canvas.drawPath(path, paint);
        }
        canvas.restore();
    }
}
