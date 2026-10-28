importScripts("dockly.js");

function flash(ok) {
  chrome.action.setBadgeBackgroundColor({ color: ok ? "#00b8d9" : "#e5484d" });
  chrome.action.setBadgeText({ text: ok ? "OK" : "!" });
  setTimeout(() => refreshBadge(), 2000);
}

// Toolbar badge for the active tab: a check when the page is already in Dockly, "+" when one of
// your sites covers it. Silent when not paired or Dockly isn't running.
async function refreshBadge() {
  try {
    const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
    if (!tab || !tab.url) return chrome.action.setBadgeText({ text: "" });
    const [m] = await Dockly.match([tab.url]);
    if (m && m.exists) {
      chrome.action.setBadgeBackgroundColor({ color: "#3ddc97" });
      chrome.action.setBadgeText({ text: "\u2713" });
    } else if (m && m.site) {
      chrome.action.setBadgeBackgroundColor({ color: "#0891b2" });
      chrome.action.setBadgeText({ text: "+" });
    } else {
      chrome.action.setBadgeText({ text: "" });
    }
  } catch (e) {
    chrome.action.setBadgeText({ text: "" });
  }
}

chrome.tabs.onActivated.addListener(refreshBadge);
chrome.tabs.onUpdated.addListener((id, change, tab) => { if (change.status === "complete" && tab.active) refreshBadge(); });
chrome.windows.onFocusChanged.addListener(refreshBadge);

chrome.runtime.onInstalled.addListener(() => {
  chrome.contextMenus.removeAll(() => {
    chrome.contextMenus.create({ id: "dockly-link", title: "Add link to Dockly", contexts: ["link"] });
    chrome.contextMenus.create({ id: "dockly-page", title: "Add this page to Dockly", contexts: ["page"] });
    chrome.contextMenus.create({ id: "dockly-tabs", title: "Add all tabs in this window to Dockly", contexts: ["page", "action"] });
  });
});

async function addTabs(windowId) {
  const tabs = await chrome.tabs.query({ windowId });
  return Dockly.add(tabs.map((t) => ({ url: t.url, title: t.title })));
}

chrome.contextMenus.onClicked.addListener(async (info, tab) => {
  try {
    if (info.menuItemId === "dockly-tabs") await addTabs(tab.windowId);
    else await Dockly.add([info.menuItemId === "dockly-link"
      ? { url: info.linkUrl, title: info.selectionText || "" }
      : { url: tab.url, title: tab.title }]);
    flash(true);
  } catch (e) {
    flash(false);
  }
});

// Keyboard shortcuts (change them at chrome://extensions/shortcuts)
chrome.commands.onCommand.addListener(async (command) => {
  try {
    const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
    if (command === "add-page") await Dockly.add([{ url: tab.url, title: tab.title }]);
    else if (command === "add-tabs") await addTabs(tab.windowId);
    flash(true);
  } catch (e) {
    flash(false);
  }
});
