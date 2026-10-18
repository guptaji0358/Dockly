importScripts("dockly.js");

function flash(ok) {
  chrome.action.setBadgeBackgroundColor({ color: ok ? "#00b8d9" : "#e5484d" });
  chrome.action.setBadgeText({ text: ok ? "OK" : "!" });
  setTimeout(() => chrome.action.setBadgeText({ text: "" }), 2000);
}

chrome.runtime.onInstalled.addListener(() => {
  chrome.contextMenus.create({ id: "dockly-link", title: "Add link to Dockly", contexts: ["link"] });
  chrome.contextMenus.create({ id: "dockly-page", title: "Add this page to Dockly", contexts: ["page"] });
});

chrome.contextMenus.onClicked.addListener(async (info, tab) => {
  const item = info.menuItemId === "dockly-link"
    ? { url: info.linkUrl, title: info.selectionText || "" }
    : { url: tab.url, title: tab.title };
  try {
    await Dockly.add([item]);
    flash(true);
  } catch (e) {
    flash(false);
  }
});
