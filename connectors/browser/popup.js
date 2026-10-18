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
  $("title").textContent = tab.title || "";
  $("url").textContent = tab.url || "";
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

async function addPage() {
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  try {
    await Dockly.add([{ url: tab.url, title: tab.title }], $("collection").value);
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
