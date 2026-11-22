/*
 * Nyxora for the iPhone: the player in 3D with the wardrobe's accessories, drawn the same way as on Android
 * (android/src/.../CharacterView.java, ported line for line): a small software renderer draws the Minecraft model
 * from the skin and the accessories (pixel art turned into boxes) into a frame of about 360 pixels in height with a
 * depth buffer, which is then scaled up sharp. NyxLook.export writes the look for the game (the Nyxora Cosmetics mod)
 * exactly as the PC and the phone do.
 */
(function () {
  "use strict";

  const HEAD = 0, BODY = 1, RARM = 2, LARM = 3, RLEG = 4, LLEG = 5, WORLD = 6, RWING = 7, LWING = 8, CAPE = 9, PARTS = 10;
  const EMOTES = ["wave", "jump", "dance", "spin", "bow", "cheer", "clap", "salute", "shrug", "point", "dab", "flip", "think", "punch"];
  const BASE_YAW = 24 * Math.PI / 180;

  /* ------------------------------------------------------------ the wardrobe's data */

  /** #RRGGBB or #RRGGBBAA, as an unsigned ARGB number */
  function color(hex) {
    const rgb = parseInt(hex.substring(1, 7), 16);
    const alpha = hex.length >= 9 ? parseInt(hex.substring(7, 9), 16) : 255;
    return ((alpha << 24) | rgb) >>> 0;
  }

  const Cosmetics = {
    HEAD: "head", BODY: "body", BACK: "back", PET: "pet", HAND: "hand", EFFECT: "effect", EMOTE: "emote",
    items: [], outfits: [], byId: {},

    load(data) {
      this.items = [];
      this.byId = {};
      for (const o of data.items || []) {
        const item = { id: o.id, slot: o.slot, title: o.title, rarity: o.rarity || "common", layers: [] };
        for (const l of o.layers || []) {
          const colors = {};
          for (const key of Object.keys(l.colors)) colors[key] = color(l.colors[key]);
          item.layers.push({ x: l.x, y: l.y, res: Math.max(1, l.res || 1), rows: l.rows, relief: l.relief || null, colors,
            view: l.view || "front", anim: l.anim || "" });
        }
        if (o.particles) {
          item.particleColors = o.particles.colors.map(color);
          item.particleCount = o.particles.count || 10;
          item.motion = o.particles.motion || "";
          item.shape = o.particles.shape || "";
        }
        this.items.push(item);
        this.byId[item.id] = item;
      }
      this.outfits = data.outfits || [];
    },
    find(id) { return id ? this.byId[id] || null : null; },
    inSlot(slot) { return this.items.filter((i) => i.slot === slot); },
    rarityColor(r) { return r === "legendary" ? "#FFB43C" : r === "epic" ? "#B04CFF" : r === "rare" ? "#3C8CFF" : "#8A93A0"; },
    rarityName(r) { return r === "legendary" ? "Легендарный" : r === "epic" ? "Эпический" : r === "rare" ? "Редкий" : "Обычный"; },
  };

  /* ------------------------------------------------------------ 3x4 matrices */

  /** out = a * b (b applied first) */
  function mul(a, b) {
    const r = new Float64Array(12);
    for (let i = 0; i < 3; i++) {
      for (let j = 0; j < 4; j++) {
        let v = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        if (j === 3) v += a[i * 4 + 3];
        r[i * 4 + j] = v;
      }
    }
    return r;
  }

  /** a rotation about a pivot: first z, then x, then y */
  function rot(px, py, pz, rx, ry, rz) {
    const cx = Math.cos(rx), sx = Math.sin(rx), cy = Math.cos(ry), sy = Math.sin(ry), cz = Math.cos(rz), sz = Math.sin(rz);
    const z = [cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0];
    const x = [1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0];
    const y = [cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0];
    const r = mul(y, mul(x, z));
    r[3] = px - (r[0] * px + r[1] * py + r[2] * pz);
    r[7] = py - (r[4] * px + r[5] * py + r[6] * pz);
    r[11] = pz - (r[8] * px + r[9] * py + r[10] * pz);
    return r;
  }

  function translate(x, y, z) { return Float64Array.of(1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z); }
  function mix(a, b, t) { return a + (b - a) * t; }

  /* ------------------------------------------------------------ accessories as boxes (as CharacterView.build) */

  function filled(layer, ch) { return ch !== "." && ch !== " " && layer.colors[ch] !== undefined && layer.colors[ch] !== 0; }
  function bumpAt(relief, i) {
    if (!relief || i >= relief.length) return 0;
    const d = relief.charCodeAt(i) - 48;
    return d >= 0 && d <= 9 ? d : 0;
  }

  /** an item's pixels as 3D boxes {part, x0..z1, color}: around the head, on the clothes, on the back, in the hand, beside */
  function build(item, slot) {
    const out = [];
    const add = (part, x0, y0, z0, x1, y1, z1, c) => out.push({ part, x0, y0, z0, x1, y1, z1, color: c });
    const ring = /crown|halo|tiara|wreath/.test(item.id);
    let hasFront = false, hasBack = false;
    for (const l of item.layers) {
      hasFront = hasFront || l.view !== "back";
      hasBack = hasBack || l.view === "back";
    }
    for (const layer of item.layers) {
      const backView = layer.view === "back";
      const px = 1 / layer.res;
      for (let j = 0; j < layer.rows.length; j++) {
        const row = layer.rows[j];
        const relief = layer.relief && j < layer.relief.length ? layer.relief[j] : "";
        let i = 0;
        while (i < row.length) {
          const ch = row[i];
          if (!filled(layer, ch)) { i++; continue; }
          const bump = bumpAt(relief, i);
          let end = i;
          while (end < row.length && row[end] === ch && bumpAt(relief, end) === bump) end++;
          let segStart = i, segEnd = end;
          while (segStart > 0 && filled(layer, row[segStart - 1])) segStart--;
          while (segEnd < row.length && filled(layer, row[segEnd])) segEnd++;
          const gx0 = layer.x + i * px, gx1 = layer.x + end * px, gy = layer.y + j * px;
          place(add, item, slot, backView, hasFront, hasBack, ring, gx0, gx1, gy, (segEnd - segStart) * px, segStart === i, segEnd === end,
            layer.colors[ch], px, bump * 0.12);
          i = end;
        }
      }
    }
    return out;
  }

  function place(add, item, slot, backView, hasFront, hasBack, ring, gx0, gx1, gy, segWidth, segLeft, segRight, c, px, bulge) {
    let x0 = gx0 - 8, x1 = gx1 - 8, frontGx = (gx0 + gx1) / 2;
    if (backView) { x0 = 8 - gx1; x1 = 8 - gx0; frontGx = 16 - frontGx; }
    const y1 = 32 - gy, y0 = y1 - px;
    if (slot === "head") {
      const onFace = frontGx >= 4 && frontGx < 12 && gy >= 0 && gy < 8;
      if (onFace || gy >= 8) {
        if (backView) add(HEAD, x0, y0, -4.65 - bulge, x1, y1, -4.05, c);
        else add(HEAD, x0, y0, 4.05, x1, y1, 4.65 + bulge, c);
      } else if (backView) {
        return;
      } else if (ring) {
        add(HEAD, x0, y0, -4.7, x1, y1, -3.7, c);
        add(HEAD, x0, y0, 3.7, x1, y1, 4.7, c);
        if (segWidth >= 6 && segLeft) add(HEAD, x0, y0, -4.7, x0 + 1, y1, 4.7, c);
        if (segWidth >= 6 && segRight) add(HEAD, x1 - 1, y0, -4.7, x1, y1, 4.7, c);
      } else {
        const ears = item.id.endsWith("_ears");
        const half = ears ? 0.7 + bulge / 2 : Math.min(4.7, Math.max(1.2, segWidth / 2) + bulge / 2);
        add(HEAD, x0, y0, -half, x1, y1, half, c);
      }
    } else if (slot === "body") {
      let part = BODY, zf = 2.05, zb = 2.65;
      if (gy < 8) { part = HEAD; zf = 4.05; zb = 4.65; }
      else if (gy >= 20) part = frontGx < 8 ? RLEG : LLEG;
      else if (frontGx < 4) part = RARM;
      else if (frontGx >= 12) part = LARM;
      if (backView) add(part, x0, y0, -zb - bulge, x1, y1, -zf, c);
      else add(part, x0, y0, zf, x1, y1, zb + bulge, c);
    } else if (slot === "back") {
      const wings = item.id.includes("wings"), pack = item.id.includes("pack"), capeItem = item.id.includes("cape");
      if (wings) {
        if (backView && hasFront) return;
        add((x0 + x1) / 2 < 0 ? RWING : LWING, x0, y0, -3.2 - bulge, x1, y1, -2.5, c);
      } else if (pack) {
        if (!backView && hasBack && frontGx >= 4 && frontGx < 12) return;
        add(BODY, x0, y0, -5.2 - bulge, x1, y1, -2.05, c);
      } else if (capeItem) {
        if (!backView && hasBack) return;
        add(CAPE, x0, y0, -3.1 - bulge / 2, x1, y1, -2.5, c);
      } else {
        if (!backView && hasBack && frontGx >= 4 && frontGx < 12) return;
        add(BODY, x0, y0, -3.2 - bulge, x1, y1, -2.3, c);
      }
    } else if (slot === "hand") {
      if (backView) return;
      add(RARM, x0, y0, 2.1 - bulge / 2, x1, y1, 2.9 + bulge / 2, c);
    } else if (slot === "pet") {
      if (backView) return;
      add(WORLD, x0 + 1, y0, 1.5 - bulge / 2, x1 + 1, y1, 4.5 + bulge / 2, c);
    }
  }

  const MESHES = {};
  function mesh(item, slot) {
    if (!MESHES[item.id]) MESHES[item.id] = build(item, slot);
    return MESHES[item.id];
  }

  /* ------------------------------------------------------------ the look for the game (as CharacterView.exportLook) */

  const NyxLook = {
    /**
     * worn: {head, body, back}; skin: an image or canvas (64x64 or 64x32) or null; cape: an image or null.
     * Returns {look: JSON text, skin: PNG data URL or null, cape: PNG data URL or null}; only head, body and back are
     * shown in the game (pets, things in the hand and effects stay in Nyxora).
     */
    export(worn, skin, slim, cape) {
      const look = { version: 1 };
      let skinUrl = null, capeUrl = null;
      if (skin) {
        const c = document.createElement("canvas");
        c.width = 64; c.height = 64;
        const g = c.getContext("2d");
        g.imageSmoothingEnabled = false;
        const w = skin.naturalWidth || skin.width, h = skin.naturalHeight || skin.height;
        if (w === 64 && h === 32) {
          // the old 64x32 layout: the 64x64 one the player model reads, the left arm and leg from the right ones
          g.drawImage(skin, 0, 0);
          g.drawImage(skin, 0, 16, 16, 16, 16, 48, 16, 16);
          g.drawImage(skin, 40, 16, 16, 16, 32, 48, 16, 16);
        } else {
          g.drawImage(skin, 0, 0, 64, 64);
        }
        skinUrl = c.toDataURL("image/png");
        look.skin = "skin.png";
        look.slim = !!slim;
      }
      if (cape) {
        const c = document.createElement("canvas");
        c.width = cape.naturalWidth || cape.width; c.height = cape.naturalHeight || cape.height;
        c.getContext("2d").drawImage(cape, 0, 0);
        capeUrl = c.toDataURL("image/png");
        look.cape = "cape.png";
      }
      const names = ["head", "body", "rightArm", "leftArm", "rightLeg", "leftLeg"];
      const armY = slim ? 2.5 : 2;
      const pivots = [[0, 0, 0], [0, 0, 0], [-5, armY, 0], [5, armY, 0], [-1.9, 12, 0], [1.9, 12, 0]];
      const boxes = [[], [], [], [], [], []];
      for (const slot of ["head", "body", "back"]) {
        const item = Cosmetics.find(worn[slot]);
        if (!item) continue;
        for (const b of build(item, slot)) {
          let part;
          switch (b.part) {
            case HEAD: part = 0; break;
            case BODY: case RWING: case LWING: case CAPE: part = 1; break;
            case RARM: part = 2; break;
            case LARM: part = 3; break;
            case RLEG: part = 4; break;
            case LLEG: part = 5; break;
            default: continue;
          }
          const p = pivots[part];
          boxes[part].push([b.x0 - p[0], 24 - b.y1 - p[1], -b.z1 - p[2], b.x1 - p[0], 24 - b.y0 - p[1], -b.z0 - p[2], b.color >>> 0]);
        }
      }
      const parts = {};
      for (let i = 0; i < 6; i++) if (boxes[i].length) parts[names[i]] = boxes[i];
      look.parts = parts;
      return { look: JSON.stringify(look), skin: skinUrl, cape: capeUrl };
    },
  };

  /* ------------------------------------------------------------ the dark figure when there is no skin */

  let shadowSkin = null;
  function shadow() {
    if (shadowSkin) return shadowSkin;
    const t = new Uint32Array(64 * 64);
    const list = [[0, 0, 8, 8, 8], [16, 16, 8, 12, 4], [40, 16, 4, 12, 4], [0, 16, 4, 12, 4], [32, 48, 4, 12, 4], [16, 48, 4, 12, 4]];
    for (const [u, v, w, h, d] of list) {
      for (let y = v; y < v + d + h; y++) {
        for (let x = u; x < u + 2 * (d + w); x++) {
          if (y < v + d && (x < u + d || x >= u + d + 2 * w)) continue;
          const edge = y === v || y === v + d + h - 1 || x === u || x === u + 2 * (d + w) - 1 || x === u + d || x === u + d + w - 1;
          t[y * 64 + x] = edge ? 0xFF674DAB : 0xFF110E2B;
        }
      }
    }
    t[12 * 64 + 10] = t[12 * 64 + 13] = 0xFF9A62FF;
    shadowSkin = { px: t, w: 64, h: 64 };
    return shadowSkin;
  }

  /** an image's pixels as ARGB numbers */
  function pixelsOf(img, w, h) {
    const c = document.createElement("canvas");
    c.width = w; c.height = h;
    const g = c.getContext("2d");
    g.imageSmoothingEnabled = false;
    g.drawImage(img, 0, 0, w, h);
    const d = g.getImageData(0, 0, w, h).data;
    const out = new Uint32Array(w * h);
    for (let i = 0; i < w * h; i++) out[i] = ((d[i * 4 + 3] << 24) | (d[i * 4] << 16) | (d[i * 4 + 1] << 8) | d[i * 4 + 2]) >>> 0;
    return out;
  }

  function hash(a, b) {
    let h = (Math.imul(a, 374761393) + Math.imul(b, 668265263) + 0x2545F491) | 0;
    h = Math.imul(h ^ (h >>> 13), 1274126177);
    return ((h ^ (h >>> 16)) & 0x7fffffff) % 10000 / 10000;
  }

  /* ------------------------------------------------------------ the view */

  class NyxCharacter {
    /** canvas: where to draw; options: {closeUp, turnable, animated, worn: () => {slot: id}} */
    constructor(canvas, options) {
      this.canvas = canvas;
      this.ctx = canvas.getContext("2d");
      this.closeUp = !!(options && options.closeUp);
      this.turnable = !(options && options.turnable === false);
      this.animated = !(options && options.animated === false);
      this.wornOf = (options && options.worn) || (() => ({}));
      this.onTap = (options && options.onTap) || null;
      this.skin = null; this.slim = false; this.cape = null;
      this.back = false; this.nametag = "";
      this.previewSlot = null; this.previewId = null;
      this.emote = ""; this.emoteStart = 0;
      this.start = performance.now();
      this.yaw = BASE_YAW; this.yawSpeed = 0; this.pitch = -10 * Math.PI / 180;
      this.dragging = false; this.lastTouch = 0;
      this.mats = new Array(PARTS);
      this.running = false;
      if (this.turnable) this.listen();
    }

    setSkin(img, slim) {
      this.slim = !!slim;
      if (!img) { this.skin = null; return; }
      const w = img.naturalWidth || img.width, h = img.naturalHeight || img.height;
      if (w < 64 || h < 32) { this.skin = null; return; }
      const sh = Math.round(h * 64 / w);
      this.skin = { px: pixelsOf(img, 64, sh), w: 64, h: sh };
    }

    setCape(img) {
      if (!img) { this.cape = null; return; }
      // drawn at 64x32 whatever its size (HD capes have the same layout)
      this.cape = { px: pixelsOf(img, 64, 32), w: 64, h: 32 };
    }

    setBack(back) { this.back = back; this.lastTouch = 0; }
    preview(slot, id) { this.previewSlot = slot; this.previewId = id; }
    playEmote(id) { this.emote = id || ""; this.emoteStart = performance.now(); }
    randomEmote() {
      let next;
      do { next = EMOTES[Math.floor(Math.random() * EMOTES.length)]; } while (next === this.emote && EMOTES.length > 1);
      this.playEmote(next);
    }

    worn(slot) {
      if (slot === this.previewSlot && this.previewId !== null) return this.previewId;
      return this.wornOf()[slot] || "";
    }

    listen() {
      const c = this.canvas;
      let downX = 0, downY = 0, lastX = 0, downTime = 0;
      c.style.touchAction = "none";
      c.addEventListener("pointerdown", (e) => {
        downX = lastX = e.clientX; downY = e.clientY; downTime = performance.now();
        this.dragging = false; this.yawSpeed = 0;
        c.setPointerCapture(e.pointerId);
      });
      c.addEventListener("pointermove", (e) => {
        if (!c.hasPointerCapture(e.pointerId)) return;
        if (!this.dragging && Math.abs(e.clientX - downX) + Math.abs(e.clientY - downY) > 8) this.dragging = true;
        if (this.dragging) {
          const dx = e.clientX - lastX;
          this.yaw += dx * 0.012 * 2;
          this.yawSpeed = dx * 0.012 * 2;
          this.pitch = Math.max(-0.5, Math.min(0.25, this.pitch + (e.clientY - downY) * 0.0003));
          lastX = e.clientX;
          this.lastTouch = performance.now();
        }
      });
      const up = (e) => {
        this.lastTouch = performance.now();
        if (!this.dragging && performance.now() - downTime < 400) {
          this.randomEmote();
          if (this.onTap) this.onTap();
        }
        this.dragging = false;
        if (c.hasPointerCapture(e.pointerId)) c.releasePointerCapture(e.pointerId);
      };
      c.addEventListener("pointerup", up);
      c.addEventListener("pointercancel", () => { this.dragging = false; });
    }

    run() {
      if (this.running) return;
      this.running = true;
      const tick = () => {
        if (!this.running) return;
        if (this.canvas.isConnected && this.canvas.offsetParent !== null) this.draw();
        requestAnimationFrame(tick);
      };
      requestAnimationFrame(tick);
    }
    stop() { this.running = false; }

    resize() {
      const dpr = window.devicePixelRatio || 1;
      const w = Math.max(1, Math.round(this.canvas.clientWidth * dpr)), h = Math.max(1, Math.round(this.canvas.clientHeight * dpr));
      if (this.canvas.width !== w || this.canvas.height !== h) { this.canvas.width = w; this.canvas.height = h; }
      const k = Math.max(1, Math.ceil(h / 360));
      const bw = Math.max(1, Math.floor(w / k)), bh = Math.max(1, Math.floor(h / k));
      if (bw !== this.bufW || bh !== this.bufH) {
        this.bufW = bw; this.bufH = bh;
        this.frame = new ImageData(bw, bh);
        this.color = new Uint32Array(this.frame.data.buffer);  // little-endian: 0xAABBGGRR
        this.depth = new Float32Array(bw * bh);
        this.buffer = document.createElement("canvas");
        this.buffer.width = bw; this.buffer.height = bh;
      }
      this.k = k;
    }

    pose(time, now) {
      const emoteTime = (now - this.emoteStart) / 1000;
      const emoting = this.emote !== "" && emoteTime < 2.4;
      if (!this.dragging) {
        this.yaw += this.yawSpeed;
        this.yawSpeed *= 0.92;
        if (Math.abs(this.yawSpeed) < 0.002) this.yawSpeed = 0;
        if (this.yawSpeed === 0 && now - this.lastTouch > 2500) {
          const target = BASE_YAW + (this.back ? Math.PI : 0);
          const diff = Math.atan2(Math.sin(target - this.yaw), Math.cos(target - this.yaw));
          this.yaw += diff * 0.06;
          this.pitch += (-10 * Math.PI / 180 - this.pitch) * 0.06;
        }
      }
      let breath = Math.sin(time * 2) * 0.25, lift = 0, spin = 0, bow = 0, sway = 0, flipAngle = 0;
      let rArmX = Math.sin(time * 1.3) * 0.07, lArmX = -rArmX;
      let rArmZ = -0.05 - Math.sin(time * 2) * 0.02, lArmZ = -rArmZ;
      let rLegX = 0, lLegX = 0;
      let headYaw = Math.sin(time * 0.6) * 0.22, headPitch = Math.sin(time * 0.45) * 0.07;
      if (!this.animated) { breath = 0; rArmX = lArmX = 0; headYaw = headPitch = 0; }
      if (emoting) {
        const p = emoteTime / 2.4, fade = Math.min(1, Math.min(emoteTime * 4, (2.4 - emoteTime) * 4)), e = this.emote;
        if (e === "wave") { rArmZ = mix(rArmZ, -2.55 + Math.sin(emoteTime * 12) * 0.28, fade); headYaw *= 1 - fade; }
        else if (e === "cheer") { const wob = Math.sin(emoteTime * 14) * 0.2; rArmZ = mix(rArmZ, -2.7 + wob, fade); lArmZ = mix(lArmZ, 2.7 - wob, fade); lift = Math.abs(Math.sin(emoteTime * 7)) * 1.2 * fade; }
        else if (e === "jump") { const j = Math.max(0, Math.sin(emoteTime * Math.PI / 0.6)); lift = j * 7; rArmZ = mix(rArmZ, -0.9, j); lArmZ = mix(lArmZ, 0.9, j); rLegX = -j * 0.5; lLegX = j * 0.3; }
        else if (e === "dance") { const b = Math.sin(emoteTime * 9); sway = b * 0.35 * fade; rArmX = mix(rArmX, -1.6 + b * 0.9, fade); lArmX = mix(lArmX, -1.6 - b * 0.9, fade); rArmZ = mix(rArmZ, -0.4, fade); lArmZ = mix(lArmZ, 0.4, fade); rLegX = b * 0.35 * fade; lLegX = -b * 0.35 * fade; lift = Math.abs(Math.cos(emoteTime * 9)) * fade; }
        else if (e === "spin") { const q = Math.min(1, p * 1.6); spin = Math.PI * 2 * (q * q * (3 - 2 * q)); rArmZ = mix(rArmZ, -1.2, fade); lArmZ = mix(lArmZ, 1.2, fade); }
        else if (e === "bow") { bow = Math.sin(Math.min(1, p * 1.4) * Math.PI) * 0.6; rArmX = mix(rArmX, -0.4, fade); lArmX = mix(lArmX, 0.3, fade); }
        else if (e === "clap") { const c = Math.abs(Math.sin(emoteTime * 9)); rArmX = mix(rArmX, -1.25, fade); lArmX = mix(lArmX, -1.25, fade); rArmZ = mix(rArmZ, 0.15 + c * 0.35, fade); lArmZ = mix(lArmZ, -0.15 - c * 0.35, fade); }
        else if (e === "salute") { rArmZ = mix(rArmZ, -2.35, fade); rArmX = mix(rArmX, -0.55, fade); headPitch = mix(headPitch, -0.08, fade); headYaw *= 1 - fade; }
        else if (e === "shrug") { const u = Math.abs(Math.sin(emoteTime * 3)); rArmZ = mix(rArmZ, -0.8 - u * 0.2, fade); lArmZ = mix(lArmZ, 0.8 + u * 0.2, fade); rArmX = mix(rArmX, -0.6, fade); lArmX = mix(lArmX, -0.6, fade); headPitch = mix(headPitch, 0.12, fade); headYaw = mix(headYaw, Math.sin(emoteTime * 3) * 0.25, fade); }
        else if (e === "point") { rArmX = mix(rArmX, -1.6, fade); rArmZ = mix(rArmZ, 0.05, fade); headYaw *= 1 - fade; }
        else if (e === "dab") { rArmZ = mix(rArmZ, -2.3, fade); rArmX = mix(rArmX, -0.3, fade); lArmX = mix(lArmX, -1.3, fade); lArmZ = mix(lArmZ, -0.7, fade); headPitch = mix(headPitch, 0.45, fade); headYaw = mix(headYaw, 0.45, fade); }
        else if (e === "flip") { const q = Math.min(1, p * 1.5), arc = Math.sin(q * Math.PI); lift = arc * 9; flipAngle = -(Math.PI * 2 * (q * q * (3 - 2 * q))); rArmZ = mix(rArmZ, -2.6, arc); lArmZ = mix(lArmZ, 2.6, arc); }
        else if (e === "think") { rArmX = mix(rArmX, -2.0, fade); rArmZ = mix(rArmZ, 0.55, fade); headPitch = mix(headPitch, 0.18, fade); headYaw = mix(headYaw, -0.2, fade); }
        else if (e === "punch") { const a = Math.max(0, Math.sin(emoteTime * 10)), b = Math.max(0, -Math.sin(emoteTime * 10)); rArmX = mix(rArmX, -0.6 - a, fade); lArmX = mix(lArmX, -0.6 - b, fade); sway = (a - b) * 0.15 * fade; }
      }
      if (this.animated && !emoting && !this.dragging) { rLegX = Math.sin(time * 1.3) * 0.03; lLegX = -rLegX; }

      const totalYaw = this.yaw + spin + sway * 0.4;
      let camera = mul(rot(0, 0, 0, this.pitch, 0, 0), rot(0, 0, 0, 0, totalYaw, 0));
      camera = mul(camera, translate(0, lift + breath * 0.3, 0));
      if (flipAngle !== 0) camera = mul(camera, rot(0, 16, 0, flipAngle, 0, 0));
      const upperCam = mul(camera, rot(0, 12, 0, bow, sway * 0.6, 0));
      const armW = this.slim ? 3 : 4;
      const m = this.mats;
      m[HEAD] = mul(upperCam, rot(0, 24, 0, headPitch + bow * 0.5, headYaw, 0));
      m[BODY] = upperCam;
      m[RARM] = mul(upperCam, rot(-4 - armW / 2, 22, 0, rArmX, 0, rArmZ));
      m[LARM] = mul(upperCam, rot(4 + armW / 2, 22, 0, lArmX, 0, lArmZ));
      m[RLEG] = mul(camera, rot(-2, 12, 0, rLegX, 0, 0));
      m[LLEG] = mul(camera, rot(2, 12, 0, lLegX, 0, 0));
      m[WORLD] = camera;
      const flap = this.animated ? 0.35 + 0.35 * Math.sin(time * 2.4) : 0.4;
      m[RWING] = mul(upperCam, rot(-1, 20, -2.6, 0, -flap, 0));
      m[LWING] = mul(upperCam, rot(1, 20, -2.6, 0, flap, 0));
      const capeTilt = 0.12 + (this.animated ? Math.sin(time * 1.7) * 0.04 : 0) + (lift > 0 ? lift * 0.03 : 0) + Math.abs(this.yawSpeed) * 2;
      m[CAPE] = mul(upperCam, rot(0, 24, -2.9, capeTilt, 0, 0));
      return lift;
    }

    draw() {
      this.resize();
      const now = performance.now(), time = (now - this.start) / 1000;
      const lift = this.pose(time, now);
      const bw = this.bufW, bh = this.bufH;
      if (this.closeUp) {
        this.scale = Math.min(bw / 24, bh / 19); this.centerX = bw / 2; this.centerY = bh * 0.55 + 25 * this.scale;
      } else {
        this.scale = Math.min(bw / 34, bh / 44); this.centerX = bw / 2; this.centerY = bh * 0.5 + 15 * this.scale;
      }
      this.color.fill(0);
      this.depth.fill(-1e9);
      const m = this.mats;
      const tex = this.skin || shadow();
      const armW = this.slim ? 3 : 4;
      const modern = tex.h >= 64;
      this.drawBox(m[HEAD], -4, 24, -4, 8, 8, 8, 0, 0, tex, 0);
      this.drawBox(m[BODY], -4, 12, -2, 8, 12, 4, 16, 16, tex, 0);
      this.drawBox(m[RARM], -4 - armW, 12, -2, armW, 12, 4, 40, 16, tex, 0);
      this.drawBox(m[RLEG], -4, 0, -2, 4, 12, 4, 0, 16, tex, 0);
      if (modern) {
        this.drawBox(m[LARM], 4, 12, -2, armW, 12, 4, 32, 48, tex, 0);
        this.drawBox(m[LLEG], 0, 0, -2, 4, 12, 4, 16, 48, tex, 0);
        this.drawBox(m[HEAD], -4, 24, -4, 8, 8, 8, 32, 0, tex, 0.5);
        this.drawBox(m[BODY], -4, 12, -2, 8, 12, 4, 16, 32, tex, 0.25);
        this.drawBox(m[RARM], -4 - armW, 12, -2, armW, 12, 4, 40, 32, tex, 0.25);
        this.drawBox(m[LARM], 4, 12, -2, armW, 12, 4, 48, 48, tex, 0.25);
        this.drawBox(m[RLEG], -4, 0, -2, 4, 12, 4, 0, 32, tex, 0.25);
        this.drawBox(m[LLEG], 0, 0, -2, 4, 12, 4, 0, 48, tex, 0.25);
      } else {
        this.drawBox(m[LARM], 4, 12, -2, armW, 12, 4, 40, 16, tex, 0, true);
        this.drawBox(m[LLEG], 0, 0, -2, 4, 12, 4, 0, 16, tex, 0, true);
        this.drawBox(m[HEAD], -4, 24, -4, 8, 8, 8, 32, 0, tex, 0.5);
      }
      const backItem = this.worn("back");
      if (this.cape && !backItem.includes("cape") && backItem !== "backpack" && backItem !== "jetpack") {
        const capeMat = mul(m[CAPE], rot(0, 24, -2.9, 0, Math.PI, 0));
        this.drawBox(capeMat, -5, 8, -3.4, 10, 16, 1, 0, 0, this.cape, 0);
      }
      for (const slot of ["back", "body", "head", "pet", "hand"]) this.drawItem(Cosmetics.find(this.worn(slot)), slot, time);
      this.drawParticles(Cosmetics.find(this.worn("effect")), time);

      // the violet light on the floor, then the frame, sharp
      const g = this.ctx, k = this.k;
      g.setTransform(1, 0, 0, 1, 0, 0);
      g.clearRect(0, 0, this.canvas.width, this.canvas.height);
      const foot = this.project(m[WORLD], 0, 0, 0);
      const fx = foot[0] * k, fy = foot[1] * k + (lift > 0 ? lift * this.scale * k : 0), radius = 16 * this.scale * k;
      g.save();
      g.translate(fx, fy);
      g.scale(1, 0.25);
      const glow = g.createRadialGradient(0, 0, 0, 0, 0, radius);
      glow.addColorStop(0, "rgba(129,84,240,0.44)");
      glow.addColorStop(1, "rgba(129,84,240,0)");
      g.fillStyle = glow;
      g.beginPath(); g.arc(0, 0, radius, 0, Math.PI * 2); g.fill();
      g.restore();
      this.buffer.getContext("2d").putImageData(this.frame, 0, 0);
      g.imageSmoothingEnabled = false;
      g.drawImage(this.buffer, 0, 0, bw * k, bh * k);
      if (this.nametag && !this.closeUp) {
        const top = this.project(m[HEAD], 0, 36, 0);
        const size = Math.max(10 * (window.devicePixelRatio || 1), 2.6 * this.scale * k);
        g.font = `${size}px Tiny5, monospace`;
        g.textAlign = "center";
        const half = g.measureText(this.nametag).width / 2 + this.scale * k;
        g.fillStyle = "rgba(0,0,0,0.55)";
        g.fillRect(top[0] * k - half, top[1] * k - size, half * 2, size * 1.3);
        g.fillStyle = "#fff";
        g.fillText(this.nametag, top[0] * k, top[1] * k);
      }
    }

    project(m, x, y, z) {
      return [this.centerX + (m[0] * x + m[1] * y + m[2] * z + m[3]) * this.scale,
              this.centerY - (m[4] * x + m[5] * y + m[6] * z + m[7]) * this.scale,
              m[8] * x + m[9] * y + m[10] * z + m[11]];
    }

    /** a box of the Minecraft model with the usual texture layout at (u, v); `inflate` sets the second layer off */
    drawBox(m, x, y, z, w, h, d, u, v, tex, inflate, mirror) {
      const x0 = x - inflate, x1 = x + w + inflate, y0 = y - inflate, y1 = y + h + inflate, z0 = z - inflate, z1 = z + d + inflate;
      const iw = Math.round(w), ih = Math.round(h), id = Math.round(d);
      if (!mirror) {
        this.face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, tex, u + id, v + id, iw, ih, 0, false);
        this.face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, tex, u + 2 * id + iw, v + id, iw, ih, 0, false);
        this.face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, tex, u, v + id, id, ih, 0, false);
        this.face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, tex, u + id + iw, v + id, id, ih, 0, false);
      } else {
        this.face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, tex, u + id, v + id, iw, ih, 0, true);
        this.face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, tex, u + 2 * id + iw, v + id, iw, ih, 0, true);
        this.face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, tex, u, v + id, id, ih, 0, true);
        this.face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, tex, u + id + iw, v + id, id, ih, 0, true);
      }
      this.face(m, x0, y1, z0, x1, y1, z0, x0, y1, z1, tex, u + id, v, iw, id, 0, !!mirror);
      this.face(m, x0, y0, z1, x1, y0, z1, x0, y0, z0, tex, u + id + iw, v, iw, id, 0, !!mirror);
    }

    solidBox(m, x0, y0, z0, x1, y1, z1, c) {
      this.face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, null, 0, 0, 1, 1, c, false);
      this.face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, null, 0, 0, 1, 1, c, false);
      this.face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, null, 0, 0, 1, 1, c, false);
      this.face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, null, 0, 0, 1, 1, c, false);
      this.face(m, x0, y1, z0, x1, y1, z0, x0, y1, z1, null, 0, 0, 1, 1, c, false);
      this.face(m, x0, y0, z1, x1, y0, z1, x0, y0, z0, null, 0, 0, 1, 1, c, false);
    }

    /** one face from three corners (top left, top right, bottom left), shaded by how it faces the light, depth-tested */
    face(m, ax, ay, az, bx, by, bz, cx, cy, cz, tex, su, sv, sw, sh, solid, mirror) {
      const AX = m[0] * ax + m[1] * ay + m[2] * az + m[3], AY = m[4] * ax + m[5] * ay + m[6] * az + m[7], AZ = m[8] * ax + m[9] * ay + m[10] * az + m[11];
      const BX = m[0] * bx + m[1] * by + m[2] * bz + m[3], BY = m[4] * bx + m[5] * by + m[6] * bz + m[7], BZ = m[8] * bx + m[9] * by + m[10] * bz + m[11];
      const CX = m[0] * cx + m[1] * cy + m[2] * cz + m[3], CY = m[4] * cx + m[5] * cy + m[6] * cz + m[7], CZ = m[8] * cx + m[9] * cy + m[10] * cz + m[11];
      const ex = BX - AX, ey = BY - AY, ez = BZ - AZ, fx = CX - AX, fy = CY - AY, fz = CZ - AZ;
      const nx = fy * ez - fz * ey, ny = fz * ex - fx * ez, nz = fx * ey - fy * ex;
      const len = Math.sqrt(nx * nx + ny * ny + nz * nz);
      if (len < 1e-6 || nz / len <= 0.001) return;
      const light = (nx * -0.35 + ny * 0.8 + nz * 0.5) / len / 0.995;
      let bright = 0.56 + 0.46 * Math.max(0, light);
      if (ny / len < -0.5) bright *= 0.85;
      const b256 = Math.round(Math.min(1.08, bright) * 256);
      const scale = this.scale, bufW = this.bufW, bufH = this.bufH;
      const sAx = this.centerX + AX * scale, sAy = this.centerY - AY * scale;
      const e1x = ex * scale, e1y = -ey * scale, e2x = fx * scale, e2y = -fy * scale;
      const det = e1x * e2y - e1y * e2x;
      if (Math.abs(det) < 1e-4) return;
      const minX = Math.min(sAx, sAx + e1x, sAx + e2x, sAx + e1x + e2x), maxX = Math.max(sAx, sAx + e1x, sAx + e2x, sAx + e1x + e2x);
      const minY = Math.min(sAy, sAy + e1y, sAy + e2y, sAy + e1y + e2y), maxY = Math.max(sAy, sAy + e1y, sAy + e2y, sAy + e1y + e2y);
      const x0 = Math.max(0, Math.floor(minX)), x1 = Math.min(bufW - 1, Math.ceil(maxX));
      const y0 = Math.max(0, Math.floor(minY)), y1 = Math.min(bufH - 1, Math.ceil(maxY));
      const dsdx = e2y / det, dsdy = -e2x / det, dtdx = -e1y / det, dtdy = e1x / det;
      const dz1 = BZ - AZ, dz2 = CZ - AZ, eps = 0.02;
      const color = this.color, depth = this.depth;
      const tpx = tex ? tex.px : null, tw = tex ? tex.w : 0, th = tex ? tex.h : 0;
      for (let py = y0; py <= y1; py++) {
        const ry = py + 0.5 - sAy, rx = x0 + 0.5 - sAx;
        let s = rx * dsdx + ry * dsdy, t = rx * dtdx + ry * dtdy;
        const row = py * bufW;
        for (let px = x0; px <= x1; px++, s += dsdx, t += dtdx) {
          if (s < -eps || s >= 1 + eps || t < -eps || t >= 1 + eps) continue;
          const z = AZ + s * dz1 + t * dz2, i = row + px;
          if (z <= depth[i]) continue;
          let c;
          if (tpx) {
            let tu = Math.floor(Math.max(0, Math.min(0.999, s)) * sw);
            const tv = Math.floor(Math.max(0, Math.min(0.999, t)) * sh);
            if (mirror) tu = sw - 1 - tu;
            const ux = su + tu, vy = sv + tv;
            if (ux < 0 || vy < 0 || ux >= tw || vy >= th) continue;
            c = tpx[vy * tw + ux];
          } else {
            c = solid;
          }
          if ((c >>> 24) < 128) continue;
          const r = Math.min(255, (((c >>> 16) & 0xFF) * b256) >> 8);
          const gg = Math.min(255, (((c >>> 8) & 0xFF) * b256) >> 8);
          const bl = Math.min(255, ((c & 0xFF) * b256) >> 8);
          color[i] = (0xFF000000 | (bl << 16) | (gg << 8) | r) >>> 0;  // ImageData order: R, G, B, A
          depth[i] = z;
        }
      }
    }

    drawItem(item, slot, time) {
      if (!item) return;
      let bob = 0;
      const anim = item.layers.length ? item.layers[0].anim : "";
      if (this.animated && anim === "bob") bob = Math.sin(time * 2) * 0.5;
      else if (this.animated && anim === "hop") bob = Math.abs(Math.sin(time * 3)) * 2;
      else if (this.animated && anim === "float") bob = Math.sin(time * 2) * 1.2;
      const shift = bob !== 0 ? translate(0, bob, 0) : null;
      const cache = {};
      for (const b of mesh(item, slot)) {
        let m = this.mats[b.part];
        if (shift) m = cache[b.part] || (cache[b.part] = mul(m, shift));
        this.solidBox(m, b.x0, b.y0, b.z0, b.x1, b.y1, b.z1, b.color);
      }
    }

    drawParticles(item, time) {
      if (!item || !item.particleColors) return;
      const m = this.mats[WORLD];
      for (let i = 0; i < item.particleCount; i++) {
        const col = item.particleColors[i % item.particleColors.length];
        const speed = 4 + hash(i, 1) * 5, phase = hash(i, 2) * 40;
        let x, y, z, alpha;
        if (item.motion === "orbit") {
          const angle = time * 1.4 + i * Math.PI * 2 / item.particleCount;
          x = Math.cos(angle) * 11; z = Math.sin(angle) * 11; y = 16 + Math.sin(time * 2 + i) * 7; alpha = 1;
        } else if (item.motion === "fall") {
          x = -14 + hash(i, 3) * 28; z = -10 + hash(i, 6) * 20; y = 40 - (time * speed + phase) % 44; alpha = 0.85;
        } else if (item.motion === "spark") {
          x = -12 + hash(i, 3) * 24; z = -8 + hash(i, 6) * 16; y = 2 + hash(i, 4) * 36; alpha = Math.max(0, Math.sin(time * (2 + hash(i, 5) * 3) + phase));
        } else {
          x = -10 + hash(i, 3) * 20; z = -7 + hash(i, 6) * 14;
          const travel = (time * speed + phase) % 40;
          y = travel - 2; alpha = Math.min(1, travel / 6) * Math.max(0, 1 - travel / 40);
        }
        if (alpha <= 0.05) continue;
        const p = this.project(m, x, y, z);
        const size = Math.max(1, Math.round(this.scale * (item.shape === "heart" ? 0.45 : item.shape === "note" || item.shape === "star" ? 1.2 : 0.8)));
        if (item.shape === "heart") {
          for (const [hx, hy] of [[0, 0], [2, 0], [0, 1], [1, 1], [2, 1], [1, 2]]) this.dot(p[0] + (hx - 1) * size, p[1] + (hy - 1) * size, p[2], size, col, alpha);
        } else if (item.shape === "drop") {
          this.dot(p[0], p[1], p[2], size, col, alpha); this.dot(p[0], p[1] + size, p[2], size, col, alpha);
        } else if (item.shape === "note") {
          const q = Math.max(1, Math.floor(size / 2));
          for (let k = 0; k < 4; k++) this.dot(p[0] + q, p[1] - k * q, p[2], q, col, alpha);
          this.dot(p[0] + 2 * q, p[1] - 3 * q, p[2], q, col, alpha);
          this.dot(p[0], p[1], p[2], q * 2, col, alpha);
        } else if (item.shape === "star") {
          const q = Math.max(1, Math.floor(size / 2));
          for (const [dx, dy] of [[0, 0], [-1, 0], [1, 0], [0, -1], [0, 1]]) this.dot(p[0] + dx * q, p[1] + dy * q, p[2], q, col, alpha);
        } else {
          this.dot(p[0], p[1], p[2], size, col, alpha);
        }
      }
    }

    dot(sx, sy, z, size, col, alpha) {
      const a = Math.round(((col >>> 24) & 0xFF) * Math.min(1, alpha));
      const x0 = Math.round(sx - size / 2), y0 = Math.round(sy - size / 2);
      const cr = (col >>> 16) & 0xFF, cg = (col >>> 8) & 0xFF, cb = col & 0xFF;
      for (let y = y0; y < y0 + size; y++) {
        if (y < 0 || y >= this.bufH) continue;
        for (let x = x0; x < x0 + size; x++) {
          if (x < 0 || x >= this.bufW) continue;
          const i = y * this.bufW + x;
          if (z < this.depth[i]) continue;
          const under = this.color[i], ua = under >>> 24;
          const ur = under & 0xFF, ug = (under >>> 8) & 0xFF, ub = (under >>> 16) & 0xFF;
          const blend = (u, o) => (ua === 0 ? o : Math.floor((u * (255 - a) + o * a) / 255));
          this.color[i] = ((Math.max(ua, a) << 24) | (blend(ub, cb) << 16) | (blend(ug, cg) << 8) | blend(ur, cr)) >>> 0;
        }
      }
    }
  }

  /* ------------------------------------------------------------ an item's picture for the wardrobe's tiles */

  /** draws an item's front layers (the back ones for capes and things on the back) fitted into a canvas */
  function paintIcon(canvas, item) {
    const g = canvas.getContext("2d");
    g.clearRect(0, 0, canvas.width, canvas.height);
    if (!item || !item.layers.length) return;
    const behind = item.slot === "back" && item.layers.some((l) => l.view === "back") && !item.id.includes("wings");
    const layers = item.layers.filter((l) => l.view === "both" || l.view === (behind ? "back" : "front"));
    const use = layers.length ? layers : item.layers;
    let x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9;
    for (const l of use) {
      const w = Math.max(0, ...l.rows.map((r) => r.length)) / l.res;
      x0 = Math.min(x0, l.x); y0 = Math.min(y0, l.y); x1 = Math.max(x1, l.x + w); y1 = Math.max(y1, l.y + l.rows.length / l.res);
    }
    const u = Math.floor(Math.min(canvas.width / (x1 - x0 + 1), canvas.height / (y1 - y0 + 1)) * 100) / 100;
    const ox = (canvas.width - (x1 - x0) * u) / 2, oy = (canvas.height - (y1 - y0) * u) / 2;
    for (const l of use) {
      const cell = 1 / l.res;
      for (let j = 0; j < l.rows.length; j++) {
        const row = l.rows[j];
        for (let i = 0; i < row.length; i++) {
          const c = l.colors[row[i]];
          if (row[i] === "." || row[i] === " " || c === undefined || c === 0) continue;
          g.fillStyle = `rgba(${(c >>> 16) & 255},${(c >>> 8) & 255},${c & 255},${((c >>> 24) & 255) / 255})`;
          g.fillRect(Math.floor(ox + (l.x - x0 + i * cell) * u), Math.floor(oy + (l.y - y0 + j * cell) * u), Math.ceil(cell * u), Math.ceil(cell * u));
        }
      }
    }
  }

  window.NyxCosmetics = Cosmetics;
  window.NyxCharacter = NyxCharacter;
  window.NyxLook = NyxLook;
  window.NyxPaintIcon = paintIcon;
  window.NyxEmotes = EMOTES;
})();
