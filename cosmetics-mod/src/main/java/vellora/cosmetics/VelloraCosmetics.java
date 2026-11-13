// SPDX-License-Identifier: GPL-3.0-only
package vellora.cosmetics;

import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Nyxora Cosmetics: the player's look from the Nyxora Launcher in the game. The launcher writes it into
 * config/vellora-cosmetics/ before every start: look.json (the accessories on the head, body and back as coloured
 * boxes on the parts of the player model, whether the skin is slim) with skin.png and cape.png. The mod shows them on
 * the player of this game only (the launcher has no server to share them): in third person, in the inventory, in mirrors
 * of other mods. Pets, things in the hand and effects stay in the launcher.
 */
public final class VelloraCosmetics implements ClientModInitializer {
    public static final String MOD_ID = "vellora_cosmetics";
    static final Logger LOG = LoggerFactory.getLogger("Nyxora Cosmetics");
    private static Look look = Look.NONE;

    @Override
    public void onInitializeClient() {
        look = Look.load(FabricLoader.getInstance().getConfigDir().resolve("vellora-cosmetics"));
    }

    public static Look look() {
        return look;
    }

    /** the look is the player's own: it is shown on this game's player only */
    public static boolean isOwn(AbstractClientPlayerEntity player) {
        return player != null && player == MinecraftClient.getInstance().player;
    }
}
