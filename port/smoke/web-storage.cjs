// Build with 'just web'. Use Playwright's Chromium ('npx playwright install chromium') or
// set CHROMIUM_EXECUTABLE. Run 'node --test port/smoke/web-storage.cjs' with playwright installed.
const { test, before, after } = require("node:test");
const assert = require("node:assert/strict");
const path = require("node:path");
const { chromium } = require("playwright");

let browser;
before(async () => { browser = await chromium.launch({ executablePath: process.env.CHROMIUM_EXECUTABLE }); });
after(async () => { await browser?.close(); });

async function fixture(t) {
  const ctx = await browser.newContext({ serviceWorkers: "block" });
  t.after(() => ctx.close());
  // Intercept every request in a fresh browser profile. These tests use only synthetic player data.
  const files = {
    "/": ["web/index.html", "text/html"], "/harvest.js": ["zig-out/web/harvest.js", "text/javascript"],
    "/harvest.wasm": ["zig-out/web/harvest.wasm", "application/wasm"],
  };
  await ctx.route("**/*", async (route) => {
    const file = files[new URL(route.request().url()).pathname];
    if (!file) return route.fulfill({ status: 404, body: "" });
    return route.fulfill({ path: path.resolve(__dirname, "..", file[0]), contentType: file[1] });
  });
  return async (init) => {
    const page = await ctx.newPage();
    if (init) await page.addInitScript(init);
    await page.goto("https://harvest-test.invalid/");
    return page;
  };
}

async function ready(page) {
  await page.waitForFunction(() => storageReady && !changingData);
  await page.evaluate(() => { Module.callMain = () => {}; });
}

async function writeSave(page) {
  await page.evaluate(async () => {
    Module.FS.mkdirTree("/data/harvestClientData");
    Module.FS.mkdirTree("/libsdl/Harvest");
    Module.FS.writeFile("/libsdl/Harvest/test.sav", "new save");
    play();
    saveUserData();
    await userSync;
  });
}

test("a queued tab hydrates the previous writer's new save before autosaving", async (t) => {
  const tab = await fixture(t);
  const a = await tab();
  await ready(a);
  const b = await tab();
  await b.waitForFunction(() => state() === "waiting");
  assert.equal(await b.evaluate(() => storageReady), false);
  await writeSave(a);
  await a.close();
  await ready(b);
  assert.equal(await b.evaluate(() => Module.FS.readFile("/libsdl/Harvest/test.sav", { encoding: "utf8" })), "new save");
});

test("a failed initial read cannot start the game or clear persisted saves", async (t) => {
  const tab = await fixture(t);
  const a = await tab();
  await ready(a);
  await writeSave(a);
  await a.close();

  const b = await tab(() => {
    const transaction = IDBDatabase.prototype.transaction;
    let injected = false;
    IDBDatabase.prototype.transaction = function(store, mode, ...args) {
      if (!injected && this.name === "/libsdl" && mode === "readonly") {
        injected = true;
        throw new Error("initial read failed");
      }
      return transaction.call(this, store, mode, ...args);
    };
  });
  await b.waitForFunction(() => state() === "blocked");
  assert.equal(await b.evaluate(async () => {
    play(); saveUserData(); await userSync;
    return storageReady || started;
  }), false);
  // The failed tab is still open; its hydration error released the writer lock.
  const c = await tab();
  await ready(c);
  assert.equal(await c.evaluate(() => exists("/libsdl/Harvest/test.sav")), true);
});

test("a broken replacement keeps old assets and overlapping imports cannot interleave", async (t) => {
  const tab = await fixture(t);
  const a = await tab();
  await ready(a);
  await a.evaluate(async () => {
    Module.FS.mkdirTree("/data/harvestClientData");
    Module.FS.writeFile("/data/harvestClientData/good", "old assets");
    await sync(DATA_ROOT, false);
    await accept(() => [
      { path: "install/harvestClientData/partial", read: () => new Uint8Array([1]) },
      { path: "install/harvestClientData/bad", read: () => { throw new Error("corrupt member"); } },
    ]);
  });
  await a.waitForFunction(() => state() === "ready");
  assert.deepEqual(await a.evaluate(() => [
    Module.FS.readFile("/data/harvestClientData/good", { encoding: "utf8" }),
    exists("/data/harvestClientData/partial"), exists("/data/.import"),
  ]), ["old assets", false, false]);

  await a.evaluate(async () => {
    const put = IDBObjectStore.prototype.put;
    IDBObjectStore.prototype.put = function(value, key) {
      if (key.endsWith("/bad-cache")) throw new DOMException("Quota exhausted", "QuotaExceededError");
      return put.call(this, value, key);
    };
    try {
      await accept(() => [{ path: "harvestClientData/bad-cache", read: () => new Uint8Array([2]) }]);
    } finally {
      IDBObjectStore.prototype.put = put;
    }
  });
  await a.waitForFunction(() => state() === "ready");
  assert.equal(await a.evaluate(() => Module.FS.readFile("/data/harvestClientData/good", { encoding: "utf8" })), "old assets");
  await a.reload();
  await ready(a);
  assert.equal(await a.evaluate(() => Module.FS.readFile("/data/harvestClientData/good", { encoding: "utf8" })), "old assets");

  assert.equal(await a.evaluate(async () => {
    let resume;
    const delayed = new Promise((done) => { resume = done; });
    const first = accept(() => [{ path: "harvestClientData/winner", read: () => delayed }]);
    let ran = false;
    await accept(() => { ran = true; return []; });
    resume(new Uint8Array([42]));
    await first;
    return ran;
  }), false);
  await a.close();
  const b = await tab();
  await ready(b);
  assert.deepEqual(await b.evaluate(() => [
    Array.from(Module.FS.readFile("/data/harvestClientData/winner")),
    exists("/data/harvestClientData/good"), exists("/data/.import"),
  ]), [[42], false, false]);
});
