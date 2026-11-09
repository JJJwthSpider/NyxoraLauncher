// SPDX-License-Identifier: GPL-3.0-only
package vellora.cosmetics.mixin;

import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.util.Identifier;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import vellora.cosmetics.Look;
import vellora.cosmetics.VelloraCosmetics;

/** The skin and the cape of the launcher on the player of this game */
@Mixin(AbstractClientPlayerEntity.class)
abstract class AbstractClientPlayerEntityMixin {
    @Inject(method = "getSkinTexture", at = @At("HEAD"), cancellable = true)
    private void vellora$skin(CallbackInfoReturnable<Identifier> result) {
        if (VelloraCosmetics.isOwn((AbstractClientPlayerEntity) (Object) this)) {
            Identifier skin = VelloraCosmetics.look().skin();
            if (skin != null) {
                result.setReturnValue(skin);
            }
        }
    }

    @Inject(method = "getModel", at = @At("HEAD"), cancellable = true)
    private void vellora$model(CallbackInfoReturnable<String> result) {
        Look look = VelloraCosmetics.look();
        if (look.hasSkin() && VelloraCosmetics.isOwn((AbstractClientPlayerEntity) (Object) this)) {
            result.setReturnValue(look.slim() ? "slim" : "default");
        }
    }

    @Inject(method = "getCapeTexture", at = @At("HEAD"), cancellable = true)
    private void vellora$cape(CallbackInfoReturnable<Identifier> result) {
        if (VelloraCosmetics.isOwn((AbstractClientPlayerEntity) (Object) this)) {
            Identifier cape = VelloraCosmetics.look().cape();
            if (cape != null) {
                result.setReturnValue(cape);
            }
        }
    }

    @Inject(method = "canRenderCapeTexture", at = @At("HEAD"), cancellable = true)
    private void vellora$canRenderCape(CallbackInfoReturnable<Boolean> result) {
        if (VelloraCosmetics.isOwn((AbstractClientPlayerEntity) (Object) this) && VelloraCosmetics.look().cape() != null) {
            result.setReturnValue(true);
        }
    }
}
