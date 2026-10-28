package com.sculkshoot.client;

import java.util.List;

final class Util {
    private Util() {}

    static String join(List<String> items, String separator) {
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < items.size(); i++) {
            if (i > 0) {
                out.append(separator);
            }
            out.append(items.get(i));
        }
        return out.toString();
    }

    static String escape(String text) {
        return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;");
    }
}
