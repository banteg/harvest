// The browser downloads the public game data from its own origin. R2 stays behind the Worker.
const key = "harvest/harvestClientData.zip";

function headersFor(object) {
  const headers = new Headers();
  object.writeHttpMetadata(headers);
  headers.set("Content-Type", "application/zip");
  headers.set("ETag", object.httpEtag);
  headers.set("Last-Modified", object.uploaded.toUTCString());
  headers.set("Cache-Control", "public, max-age=3600");
  headers.set("Accept-Ranges", "bytes");
  return headers;
}

function conditionStatus(headers, object) {
  const modified = Date.parse(object.uploaded.toUTCString()); // HTTP dates have second precision
  function matches(value, weak) {
    if (value.trim() === "*") return true;
    const tags = value.match(/(?:W\/)?"[^"]*"/g) || [];
    return tags.some((tag) => (weak ? tag.replace(/^W\//, "") : tag) === object.httpEtag);
  }
  const ifMatch = headers.get("If-Match");
  if (ifMatch !== null ? !matches(ifMatch, false) : modified > Date.parse(headers.get("If-Unmodified-Since")))
    return 412;
  const ifNone = headers.get("If-None-Match");
  if (ifNone !== null ? matches(ifNone, true) : modified <= Date.parse(headers.get("If-Modified-Since")))
    return 304;
  return 200;
}

// A single range. Ignore malformed or multipart ranges and serve the full representation.
function byteRange(header, size) {
  const m = /^bytes=(\d*)-(\d*)$/i.exec(header.trim());
  if (!m || (!m[1] && !m[2])) return null;
  const offset = m[1] ? Number(m[1]) : Math.max(0, size - Number(m[2]));
  const end = m[1] && m[2] ? Math.min(Number(m[2]), size - 1) : size - 1;
  return { offset, length: Math.max(0, end - offset + 1) };
}

export default {
  async fetch(request, env) {
    if (new URL(request.url).pathname !== "/harvestClientData.zip") return env.ASSETS.fetch(request);
    if (request.method !== "GET" && request.method !== "HEAD")
      return new Response("Method not allowed", { status: 405, headers: { Allow: "GET, HEAD" } });
    const bucket = env.REFLEXIVE;
    const head = request.method === "HEAD";
    const rangeHeader = request.headers.get("Range");
    const conditional = ["If-Match", "If-Unmodified-Since", "If-None-Match", "If-Modified-Since"]
      .some((name) => request.headers.has(name));
    if (head || rangeHeader !== null || conditional) {
      const meta = await bucket.head(key);
      if (!meta) return new Response(null, { status: 404 });
      const headers = headersFor(meta);
      const status = conditionStatus(request.headers, meta);
      if (status !== 200) return new Response(null, { status, headers });
      if (head) {
        headers.set("Content-Length", String(meta.size));
        return new Response(null, { headers });
      }
      const validator = request.headers.get("If-Range");
      if (rangeHeader !== null && (!validator || validator === meta.httpEtag || validator === headers.get("Last-Modified"))) {
        const range = byteRange(rangeHeader, meta.size);
        if (range) {
          if (!range.length) {
            headers.set("Content-Range", `bytes */${meta.size}`);
            return new Response(null, { status: 416, headers });
          }
          const part = await bucket.get(key, { range, onlyIf: { etagMatches: meta.etag } });
          if (part && "body" in part) {
            headers.set("Content-Range", `bytes ${range.offset}-${range.offset + range.length - 1}/${meta.size}`);
            headers.set("Content-Length", String(range.length));
            return new Response(part.body, { status: 206, headers });
          }
          // The object changed between the two operations. Re-evaluate the full GET below.
        }
      }
    }
    const object = await bucket.get(key);
    if (!object) return new Response(null, { status: 404 });
    const headers = headersFor(object);
    const status = conditionStatus(request.headers, object);
    if (status !== 200) return new Response(null, { status, headers });
    headers.set("Content-Length", String(object.size));
    return new Response(object.body, { headers });
  },
};
