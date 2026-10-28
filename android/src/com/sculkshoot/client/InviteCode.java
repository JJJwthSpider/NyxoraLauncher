package com.sculkshoot.client;

import java.io.ByteArrayOutputStream;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Locale;

/** Invite codes of the desktop launcher's "My server" ("SC-XXXX-XXXX-..."): decodes them into an address and a port. */
final class InviteCode {
    private InviteCode() {}

    private static final String ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    private static final String BORE_HOST = "bore.pub";
    private static final int IPV4 = 1, BORE = 2, HOST_NAME = 3;

    static final class Address {
        final String host;
        final int port;

        Address(String host, int port) {
            this.host = host;
            this.port = port;
        }
    }

    private static int check(byte[] bytes, int length) {
        int sum = 0x5A;
        for (int i = 0; i < length; i++) {
            sum = (((sum << 1) | (sum >> 7)) & 0xFF) ^ (bytes[i] & 0xFF);
        }
        return sum & 0xFF;
    }

    private static byte[] fromBase32(String text) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        long buffer = 0;
        int bits = 0;
        for (char ch : text.toCharArray()) {
            ch = Character.toUpperCase(ch);
            if (ch == 'O') {
                ch = '0';
            } else if (ch == 'I' || ch == 'L') {
                ch = '1';
            }
            int value = ALPHABET.indexOf(ch);
            if (value < 0) {
                return null;
            }
            buffer = ((buffer << 5) | value) & 0xFFFFFFFFL;
            bits += 5;
            if (bits >= 8) {
                out.write((int) ((buffer >> (bits - 8)) & 0xFF));
                bits -= 8;
            }
        }
        return out.toByteArray();
    }

    /** The address in an invite code, or null if the code is not valid. */
    static Address decode(String code) {
        String text = code.trim().toUpperCase(Locale.ROOT);
        if (text.startsWith("SC-") || text.startsWith("SC ")) {
            text = text.substring(3);
        }
        text = text.replace("-", "").replace(" ", "");
        byte[] bytes = text.length() < 4 ? null : fromBase32(text);
        if (bytes == null || bytes.length < 4) {
            return null;
        }
        // base32 may add a few padding bits that make an extra zero byte at the end
        int length = bytes.length;
        while (length >= 4 && check(bytes, length - 1) != (bytes[length - 1] & 0xFF) && bytes[length - 1] == 0) {
            length--;
        }
        if (length < 4 || check(bytes, length - 1) != (bytes[length - 1] & 0xFF)) {
            return null;
        }
        bytes = Arrays.copyOf(bytes, length - 1);

        String host;
        int at;
        switch (bytes[0]) {
            case BORE:
                host = BORE_HOST;
                at = 1;
                break;
            case IPV4:
                if (bytes.length < 7) {
                    return null;
                }
                host = (bytes[1] & 0xFF) + "." + (bytes[2] & 0xFF) + "." + (bytes[3] & 0xFF) + "." + (bytes[4] & 0xFF);
                at = 5;
                break;
            case HOST_NAME: {
                int nameLength = bytes[1] & 0xFF;
                if (bytes.length < 2 + nameLength + 2) {
                    return null;
                }
                host = new String(bytes, 2, nameLength, Charset.forName("UTF-8"));
                at = 2 + nameLength;
                break;
            }
            default:
                return null;
        }
        if (bytes.length != at + 2) {
            return null;
        }
        int port = ((bytes[at] & 0xFF) << 8) | (bytes[at + 1] & 0xFF);
        return port == 0 ? null : new Address(host, port);
    }
}
