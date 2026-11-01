// node --test port/smoke/web-worker.mjs
import assert from "node:assert/strict";
import test from "node:test";
import { readFile } from "node:fs/promises";

// Pages loads _worker.js as an ES module regardless of the repository's Node package type.
const source = await readFile(new URL("../web/_worker.js", import.meta.url));
const { default: worker } = await import("data:text/javascript;base64," + source.toString("base64"));
const uploaded = new Date("2026-10-01T12:01:02.345Z");
const date = uploaded.toUTCString();
const meta = { etag: "abc", httpEtag: '"abc"', uploaded, size: 10, writeHttpMetadata() {} };
const contents = "0123456789";
const bucket = {
  head: async () => meta,
  async get(_key, {range, onlyIf} = {}) {
    if (range) assert.equal(onlyIf.etagMatches, meta.etag);
    return { ...meta, body: range ? contents.slice(range.offset, range.offset + range.length) : contents };
  },
};
async function read(headers, method = "GET", binding = bucket) {
  return worker.fetch(new Request("https://test.invalid/harvestClientData.zip", { method, headers }),
    { REFLEXIVE: binding });
}

test("partial downloads identify the exact bytes for bounded, open, and suffix ranges", async () => {
  for (const [range, body, expected] of [
    ["bytes=2-4", "234", "bytes 2-4/10"], ["bytes=7-", "789", "bytes 7-9/10"],
    ["bytes=-3", "789", "bytes 7-9/10"], ["bytes=8-99", "89", "bytes 8-9/10"],
  ]) {
    const res = await read({ Range: range, "If-Range": '"abc"' });
    assert.equal(res.status, 206);
    assert.equal(res.headers.get("Content-Range"), expected);
    assert.equal(Number(res.headers.get("Content-Length")), body.length);
    assert.equal(await res.text(), body);
  }
  const unsatisfied = await read({ Range: "bytes=10-" });
  assert.equal(unsatisfied.status, 416);
  assert.equal(unsatisfied.headers.get("Content-Range"), "bytes */10");
  for (const headers of [{ Range: "bytes=2-4", "If-Range": '"old"' }, { Range: "bytes=1-2,6-8" }]) {
    const res = await read(headers);
    assert.equal(res.status, 200);
    assert.equal(await res.text(), contents);
  }
  const head = await read({ Range: "bytes=2-4" }, "HEAD");
  assert.equal(head.status, 200);
  assert.equal(head.headers.get("Content-Length"), "10");
  assert.equal(await head.text(), "");
});

test("GET and HEAD distinguish failed preconditions from cache hits in HTTP precedence order", async () => {
  for (const method of ["GET", "HEAD"]) {
    for (const [checks, status] of [
      [{ "If-Match": '"old"' }, 412], [{ "If-Match": 'W/"abc"' }, 412],
      [{ "If-Match": '"old"', "If-None-Match": '"abc"' }, 412],
      [{ "If-None-Match": '"old", W/"abc"' }, 304], [{ "If-None-Match": "*" }, 304],
      [{ "If-Modified-Since": date }, 304], [{ "If-Unmodified-Since": "Wed, 01 Jan 2020 00:00:00 GMT" }, 412],
      [{ "If-Match": "*", "If-Unmodified-Since": "Wed, 01 Jan 2020 00:00:00 GMT" }, 200],
      [{ "If-None-Match": '"old"', "If-Modified-Since": date }, 200],
    ]) {
      const res = await read(checks, method);
      assert.equal(res.status, status, JSON.stringify(checks));
      if (status !== 200 || method === "HEAD") assert.equal(await res.text(), "");
    }
  }
});

test("an object replaced during a range read never returns stale range headers", async () => {
  const changed = {
    head: bucket.head,
    async get(_key, options) {
      return options?.range ? { ...meta, etag: "new" } : { ...meta, etag: "new", httpEtag: '"new"', size: 3, body: "new" };
    },
  };
  const res = await read({ Range: "bytes=3-5", "If-Range": '"abc"' }, "GET", changed);
  assert.equal(res.status, 200);
  assert.equal(res.headers.get("Content-Range"), null);
  assert.equal(res.headers.get("Content-Length"), "3");
  assert.equal(await res.text(), "new");
});
