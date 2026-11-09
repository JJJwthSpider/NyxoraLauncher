// SPDX-License-Identifier: GPL-3.0-only
package vellora.cosmetics;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.texture.NativeImage;
import net.minecraft.client.texture.NativeImageBackedTexture;
import net.minecraft.util.Identifier;

import java.io.IOException;
import java.io.InputStream;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** The look written by the launcher: the skin, the cape and the boxes of the accessories on each part of the player */
public final class Look {
    /** a box of an accessory, in the pixels of its model part (as the cubes of the player model), and its colour (ARGB) */
    public record Box(float x0, float y0, float z0, float x1, float y1, float z1, int argb) {
    }

    /** the parts of the player model the boxes are attached to */
    public static final String[] PARTS = { "head", "body", "rightArm", "leftArm", "rightLeg", "leftLeg" };

    static final Look NONE = new Look(null, false, null, Map.of());

    private final Path skinFile, capeFile;
    private final boolean slim;
    private final Map<String, List<Box>> parts;
    private Identifier skin, cape, white;
    private boolean skinLoaded, capeLoaded;

    private Look(Path skinFile, boolean slim, Path capeFile, Map<String, List<Box>> parts) {
        this.skinFile = skinFile;
        this.slim = slim;
        this.capeFile = capeFile;
        this.parts = parts;
    }

    static Look load(Path dir) {
        Path file = dir.resolve("look.json");
        if (!Files.isRegularFile(file)) {
            return NONE;
        }
        try (Reader reader = Files.newBufferedReader(file, StandardCharsets.UTF_8)) {
            JsonObject json = JsonParser.parseReader(reader).getAsJsonObject();
            Path skin = json.has("skin") ? dir.resolve(json.get("skin").getAsString()) : null;
            Path cape = json.has("cape") ? dir.resolve(json.get("cape").getAsString()) : null;
            boolean slim = json.has("slim") && json.get("slim").getAsBoolean();
            Map<String, List<Box>> parts = new HashMap<>();
            JsonObject partsJson = json.has("parts") ? json.getAsJsonObject("parts") : new JsonObject();
            int count = 0;
            for (String part : PARTS) {
                if (!partsJson.has(part)) {
                    continue;
                }
                List<Box> boxes = new ArrayList<>();
                for (JsonElement element : partsJson.getAsJsonArray(part)) {
                    JsonArray b = element.getAsJsonArray();
                    boxes.add(new Box(b.get(0).getAsFloat(), b.get(1).getAsFloat(), b.get(2).getAsFloat(), b.get(3).getAsFloat(), b.get(4).getAsFloat(),
                                      b.get(5).getAsFloat(), (int) b.get(6).getAsLong()));
                }
                parts.put(part, List.copyOf(boxes));
                count += boxes.size();
            }
            VelloraCosmetics.LOG.info("Look from the launcher: skin {}, cape {}, {} boxes of accessories", skin != null, cape != null, count);
            return new Look(skin, slim, cape, parts);
        } catch (IOException | RuntimeException e) {
            VelloraCosmetics.LOG.warn("The look from the launcher could not be read: {}", e.toString());
            return NONE;
        }
    }

    public boolean hasSkin() {
        return skinFile != null;
    }

    public boolean slim() {
        return slim;
    }

    public List<Box> boxes(String part) {
        return parts.getOrDefault(part, List.of());
    }

    public boolean hasBoxes() {
        return !parts.isEmpty();
    }

    /** the skin's texture (loaded on the first use, on the render thread), or null */
    public Identifier skin() {
        if (!skinLoaded) {
            skinLoaded = true;
            skin = register("skin", skinFile);
        }
        return skin;
    }

    /** the cape's texture, or null */
    public Identifier cape() {
        if (!capeLoaded) {
            capeLoaded = true;
            cape = register("cape", capeFile);
        }
        return cape;
    }

    /** a white texture: the boxes take their colours from the vertices */
    public Identifier white() {
        if (white == null) {
            NativeImage image = new NativeImage(16, 16, false);
            image.fillRect(0, 0, 16, 16, 0xFFFFFFFF);
            white = new Identifier(VelloraCosmetics.MOD_ID, "white");
            MinecraftClient.getInstance().getTextureManager().registerTexture(white, new NativeImageBackedTexture(image));
        }
        return white;
    }

    private static Identifier register(String name, Path file) {
        if (file == null || !Files.isRegularFile(file)) {
            return null;
        }
        try (InputStream in = Files.newInputStream(file)) {
            NativeImage image = NativeImage.read(in);
            Identifier id = new Identifier(VelloraCosmetics.MOD_ID, name);
            MinecraftClient.getInstance().getTextureManager().registerTexture(id, new NativeImageBackedTexture(image));
            return id;
        } catch (IOException | RuntimeException e) {
            VelloraCosmetics.LOG.warn("The {} from the launcher could not be loaded: {}", name, e.toString());
            return null;
        }
    }
}
