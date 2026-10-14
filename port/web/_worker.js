// The browser downloads the public game data from its own origin. R2 stays behind the Worker.
export default {
  async fetch(request, env) {
    if (new URL(request.url).pathname !== "/harvestClientData.zip") return env.ASSETS.fetch(request);
    if (request.method !== "GET" && request.method !== "HEAD") {
      return new Response("Method not allowed", { status: 405, headers: { Allow: "GET, HEAD" } });
    }
    const key = "harvest/harvestClientData.zip";
    const object = request.method === "HEAD"
      ? await env.REFLEXIVE.head(key)
      : await env.REFLEXIVE.get(key, { onlyIf: request.headers, range: request.headers });
    if (!object) return new Response("Game data not found", { status: 404 });
    const headers = new Headers();
    object.writeHttpMetadata(headers);
    headers.set("Content-Type", "application/zip");
    headers.set("ETag", object.httpEtag);
    headers.set("Cache-Control", "public, max-age=3600");
    if (request.method === "HEAD") {
      headers.set("Content-Length", String(object.size));
      return new Response(null, { headers });
    }
    if (!("body" in object)) return new Response(null, { status: 304, headers });
    const length = object.range && "length" in object.range ? object.range.length : object.size;
    headers.set("Content-Length", String(length));
    const status = request.headers.has("range") && object.range ? 206 : 200;
    return new Response(object.body, { status, headers });
  },
};
