// The browser downloads the public game data from its own origin. R2 stays behind the Worker.
export default {
  async fetch(request, env) {
    if (new URL(request.url).pathname !== "/harvestClientData.zip") return env.ASSETS.fetch(request);
    if (request.method !== "GET" && request.method !== "HEAD") {
      return new Response("Method not allowed", { status: 405, headers: { Allow: "GET, HEAD" } });
    }
    const object = await env.REFLEXIVE.get("harvest/harvestClientData.zip");
    if (!object) return new Response("Game data not found", { status: 404 });
    const headers = new Headers();
    object.writeHttpMetadata(headers);
    headers.set("Content-Type", "application/zip");
    headers.set("Content-Length", String(object.size));
    headers.set("ETag", object.httpEtag);
    headers.set("Cache-Control", "public, max-age=3600");
    return new Response(request.method === "HEAD" ? null : object.body, { headers });
  },
};
