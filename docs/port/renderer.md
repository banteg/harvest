# Renderer

The renderer is `daisy::video::CVideoNull` (the driver base, recovered in
[`src/daisy/video/Null/CVideoNull.cpp`](../../src/daisy/video/Null/CVideoNull.cpp)) and
`CVideoOpenGL`, which derives from it. Texture loading is in [textures.md](textures.md) and the 3D
main menu in [menu-scene.md](menu-scene.md). This page covers the driver state the game relies on.

## Package caches

Sprite packages, particle packages and textures each live in a cache: a vector sorted by name,
searched with a binary search. The key is the name exactly as passed, lowercased with `tolower`;
there is no path normalisation, so names keep their alias (for example
`$GAME_RESOURCES$/harvestClientData/gfx/particles.pfx`) and one file reached through two spellings is
loaded twice. Removal matches case-insensitively.

- `getSpritePackage(path, keepStates)` loads on first use with `new CSpritePackage(driver, keepStates)`
  and `load(file, path)`. The flag is the package's keep-states option.
- `getParticlePackage(path)` loads on first use with `new CParticlePackage(driver)` and `load(file)`.
- The cache holds the only reference. The getters return borrowed pointers; `remove*` drops the
  cache's reference and the driver's destructor drops everything. A failed load returns null.
- Loading logs "Loaded …" at information level, and "Could not open file of …" or "Could not load …"
  at error level.

## Screen size and 2D coordinates

- `getScreenSize` returns the render size; `getPhysicalScreenSize` the window size. They are equal
  unless `setRenderScreenSize(w, h)` sets a different render size (w ≤ 0 restores the window size).
  A window resize resets both.
- 2D drawing uses pixels with the origin at the top left and y pointing down.
  `update2dViewValues(ox, oy)` rounds the width and height up to even numbers and maps a pixel to
  clip space as `((x + ox − w/2) / (w/2), (h/2 − oy − y) / (h/2))`.
- 2D quads are batched by texture, up to 1024 quads per batch; `endScene` flushes the last batch.
  `getNumStatusSwitches` returns the previous frame's batch count.

## Other driver state

- **Material renderers.** The built-in ones are registered without names and get the names `solid`
  … `trans_reflection_2layer` (15 entries); `setMaterialRendererName` refuses those indices.
- **Image loaders** are tried by file extension first, then by content, in the order BMP, JPEG, TGA,
  PSD, PCX. `createImageFromData` takes ownership of the buffer.
- **Shader files.** `add*ShaderMaterialFromFiles` reads each file into a NUL-terminated buffer and
  passes it to the driver. The driver base creates the Cg context and destroys it on exit.
- **Frame counting.** `endScene` registers the frame with the FPS counter.

The OpenGL driver's drawing, blending and projection rules will be added here from its recovery.
