// Talks to the Dockly app on this PC. Dockly listens on 127.0.0.1 only; the pairing code copied
// from Dockly (Connectors page) holds the port and a secret token.
const Dockly = {
  async config() {
    const { pair } = await chrome.storage.local.get("pair");
    if (!pair || pair.indexOf(".") < 1) return null;
    const i = pair.indexOf(".");
    return { port: pair.slice(0, i), token: pair.slice(i + 1) };
  },

  async call(path, body) {
    const c = await this.config();
    if (!c) throw new Error("Not paired yet - paste the pairing code from Dockly.");
    let res;
    try {
      res = await fetch(`http://127.0.0.1:${c.port}${path}`, {
        method: body ? "POST" : "GET",
        headers: { Authorization: "Bearer " + c.token, "Content-Type": "application/json" },
        body: body ? JSON.stringify(body) : undefined,
      });
    } catch (e) {
      throw new Error("Dockly isn't running, or the browser connector is switched off.");
    }
    const json = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(json.error || "Dockly answered " + res.status);
    return json;
  },

  ping() { return this.call("/ping"); },
  add(items, collection) {
    const list = items
      .filter((t) => /^https?:\/\//i.test(t.url))
      .map((t) => ({ url: t.url, title: t.title || "", collection, icon: t.icon }));
    return this.call("/add", { items: list });
  },
};
