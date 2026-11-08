package com.sculkshoot.client;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

/** Children in rows, a new row when one is full: the version grid and the chips of the "Создать" tab, like Millida's. */
final class FlowLayout extends ViewGroup {
    private final int gap;

    FlowLayout(Context context, int gap) {
        super(context);
        this.gap = gap;
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        int width = MeasureSpec.getSize(widthSpec) - getPaddingLeft() - getPaddingRight();
        int x = 0, y = 0, row = 0;
        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            if (child.getVisibility() == GONE) {
                continue;
            }
            LayoutParams lp = child.getLayoutParams();
            int cw = lp.width > 0 ? MeasureSpec.makeMeasureSpec(lp.width, MeasureSpec.EXACTLY)
                                  : MeasureSpec.makeMeasureSpec(Math.max(0, width), MeasureSpec.AT_MOST);
            int ch = lp.height > 0 ? MeasureSpec.makeMeasureSpec(lp.height, MeasureSpec.EXACTLY) : MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
            child.measure(cw, ch);
            if (x > 0 && x + child.getMeasuredWidth() > width) {
                x = 0;
                y += row + gap;
                row = 0;
            }
            x += child.getMeasuredWidth() + gap;
            row = Math.max(row, child.getMeasuredHeight());
        }
        int height = y + row + getPaddingTop() + getPaddingBottom();
        setMeasuredDimension(MeasureSpec.getSize(widthSpec), resolveSize(height, heightSpec));
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        int width = r - l - getPaddingLeft() - getPaddingRight();
        int x = 0, y = 0, row = 0;
        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            if (child.getVisibility() == GONE) {
                continue;
            }
            int w = child.getMeasuredWidth(), h = child.getMeasuredHeight();
            if (x > 0 && x + w > width) {
                x = 0;
                y += row + gap;
                row = 0;
            }
            child.layout(getPaddingLeft() + x, getPaddingTop() + y, getPaddingLeft() + x + w, getPaddingTop() + y + h);
            x += w + gap;
            row = Math.max(row, h);
        }
    }

    @Override
    protected LayoutParams generateDefaultLayoutParams() {
        return new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    }
}
