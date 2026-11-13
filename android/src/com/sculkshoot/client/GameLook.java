package com.sculkshoot.client;

import android.graphics.Bitmap;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.Arrays;

/**
 * The player's look in the game: the Nyxora Cosmetics mod (cosmetics-mod/ in the repository, Fabric) shows the skin, the
 * cape and the accessories of the head, body and back slots from Nyxora on the player of the game; pets, things in the
 * hand and effects stay in Nyxora. Before every start the mod is put into a Fabric or Quilt pack of Minecraft 1.20.1 and
 * the look is written into its config/vellora-cosmetics/; from any other pack the mod is taken out (Fabric would refuse
 * to start with it). The same as the PC launcher does (VelloraLook).
 */
final class GameLook {
    static final String MINECRAFT = "1.20.1";
    static final String MOD_FILE = "vellora-cosmetics-1.20.1.jar";

    private GameLook() {
    }

    static boolean fits(Installation inst) {
        return MINECRAFT.equals(inst.minecraftVersion) && ("fabric".equals(inst.loader) || "quilt".equals(inst.loader));
    }

    /** before a start: the mod and the look in the pack's game folder, or the mod out of a pack it does not fit */
    static void prepare(MainActivity activity, Installation inst, File gameDir) {
        File jar = new File(new File(gameDir, "mods"), MOD_FILE);
        if (!fits(inst)) {
            if (jar.isFile()) {
                jar.delete();
            }
            return;
        }
        try {
            byte[] mod = read(activity.getAssets().open(MOD_FILE));
            if (!jar.isFile() || !Arrays.equals(mod, read(new FileInputStream(jar)))) {
                jar.getParentFile().mkdirs();
                FileOutputStream out = new FileOutputStream(jar);
                try {
                    out.write(mod);
                } finally {
                    out.close();
                }
            }
            Account account = activity.account();
            Bitmap skin = null, cape = SkinCache.generatedCape(activity);
            boolean slim = false;
            if (account != null && account.guest) {
                // a guest's own look, made from the nickname (the same as in the lobby)
                SkinGenerator.Skin own = SkinGenerator.skins("", 1, account.name.hashCode()).get(0);
                skin = own.bitmap;
                slim = own.slim;
            } else if (account != null) {
                skin = SkinCache.skin(activity, account);
                slim = "SLIM".equalsIgnoreCase(account.skinVariant);
                if (cape == null) {
                    cape = SkinCache.cape(activity, account);
                }
            }
            CharacterView.exportLook(activity, new File(gameDir, "config/vellora-cosmetics"), skin, slim, cape,
                                     Cosmetics.worn(activity, Cosmetics.HEAD), Cosmetics.worn(activity, Cosmetics.BODY),
                                     Cosmetics.worn(activity, Cosmetics.BACK));
        } catch (Exception e) {
            Log.w("Nyxora", "The look for the game was not written: " + e);
        }
    }

    private static byte[] read(InputStream in) throws java.io.IOException {
        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            byte[] chunk = new byte[16384];
            for (int n; (n = in.read(chunk)) > 0;) {
                bytes.write(chunk, 0, n);
            }
            return bytes.toByteArray();
        } finally {
            in.close();
        }
    }
}
