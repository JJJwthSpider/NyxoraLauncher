package com.sculkshoot.client;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;

/**
 * A button's background with a small pixel picture in the middle, like the white pixel icons of Millida's lobby (the
 * speaker, the note, the chest, the chat, the gamepad, the gear, two friends). '#' is white, 'r' red. The background
 * (the block of the button) is drawn first.
 */
final class PixelGlyph extends Drawable {
    static final String[] SPEAKER = { "....#.......", "...##..#....", "..###...#...", "#####.#..#..", "#####..#.#..", "#####..#.#..",
                                      "#####.#..#..", "..###...#...", "...##..#....", "....#......." };
    static final String[] SPEAKER_OFF = { "....#.......", "...##.......", "..###.r...r.", "#####..r.r..", "#####...r...", "#####..r.r..",
                                          "#####.r...r.", "..###.......", "...##.......", "....#......." };
    static final String[] NOTE = { "....#####...", "....#...#...", "....#...#...", "....#...#...", "....#...#...", "..###.###...", ".####.###...",
                                   "..##...#...." };
    static final String[] NOTE_OFF = { "r...#####...", ".r..#...#...", "..r.#...#...", "...r#...#...", "....r...#...", "..###r###...",
                                       ".####.r##...", "..##...r...." };
    static final String[] CHEST = { "..########..", ".#........#.", "############", "#....##....#", "#....rr....#", "############", "#..........#",
                                    "#..........#", "############" };
    static final String[] CHAT = { "############", "#..........#", "#.##.##.##.#", "#..........#", "############", "..##........", ".#.........." };
    static final String[] PAD = { "..########..", ".#........#.", "#..#....r..#", "#.###..r.r.#", "#..#....r..#", "#..........#", ".###....###." };
    static final String[] GEAR = { "....##....", ".#.####.#.", "..######..", ".###..###.", "###....###", "###....###", ".###..###.", "..######..",
                                   ".#.####.#.", "....##...." };
    static final String[] PEOPLE = { "...##.......", "..####..##..", "..####.####.", "...##..####.", "..####..##..", ".######.###.", ".######.####",
                                     ".######.####" };

    private final Drawable background;
    private String[] rows;
    private final float density;
    private final Paint paint = new Paint();

    PixelGlyph(Drawable background, String[] rows, float density) {
        this.background = background;
        this.rows = rows;
        this.density = density;
    }

    void setRows(String[] rows) {
        this.rows = rows;
        invalidateSelf();
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        if (background != null) {
            background.setBounds(bounds);
        }
    }

    @Override
    public void draw(Canvas canvas) {
        if (background != null) {
            background.draw(canvas);
        }
        Rect b = getBounds();
        int w = 0;
        for (String r : rows) {
            w = Math.max(w, r.length());
        }
        float u = Math.max(1, Math.round(2 * density));
        float ox = b.left + (b.width() - w * u) / 2f, oy = b.top + (b.height() - rows.length * u) / 2f - density;  // a little up: the block's lip
        for (int y = 0; y < rows.length; y++) {
            for (int x = 0; x < rows[y].length(); x++) {
                char c = rows[y].charAt(x);
                if (c == '#' || c == 'r') {
                    paint.setColor(c == '#' ? 0xFFFBEFF1 : 0xFFFF4654);
                    canvas.drawRect(ox + x * u, oy + y * u, ox + (x + 1) * u, oy + (y + 1) * u, paint);
                }
            }
        }
    }

    @Override
    public void setAlpha(int alpha) {
        paint.setAlpha(alpha);
    }

    @Override
    public void setColorFilter(ColorFilter filter) {
        paint.setColorFilter(filter);
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }
}
