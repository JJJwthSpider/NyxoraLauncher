package com.sculkshoot.client;

import java.nio.charset.StandardCharsets;

/**
 * A small QR code encoder (byte mode, error correction level M, versions 1-10): enough for the sign-in link
 * "https://www.microsoft.com/link?otc=CODE" the phone camera opens. Follows the QR code standard (ISO/IEC 18004)
 * the way Project Nayuki's MIT-licensed reference encoder lays it out. Plain Java, no Android classes.
 */
final class QrCode {
    // error correction level M, index = version (0 unused)
    private static final int[] ECC_PER_BLOCK = { -1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26 };
    private static final int[] BLOCKS = { -1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5 };
    private static final int FORMAT_M = 0;

    final int size;
    private final boolean[][] modules;   // [y][x], true = dark
    private final boolean[][] function;  // finder, timing, alignment, format and version modules

    /** the dark modules: [y][x] */
    boolean dark(int x, int y) {
        return modules[y][x];
    }

    static QrCode encode(String text) {
        byte[] data = text.getBytes(StandardCharsets.UTF_8);
        for (int version = 1; version <= 10; version++) {
            int capacityBits = dataCodewords(version) * 8;
            int countBits = version <= 9 ? 8 : 16;
            if (4 + countBits + data.length * 8 <= capacityBits) {
                return new QrCode(version, data);
            }
        }
        throw new IllegalArgumentException("text too long for a QR code: " + data.length + " bytes");
    }

    private QrCode(int version, byte[] data) {
        size = version * 4 + 17;
        modules = new boolean[size][size];
        function = new boolean[size][size];
        drawFunctionPatterns(version);
        byte[] codewords = addEccAndInterleave(version, dataCodewords(version, data));
        drawCodewords(codewords);
        // the mask that gives the most readable code
        int best = 0;
        int bestPenalty = Integer.MAX_VALUE;
        for (int mask = 0; mask < 8; mask++) {
            applyMask(mask);
            drawFormatBits(mask);
            int penalty = penalty();
            if (penalty < bestPenalty) {
                best = mask;
                bestPenalty = penalty;
            }
            applyMask(mask);  // undo
        }
        applyMask(best);
        drawFormatBits(best);
    }

    /* ------------------------------------------------------------ data */

    private static int rawDataModules(int ver) {
        int result = (16 * ver + 128) * ver + 64;
        if (ver >= 2) {
            int numAlign = ver / 7 + 2;
            result -= (25 * numAlign - 10) * numAlign - 55;
            if (ver >= 7) {
                result -= 36;
            }
        }
        return result;
    }

    private static int dataCodewords(int ver) {
        return rawDataModules(ver) / 8 - ECC_PER_BLOCK[ver] * BLOCKS[ver];
    }

    /** byte mode segment, terminator and padding */
    private static byte[] dataCodewords(int version, byte[] data) {
        int capacity = dataCodewords(version) * 8;
        BitBuffer bits = new BitBuffer();
        bits.append(4, 4);  // byte mode
        bits.append(data.length, version <= 9 ? 8 : 16);
        for (byte b : data) {
            bits.append(b & 0xFF, 8);
        }
        bits.append(0, Math.min(4, capacity - bits.length));
        bits.append(0, (8 - bits.length % 8) % 8);
        for (int pad = 0xEC; bits.length < capacity; pad ^= 0xEC ^ 0x11) {
            bits.append(pad, 8);
        }
        return bits.toBytes();
    }

    private static final class BitBuffer {
        boolean[] bits = new boolean[64];
        int length;

        void append(int value, int count) {
            for (int i = count - 1; i >= 0; i--) {
                if (length == bits.length) {
                    bits = java.util.Arrays.copyOf(bits, bits.length * 2);
                }
                bits[length++] = ((value >>> i) & 1) != 0;
            }
        }

        byte[] toBytes() {
            byte[] out = new byte[length / 8];
            for (int i = 0; i < length; i++) {
                if (bits[i]) {
                    out[i >>> 3] |= 1 << (7 - (i & 7));
                }
            }
            return out;
        }
    }

    private static byte[] addEccAndInterleave(int version, byte[] data) {
        int numBlocks = BLOCKS[version];
        int eccLen = ECC_PER_BLOCK[version];
        int rawCodewords = rawDataModules(version) / 8;
        int numShortBlocks = numBlocks - rawCodewords % numBlocks;
        int shortBlockLen = rawCodewords / numBlocks;
        byte[][] blocks = new byte[numBlocks][];
        byte[] divisor = reedSolomonDivisor(eccLen);
        for (int i = 0, k = 0; i < numBlocks; i++) {
            int datLen = shortBlockLen - eccLen + (i < numShortBlocks ? 0 : 1);
            byte[] dat = java.util.Arrays.copyOfRange(data, k, k + datLen);
            k += datLen;
            byte[] block = java.util.Arrays.copyOf(dat, shortBlockLen + 1);
            byte[] ecc = reedSolomonRemainder(dat, divisor);
            System.arraycopy(ecc, 0, block, block.length - eccLen, eccLen);
            blocks[i] = block;
        }
        byte[] result = new byte[rawCodewords];
        int n = 0;
        for (int i = 0; i < blocks[0].length; i++) {
            for (int j = 0; j < blocks.length; j++) {
                // short blocks have a placeholder byte where long blocks have their last data byte
                if (i != shortBlockLen - eccLen || j >= numShortBlocks) {
                    result[n++] = blocks[j][i];
                }
            }
        }
        return result;
    }

    private static byte[] reedSolomonDivisor(int degree) {
        byte[] result = new byte[degree];
        result[degree - 1] = 1;
        int root = 1;
        for (int i = 0; i < degree; i++) {
            for (int j = 0; j < result.length; j++) {
                result[j] = (byte) multiply(result[j] & 0xFF, root);
                if (j + 1 < result.length) {
                    result[j] ^= result[j + 1];
                }
            }
            root = multiply(root, 0x02);
        }
        return result;
    }

    private static byte[] reedSolomonRemainder(byte[] data, byte[] divisor) {
        byte[] result = new byte[divisor.length];
        for (byte b : data) {
            int factor = (b ^ result[0]) & 0xFF;
            System.arraycopy(result, 1, result, 0, result.length - 1);
            result[result.length - 1] = 0;
            for (int i = 0; i < result.length; i++) {
                result[i] ^= (byte) multiply(divisor[i] & 0xFF, factor);
            }
        }
        return result;
    }

    /** multiplication in GF(2^8) modulo x^8 + x^4 + x^3 + x^2 + 1 */
    private static int multiply(int x, int y) {
        int z = 0;
        for (int i = 7; i >= 0; i--) {
            z = (z << 1) ^ ((z >>> 7) * 0x11D);
            z ^= ((y >>> i) & 1) * x;
        }
        return z;
    }

    /* ------------------------------------------------------------ layout */

    private void set(int x, int y, boolean dark) {
        modules[y][x] = dark;
        function[y][x] = true;
    }

    private void drawFunctionPatterns(int version) {
        for (int i = 0; i < size; i++) {
            set(6, i, i % 2 == 0);
            set(i, 6, i % 2 == 0);
        }
        drawFinder(3, 3);
        drawFinder(size - 4, 3);
        drawFinder(3, size - 4);
        int[] align = alignmentPositions(version);
        int n = align.length;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (!(i == 0 && j == 0 || i == 0 && j == n - 1 || i == n - 1 && j == 0)) {
                    drawAlignment(align[i], align[j]);
                }
            }
        }
        drawFormatBits(0);  // reserves the format area; the real bits come with the mask
        if (version >= 7) {
            int rem = version;
            for (int i = 0; i < 12; i++) {
                rem = (rem << 1) ^ ((rem >>> 11) * 0x1F25);
            }
            int bits = version << 12 | rem;
            for (int i = 0; i < 18; i++) {
                boolean bit = ((bits >>> i) & 1) != 0;
                int a = size - 11 + i % 3;
                int b = i / 3;
                set(a, b, bit);
                set(b, a, bit);
            }
        }
    }

    private void drawFinder(int x, int y) {
        for (int dy = -4; dy <= 4; dy++) {
            for (int dx = -4; dx <= 4; dx++) {
                int dist = Math.max(Math.abs(dx), Math.abs(dy));
                int xx = x + dx;
                int yy = y + dy;
                if (xx >= 0 && xx < size && yy >= 0 && yy < size) {
                    set(xx, yy, dist != 2 && dist != 4);
                }
            }
        }
    }

    private void drawAlignment(int x, int y) {
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                set(x + dx, y + dy, Math.max(Math.abs(dx), Math.abs(dy)) != 1);
            }
        }
    }

    private static int[] alignmentPositions(int version) {
        if (version == 1) {
            return new int[0];
        }
        int numAlign = version / 7 + 2;
        int step = (version * 4 + numAlign * 2 + 1) / (numAlign * 2 - 2) * 2;
        int[] result = new int[numAlign];
        result[0] = 6;
        for (int i = result.length - 1, pos = version * 4 + 10; i >= 1; i--, pos -= step) {
            result[i] = pos;
        }
        return result;
    }

    private void drawFormatBits(int mask) {
        int data = FORMAT_M << 3 | mask;
        int rem = data;
        for (int i = 0; i < 10; i++) {
            rem = (rem << 1) ^ ((rem >>> 9) * 0x537);
        }
        int bits = (data << 10 | rem) ^ 0x5412;
        for (int i = 0; i <= 5; i++) {
            set(8, i, bit(bits, i));
        }
        set(8, 7, bit(bits, 6));
        set(8, 8, bit(bits, 7));
        set(7, 8, bit(bits, 8));
        for (int i = 9; i < 15; i++) {
            set(14 - i, 8, bit(bits, i));
        }
        for (int i = 0; i < 8; i++) {
            set(size - 1 - i, 8, bit(bits, i));
        }
        for (int i = 8; i < 15; i++) {
            set(8, size - 15 + i, bit(bits, i));
        }
        set(8, size - 8, true);  // always dark
    }

    private static boolean bit(int x, int i) {
        return ((x >>> i) & 1) != 0;
    }

    private void drawCodewords(byte[] data) {
        int i = 0;
        for (int right = size - 1; right >= 1; right -= 2) {
            if (right == 6) {
                right = 5;  // the vertical timing line
            }
            for (int vert = 0; vert < size; vert++) {
                for (int j = 0; j < 2; j++) {
                    int x = right - j;
                    boolean upward = ((right + 1) & 2) == 0;
                    int y = upward ? size - 1 - vert : vert;
                    if (!function[y][x] && i < data.length * 8) {
                        modules[y][x] = bit(data[i >>> 3], 7 - (i & 7));
                        i++;
                    }
                }
            }
        }
    }

    private void applyMask(int mask) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                boolean invert;
                switch (mask) {
                    case 0: invert = (x + y) % 2 == 0; break;
                    case 1: invert = y % 2 == 0; break;
                    case 2: invert = x % 3 == 0; break;
                    case 3: invert = (x + y) % 3 == 0; break;
                    case 4: invert = (x / 3 + y / 2) % 2 == 0; break;
                    case 5: invert = x * y % 2 + x * y % 3 == 0; break;
                    case 6: invert = (x * y % 2 + x * y % 3) % 2 == 0; break;
                    default: invert = ((x + y) % 2 + x * y % 3) % 2 == 0; break;
                }
                if (invert && !function[y][x]) {
                    modules[y][x] = !modules[y][x];
                }
            }
        }
    }

    /** a simplified version of the standard's penalty: long runs, 2x2 blocks and the dark/light balance */
    private int penalty() {
        int result = 0;
        for (int a = 0; a < size; a++) {
            int runRow = 1;
            int runCol = 1;
            for (int b = 1; b < size; b++) {
                if (modules[a][b] == modules[a][b - 1]) {
                    runRow++;
                } else {
                    result += runRow >= 5 ? runRow - 2 : 0;
                    runRow = 1;
                }
                if (modules[b][a] == modules[b - 1][a]) {
                    runCol++;
                } else {
                    result += runCol >= 5 ? runCol - 2 : 0;
                    runCol = 1;
                }
            }
            result += (runRow >= 5 ? runRow - 2 : 0) + (runCol >= 5 ? runCol - 2 : 0);
        }
        int dark = 0;
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                dark += modules[y][x] ? 1 : 0;
                if (x + 1 < size && y + 1 < size && modules[y][x] == modules[y][x + 1] && modules[y][x] == modules[y + 1][x]
                    && modules[y][x] == modules[y + 1][x + 1]) {
                    result += 3;
                }
            }
        }
        int total = size * size;
        result += (Math.abs(dark * 20 - total * 10) + total - 1) / total * 10;
        return result;
    }
}
