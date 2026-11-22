/*
 * Nyxora for the iPhone: the menu on the first page of the app (ios/native/NyxoraHomeViewController.m shows this page
 * in a web view and answers its requests). The lobby: the player in 3D with the wardrobe's accessories, the account,
 * sounds and music, Play; the wardrobe: outfits and every item, tried on in 3D. What is worn is kept by the app, and
 * the look for the game (the Nyxora Cosmetics mod) goes into every start of a Fabric or Quilt 1.20.1 profile
 * (ios/native/NyxoraLook.m). Opened in a browser, the page works on its own (the app's parts are stood in for).
 */
(function () {
  "use strict";

  const $ = (id) => document.getElementById(id);
  const C = window.NyxCosmetics;
  C.load(window.NYX_COSMETICS || { items: [], outfits: [] });

  /* ------------------------------------------------------------ the app (WKWebView) or a browser */

  const native = window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.nyxora;
  const waiting = {};
  let nextId = 1;
  /** a request to the app: the answer comes back through window.nyxoraReply(id, json) */
  function call(action, data) {
    if (!native) return Promise.resolve(browserStandIn(action, data));
    const id = nextId++;
    return new Promise((resolve) => {
      waiting[id] = resolve;
      native.postMessage({ id, action, data: data || {} });
    });
  }
  window.nyxoraReply = (id, result) => {
    const done = waiting[id];
    delete waiting[id];
    if (done) done(result);
  };
  /** the app tells the page that something changed (the account, the profile) */
  window.nyxoraChanged = () => refresh();
  /** the app tells the page that it is out of sight (another page of the app, the game) or back */
  let hidden = false;
  window.nyxoraHidden = (h) => {
    hidden = !!h;
    if (hidden) music.pause(); else startMusic();
  };

  function browserStandIn(action, data) {
    if (action === "state") {
      let worn = {};
      try { worn = JSON.parse(localStorage.getItem("nyxora.worn") || "{}"); } catch (e) { /* none yet */ }
      return { account: null, profile: { name: "1.20.1 Fabric", version: "fabric-loader-1.20.1", look: true }, worn };
    }
    if (action === "saveWorn") {
      try { localStorage.setItem("nyxora.worn", JSON.stringify(data.worn)); } catch (e) { /* private mode */ }
    }
    return { ok: true };
  }

  /* ------------------------------------------------------------ what is worn */

  const SLOTS = ["head", "body", "back", "pet", "hand", "effect"];
  let worn = {};
  let account = null, skinImage = null, capeImage = null, slim = false;

  function wear(slot, id) {
    worn[slot] = id || "";
    saveSoon();
  }
  function wearOutfit(o) {
    for (const s of SLOTS) worn[s] = o[s] || "";
    saveSoon();
  }

  let saveTimer = 0;
  function saveSoon() {
    clearTimeout(saveTimer);
    saveTimer = setTimeout(save, 350);
  }
  function save() {
    clearTimeout(saveTimer);
    saveTimer = 0;
    call("saveWorn", { worn });
    const out = window.NyxLook.export({ head: worn.head, body: worn.body, back: worn.back }, skinImage, slim, capeImage);
    call("saveLook", { look: out.look, skin: out.skin, cape: out.cape });
  }

  /* ------------------------------------------------------------ sounds and music */

  const sounds = {};
  let soundOn = localStorage.getItem("nyxora.sound") !== "off";
  let musicOn = localStorage.getItem("nyxora.music") !== "off";
  const music = new Audio("music/theme.m4a");
  music.loop = true;
  music.volume = 0.35;
  function play(name) {
    if (!soundOn) return;
    let a = sounds[name];
    if (!a) a = sounds[name] = new Audio(`sounds/${name}.wav`);
    try { a.currentTime = 0; a.volume = 0.6; a.play().catch(() => {}); } catch (e) { /* not ready */ }
  }
  function startMusic() {
    if (musicOn && !hidden && !document.hidden && music.paused) music.play().catch(() => {});
  }
  // iOS plays sound only after a touch
  document.addEventListener("pointerdown", startMusic, { once: true });
  document.addEventListener("visibilitychange", () => { if (document.hidden) music.pause(); else startMusic(); });

  function toggles() {
    $("sound").classList.toggle("off", !soundOn);
    $("music").classList.toggle("off", !musicOn);
  }
  $("sound").onclick = () => {
    soundOn = !soundOn;
    try { localStorage.setItem("nyxora.sound", soundOn ? "on" : "off"); } catch (e) { /* private mode */ }
    toggles();
    play("toggle");
  };
  $("music").onclick = () => {
    musicOn = !musicOn;
    try { localStorage.setItem("nyxora.music", musicOn ? "on" : "off"); } catch (e) { /* private mode */ }
    toggles();
    if (musicOn) startMusic(); else music.pause();
    play("toggle");
  };

  /* ------------------------------------------------------------ the neon rain behind everything */

  function rain() {
    const canvas = $("rain"), g = canvas.getContext("2d"), start = performance.now();
    const WIND = 0.16;
    const unit = (a, b) => {
      let h = (Math.imul(a, 374761393) + Math.imul(b, 668265263) + 0x2545F491) | 0;
      h = Math.imul(h ^ (h >>> 13), 1274126177);
      return (((h ^ (h >>> 16)) & 0x7fffffff) % 10000) / 10000;
    };
    function frame() {
      const dpr = window.devicePixelRatio || 1;
      const w = Math.round(innerWidth * dpr), h = Math.round(innerHeight * dpr);
      if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
      const t = (performance.now() - start) / 1000;
      const sky = g.createLinearGradient(0, 0, 0, h);
      sky.addColorStop(0, "#010103"); sky.addColorStop(0.55, "#04030b"); sky.addColorStop(1, "#06051a");
      g.fillStyle = sky; g.fillRect(0, 0, w, h);
      const glow = g.createRadialGradient(w / 2, h * 1.15, 0, w / 2, h * 1.15, Math.max(w, h) * 0.75);
      glow.addColorStop(0, "rgba(94,48,219,0.25)"); glow.addColorStop(0.5, "rgba(62,27,158,0.09)"); glow.addColorStop(1, "rgba(0,0,0,0)");
      g.fillStyle = glow; g.fillRect(0, 0, w, h);
      const cycle = t % 9.5;
      if (cycle < 0.35) {
        g.fillStyle = `rgba(134,72,255,${(40 * (1 - cycle / 0.35) * (cycle < 0.08 ? cycle / 0.08 : 1)) / 255})`;
        g.fillRect(0, 0, w, h);
      }
      const drops = Math.min(300, Math.floor((w * h) / (1300 * dpr * dpr)));
      g.lineCap = "round";
      for (let i = 0; i < drops; i++) {
        const near = unit(i, 1) < 0.3;
        const speed = h * (near ? 1.5 : 0.95) * (0.85 + unit(i, 2) * 0.3);
        const length = (near ? 26 : 14) * dpr * (0.7 + unit(i, 3) * 0.6);
        const span = h + length;
        const y = ((t * speed + unit(i, 4) * span) % span) - length;
        const x = unit(i, 5) * (w + h * WIND) - h * WIND + y * WIND;
        const alpha = (near ? 90 + unit(i, 6) * 70 : 35 + unit(i, 6) * 45) / 255;
        const blue = unit(i, 7) < 0.34;
        g.strokeStyle = blue ? `rgba(86,${near ? 190 : 170},255,${alpha})` : `rgba(${near ? 150 : 130},${near ? 100 : 90},255,${alpha})`;
        g.lineWidth = (near ? 1.8 : 1.0) * dpr;
        g.beginPath(); g.moveTo(x, y); g.lineTo(x + length * WIND, y + length); g.stroke();
      }
      requestAnimationFrame(frame);
    }
    requestAnimationFrame(frame);
  }

  /* ------------------------------------------------------------ images */

  function image(url) {
    return new Promise((resolve) => {
      if (!url) return resolve(null);
      const img = new Image();
      img.onload = () => resolve(img);
      img.onerror = () => resolve(null);
      img.src = url;
    });
  }

  function paintHead(canvas, img) {
    const g = canvas.getContext("2d");
    g.imageSmoothingEnabled = false;
    g.clearRect(0, 0, canvas.width, canvas.height);
    if (!img) {
      g.fillStyle = "#110e2b"; g.fillRect(0, 0, 32, 32);
      g.fillStyle = "#9a62ff"; g.fillRect(8, 16, 4, 4); g.fillRect(20, 16, 4, 4);
      return;
    }
    g.drawImage(img, 8, 8, 8, 8, 0, 0, 32, 32);
    g.drawImage(img, 40, 8, 8, 8, 0, 0, 32, 32);
  }

  /* ------------------------------------------------------------ the lobby */

  const player = new window.NyxCharacter($("player"), { worn: () => worn, onTap: () => play("toggle") });
  const dresser = new window.NyxCharacter($("dresser"), { worn: () => worn, onTap: () => play("toggle") });

  async function refresh() {
    const state = (await call("state")) || {};
    account = state.account || null;
    // (a change about to be saved stays)
    if (state.worn && typeof state.worn === "object" && !saveTimer) worn = Object.assign({}, state.worn);
    skinImage = await image(account && account.skin);
    capeImage = await image(account && account.cape);
    slim = !!(account && account.slim);
    for (const view of [player, dresser]) {
      view.setSkin(skinImage, slim);
      view.setCape(capeImage);
      view.nametag = account ? account.name : "";
    }
    $("accountName").textContent = account ? account.name : "Войти";
    $("accountKind").textContent = !account ? "аккаунт Microsoft"
      : account.demo ? "демо-версия" : account.kind === "local" ? "без входа" : "Microsoft";
    paintHead($("head"), skinImage);
    const profile = state.profile;
    $("profileLine").textContent = profile ? `Сборка: ${profile.name}` : "";
    $("lookStatus").textContent = profile && profile.look
      ? "Твой образ будет виден в этой сборке: скин, плащ и вещи головы, тела и спины."
      : "Образ виден в игре на Fabric и Quilt 1.20.1: скин, плащ и вещи головы, тела и спины.";
    const icon = C.find(worn.back) || C.find(worn.head) || C.inSlot("back")[0];
    window.NyxPaintIcon($("wardrobeIcon"), icon);
    $("wardrobeCount").textContent = `${C.items.filter((i) => i.slot !== "emote").length} вещей — всё открыто`;
    // the look for the game follows the current skin and what is worn
    save();
  }

  $("account").onclick = () => { play("open"); call("open", { screen: "accounts" }); };
  $("play").onclick = async () => {
    play("launch");
    const done = await call("play");
    if (done && done.ok === false) {
      // the app is still loading the list of versions
      $("profileLine").textContent = "Подожди пару секунд: загружается список версий";
      setTimeout(refresh, 2500);
    }
  };
  $("openWardrobe").onclick = () => { play("open"); show("wardrobe"); };
  $("back").onclick = () => { play("close"); show("lobby"); };
  $("takeOff").onclick = () => {
    for (const s of SLOTS) worn[s] = "";
    dresser.preview(null, null);
    saveSoon();
    play("close");
    fillGrid();
  };

  function show(name) {
    for (const s of document.querySelectorAll(".screen")) s.hidden = s.id !== name;
    if (name === "wardrobe") { dresser.run(); fillTabs(); fillGrid(); } else { dresser.preview(null, null); }
  }

  /* ------------------------------------------------------------ the wardrobe */

  const TABS = [
    { id: "outfits", title: "Образы" }, { id: "head", title: "Голова" }, { id: "body", title: "Тело" }, { id: "back", title: "Спина" },
    { id: "hand", title: "В руке" }, { id: "pet", title: "Питомцы" }, { id: "effect", title: "Эффекты" }, { id: "emote", title: "Эмоции" },
  ];
  let tab = "outfits";

  function fillTabs() {
    const nav = $("tabs");
    nav.textContent = "";
    for (const t of TABS) {
      const b = document.createElement("button");
      b.type = "button";
      b.className = "tab" + (t.id === tab ? " on" : "");
      b.textContent = t.title;
      b.onclick = () => { tab = t.id; play("nav"); fillTabs(); fillGrid(); dresser.setBack(t.id === "back"); };
      nav.appendChild(b);
    }
  }

  function tile(item, on) {
    const b = document.createElement("button");
    b.type = "button";
    b.className = "item" + (on ? " on" : "");
    b.style.borderColor = C.rarityColor(item.rarity);
    const c = document.createElement("canvas");
    c.width = 64; c.height = 64;
    window.NyxPaintIcon(c, item);
    const name = document.createElement("b");
    name.textContent = item.title;
    const kind = document.createElement("small");
    kind.textContent = on ? "надето" : C.rarityName(item.rarity);
    kind.style.color = C.rarityColor(item.rarity);
    b.append(c, name, kind);
    return b;
  }

  function fillGrid() {
    const grid = $("grid");
    grid.textContent = "";
    if (tab === "outfits") {
      for (const o of C.outfits) {
        const b = document.createElement("button");
        b.type = "button";
        const all = SLOTS.every((s) => (o[s] || "") === (worn[s] || ""));
        b.className = "item outfit" + (all ? " on" : "");
        const pics = document.createElement("div");
        pics.className = "pics";
        for (const s of ["head", "back", "body", "pet"]) {
          const c = document.createElement("canvas");
          c.width = 48; c.height = 48;
          window.NyxPaintIcon(c, C.find(o[s]));
          pics.appendChild(c);
        }
        const name = document.createElement("b");
        name.textContent = o.title;
        b.append(pics, name);
        b.onclick = () => { wearOutfit(o); play("wear"); dresser.randomEmote(); fillGrid(); };
        grid.appendChild(b);
      }
      return;
    }
    if (tab === "emote") {
      for (const item of C.inSlot("emote")) {
        const b = document.createElement("button");
        b.type = "button";
        b.className = "item emote";
        b.textContent = item.title;
        b.onclick = () => { dresser.playEmote(item.id); play("toggle"); };
        grid.appendChild(b);
      }
      return;
    }
    const items = C.inSlot(tab);
    if (!items.length) {
      const p = document.createElement("p");
      p.className = "empty";
      p.textContent = "Здесь пока пусто";
      grid.appendChild(p);
      return;
    }
    for (const item of items) {
      const on = worn[tab] === item.id;
      const b = tile(item, on);
      b.onclick = () => {
        wear(tab, on ? "" : item.id);
        play(on ? "close" : "wear");
        fillGrid();
      };
      grid.appendChild(b);
    }
  }

  /* ------------------------------------------------------------ start */

  toggles();
  rain();
  player.run();
  refresh();
})();
