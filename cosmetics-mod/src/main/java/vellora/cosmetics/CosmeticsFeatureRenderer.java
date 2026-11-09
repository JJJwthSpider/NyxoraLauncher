// SPDX-License-Identifier: GPL-3.0-only
package vellora.cosmetics;

import net.minecraft.client.model.ModelPart;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.client.render.OverlayTexture;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.render.entity.feature.FeatureRenderer;
import net.minecraft.client.render.entity.feature.FeatureRendererContext;
import net.minecraft.client.render.entity.model.PlayerEntityModel;
import net.minecraft.client.util.math.MatrixStack;
import org.joml.Matrix3f;
import org.joml.Matrix4f;

import java.util.List;

/** The accessories of the player's look: coloured boxes on the head, the body, the arms and the legs, moving with them */
public final class CosmeticsFeatureRenderer extends FeatureRenderer<AbstractClientPlayerEntity, PlayerEntityModel<AbstractClientPlayerEntity>> {
    public CosmeticsFeatureRenderer(FeatureRendererContext<AbstractClientPlayerEntity, PlayerEntityModel<AbstractClientPlayerEntity>> context) {
        super(context);
    }

    @Override
    public void render(MatrixStack matrices, VertexConsumerProvider vertexConsumers, int light, AbstractClientPlayerEntity player, float limbAngle,
                       float limbDistance, float tickDelta, float animationProgress, float headYaw, float headPitch) {
        Look look = VelloraCosmetics.look();
        if (!look.hasBoxes() || !VelloraCosmetics.isOwn(player) || player.isInvisible()) {
            return;
        }
        VertexConsumer buffer = vertexConsumers.getBuffer(RenderLayer.getEntityTranslucent(look.white()));
        PlayerEntityModel<AbstractClientPlayerEntity> model = getContextModel();
        draw(matrices, buffer, light, model.head, look.boxes("head"));
        draw(matrices, buffer, light, model.body, look.boxes("body"));
        draw(matrices, buffer, light, model.rightArm, look.boxes("rightArm"));
        draw(matrices, buffer, light, model.leftArm, look.boxes("leftArm"));
        draw(matrices, buffer, light, model.rightLeg, look.boxes("rightLeg"));
        draw(matrices, buffer, light, model.leftLeg, look.boxes("leftLeg"));
    }

    private static void draw(MatrixStack matrices, VertexConsumer buffer, int light, ModelPart part, List<Look.Box> boxes) {
        if (boxes.isEmpty() || !part.visible) {
            return;
        }
        matrices.push();
        part.rotate(matrices);
        MatrixStack.Entry entry = matrices.peek();
        Matrix4f position = entry.getPositionMatrix();
        Matrix3f normal = entry.getNormalMatrix();
        for (Look.Box box : boxes) {
            cube(buffer, position, normal, light, box);
        }
        matrices.pop();
    }

    /** a box in the pixels of the part (1/16 of a block), its six faces */
    private static void cube(VertexConsumer buffer, Matrix4f position, Matrix3f normal, int light, Look.Box b) {
        float x0 = b.x0() / 16f, y0 = b.y0() / 16f, z0 = b.z0() / 16f, x1 = b.x1() / 16f, y1 = b.y1() / 16f, z1 = b.z1() / 16f;
        int a = b.argb() >>> 24, r = (b.argb() >> 16) & 0xFF, g = (b.argb() >> 8) & 0xFF, bl = b.argb() & 0xFF;
        // in the model the y axis points down and the face of the head looks to -z
        quad(buffer, position, normal, light, r, g, bl, a, 0, 0, -1, x0, y0, z0, x1, y0, z0, x1, y1, z0, x0, y1, z0);  // front
        quad(buffer, position, normal, light, r, g, bl, a, 0, 0, 1, x1, y0, z1, x0, y0, z1, x0, y1, z1, x1, y1, z1);   // back
        quad(buffer, position, normal, light, r, g, bl, a, -1, 0, 0, x0, y0, z1, x0, y0, z0, x0, y1, z0, x0, y1, z1);  // right
        quad(buffer, position, normal, light, r, g, bl, a, 1, 0, 0, x1, y0, z0, x1, y0, z1, x1, y1, z1, x1, y1, z0);   // left
        quad(buffer, position, normal, light, r, g, bl, a, 0, -1, 0, x0, y0, z1, x1, y0, z1, x1, y0, z0, x0, y0, z0);  // top
        quad(buffer, position, normal, light, r, g, bl, a, 0, 1, 0, x0, y1, z0, x1, y1, z0, x1, y1, z1, x0, y1, z1);   // bottom
    }

    private static void quad(VertexConsumer buffer, Matrix4f position, Matrix3f normal, int light, int r, int g, int b, int a, float nx, float ny,
                             float nz, float... xyz) {
        for (int i = 0; i < 12; i += 3) {
            buffer.vertex(position, xyz[i], xyz[i + 1], xyz[i + 2])
                .color(r, g, b, a)
                .texture(0.5f, 0.5f)
                .overlay(OverlayTexture.DEFAULT_UV)
                .light(light)
                .normal(normal, nx, ny, nz)
                .next();
        }
    }
}
