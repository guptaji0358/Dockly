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
    status("Connected to Dockly " + info.version, "ok");
  } catch (e) {
    status(e.message, "err");
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
    await Dockly.add([{ url: tab.url, title: $("title").value.trim() || tab.title, icon: iconData() }], $("collection").value);
    status("Added to Dockly.", "ok");
  } catch (e) { status(e.message, "err"); }
}

async function addTabs() {
  const tabs = await chrome.tabs.query({ currentWindow: true });
  try {
    const r = await Dockly.add(tabs.map((t) => ({ url: t.url, title: t.title })), $("collection").value);
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
show();
