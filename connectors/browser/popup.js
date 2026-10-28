const $ = (id) => document.getElementById(id);

function status(text, cls) {
  const s = $("status");
  s.textContent = text;
  s.className = cls || "";
}

async function show() {
  const cfg = await Dockly.config();
  $("main").hidden = !cfg;
  $("pair").hidden = !!cfg;
  $("unpair").hidden = !cfg;
  if (!cfg) return;

  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  $("title").value = tab.title || "";
  $("url").textContent = tab.url || "";
  favicon = tab.favIconUrl || "";
  drawPreview();
  try {
    const info = await Dockly.ping();
    $("collection").length = 1;
    for (const c of info.collections) {
      const o = document.createElement("option");
      o.value = c.id;
      o.textContent = c.name;
      $("collection").appendChild(o);
    }
    fillPlacement(info.sites || []);
    fillTree(info.sites || []);
    refreshMatch(tab.url);
    status("Connected to Dockly " + info.version, "ok");
  } catch (e) {
    status(e.message, "err");
  }
}

let current = { match: null, tab: null };

// Groups of every site, so a page can be filed somewhere other than where the rules would put it.
function fillPlacement(sites) {
  const sel = $("placement");
  sel.length = 2;
  for (const site of sites) {
    if (!site.groups.length) continue;
    const og = document.createElement("optgroup");
    og.label = site.name;
    for (const g of site.groups) {
      const o = document.createElement("option");
      o.value = g.name;
      o.textContent = g.name;
      og.appendChild(o);
    }
    sel.appendChild(og);
  }
}

// "Your sites": every site, its groups and their links; clicking a link opens it in a new tab.
function fillTree(sites) {
  const root = $("siteTree");
  root.textContent = "";
  const shown = sites.filter((s) => s.groups.length);
  $("mySites").hidden = !shown.length;
  for (const site of shown) {
    const d = document.createElement("details");
    const sm = document.createElement("summary");
    sm.textContent = site.name + " (" + site.groups.length + ")";
    d.appendChild(sm);
    for (const g of site.groups) {
      const gd = document.createElement("details");
      const gs = document.createElement("summary");
      gs.textContent = g.name;
      gd.appendChild(gs);
      for (const l of g.links) {
        const a = document.createElement("a");
        a.href = l.url;
        a.textContent = l.name || l.url;
        a.title = l.url;
        a.onclick = (e) => { e.preventDefault(); chrome.tabs.create({ url: l.url }); };
        gd.appendChild(a);
      }
      d.appendChild(gd);
    }
    root.appendChild(d);
  }
}

// Which site would take this page, is it saved already, and offer to make a site for a new host.
async function refreshMatch(url) {
  $("site").hidden = true;
  $("newSite").hidden = true;
  $("siteForm").hidden = true;
  let m;
  try { [m] = await Dockly.match([url]); } catch (e) { return; }
  if (!m) return;
  current.match = m;
  const bits = [];
  if (m.exists) bits.push("Already in Dockly");
  if (m.site) bits.push("site: " + m.site + (m.group ? " \u203a " + m.group : ""));
  $("site").textContent = bits.join(" \u00b7 ");
  $("site").hidden = !bits.length;
  if (!m.site && m.host) {
    const label = m.host.split(".").slice(-2, -1)[0] || m.host;
    $("siteName").value = label.charAt(0).toUpperCase() + label.slice(1);
    $("makeSite").textContent = "+ Make " + m.host + " a site";
    $("newSite").hidden = false;
  }
}

let uploaded = null; // PNG data URL of the chosen image
let favicon = ""; // the current page's own icon, shown for "Automatic"

function drawPreview() {
  const cv = $("preview"), g = cv.getContext("2d");
  g.clearRect(0, 0, 48, 48);
  if ($("iconMode").value === "tile") {
    g.fillStyle = $("color").value;
    g.beginPath(); g.roundRect(0, 0, 48, 48, 10); g.fill();
    g.fillStyle = "#fff"; g.font = "bold 28px Segoe UI, sans-serif";
    g.textAlign = "center"; g.textBaseline = "middle";
    const ch = ($("title").value.trim()[0] || "?").toUpperCase();
    g.fillText(ch, 24, 26);
  } else {
    const src = $("iconMode").value === "upload" ? uploaded : favicon;
    if (!src) return;
    const img = new Image();
    img.onload = () => g.drawImage(img, 0, 0, 48, 48);
    img.src = src;
  }
}

// Render the chosen icon at 128px for sending; null = let Dockly pick the site icon.
function iconData() {
  const mode = $("iconMode").value;
  if (mode === "auto") return undefined;
  if (mode === "upload") return uploaded || undefined;
  const cv = document.createElement("canvas");
  cv.width = cv.height = 128;
  const g = cv.getContext("2d");
  g.fillStyle = $("color").value;
  g.beginPath(); g.roundRect(0, 0, 128, 128, 26); g.fill();
  g.fillStyle = "#fff"; g.font = "bold 76px Segoe UI, sans-serif";
  g.textAlign = "center"; g.textBaseline = "middle";
  g.fillText(($("title").value.trim()[0] || "?").toUpperCase(), 64, 70);
  return cv.toDataURL("image/png");
}

function loadImage(file) {
  const r = new FileReader();
  r.onload = () => {
    const img = new Image();
    img.onload = () => {
      const cv = document.createElement("canvas");
      cv.width = cv.height = 128;
      const s = Math.min(img.width, img.height);
      cv.getContext("2d").drawImage(img, (img.width - s) / 2, (img.height - s) / 2, s, s, 0, 0, 128, 128);
      uploaded = cv.toDataURL("image/png");
      drawPreview();
    };
    img.src = r.result;
  };
  r.readAsDataURL(file);
}

$("iconMode").onchange = () => {
  const m = $("iconMode").value;
  $("color").hidden = m !== "tile";
  if (m === "upload") $("file").click();
  drawPreview();
};
$("color").oninput = drawPreview;
$("title").oninput = drawPreview;
$("file").onchange = () => { if ($("file").files[0]) loadImage($("file").files[0]); };

async function addPage() {
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  try {
    await Dockly.add([{ url: tab.url, title: $("title").value.trim() || tab.title, icon: iconData() }], $("collection").value, $("placement").value);
    status("Added to Dockly.", "ok");
    refreshMatch(tab.url);
  } catch (e) { status(e.message, "err"); }
}

async function addTabs() {
  const tabs = await chrome.tabs.query({ currentWindow: true });
  try {
    const r = await Dockly.add(tabs.map((t) => ({ url: t.url, title: t.title })), $("collection").value, $("placement").value);
    status("Added " + r.added + " tab(s) to Dockly.", "ok");
  } catch (e) { status(e.message, "err"); }
}

$("addPage").onclick = addPage;
$("addTabs").onclick = addTabs;
$("save").onclick = async () => {
  await chrome.storage.local.set({ pair: $("code").value.trim() });
  status("");
  show();
};
$("unpair").onclick = async (e) => {
  e.preventDefault();
  await chrome.storage.local.remove("pair");
  show();
};

$("makeSite").onclick = (e) => { e.preventDefault(); $("siteForm").hidden = !$("siteForm").hidden; };
$("createSite").onclick = async () => {
  const m = current.match;
  try {
    const r = await Dockly.addSite({ name: $("siteName").value.trim(), hosts: m.host, groupBy: $("siteGroupBy").value });
    status("Site created" + (r.organized ? "; filed " + r.organized + " existing link(s)." : "."), "ok");
    show();
  } catch (err) { status(err.message, "err"); }
};

// Choose which tabs of this window to add; ones already in Dockly start unticked.
$("pickTabs").onclick = async () => {
  const list = $("tabList");
  if (!list.hidden) { list.hidden = true; return; }
  const tabs = (await chrome.tabs.query({ currentWindow: true })).filter((t) => /^https?:\/\//i.test(t.url));
  let known = [];
  try { known = await Dockly.match(tabs.map((t) => t.url)); } catch (e) {}
  const have = new Set(known.filter((k) => k.exists).map((k) => k.url));
  const box = $("tabs");
  box.textContent = "";
  for (const t of tabs) {
    const row = document.createElement("label");
    const cb = document.createElement("input");
    cb.type = "checkbox";
    cb.checked = !have.has(t.url);
    cb.dataset.url = t.url;
    cb.dataset.title = t.title || "";
    const sp = document.createElement("span");
    sp.textContent = t.title || t.url;
    sp.title = t.url;
    row.append(cb, sp);
    if (have.has(t.url)) { const em = document.createElement("em"); em.textContent = "saved"; row.appendChild(em); }
    box.appendChild(row);
  }
  list.hidden = false;
};
$("addSelected").onclick = async () => {
  const picked = [...$("tabs").querySelectorAll("input:checked")].map((c) => ({ url: c.dataset.url, title: c.dataset.title }));
  if (!picked.length) { status("Tick at least one tab.", "err"); return; }
  try {
    const r = await Dockly.add(picked, $("collection").value, $("placement").value);
    status("Added " + r.added + " tab(s) to Dockly.", "ok");
    $("tabList").hidden = true;
    show();
  } catch (e) { status(e.message, "err"); }
};
show();
