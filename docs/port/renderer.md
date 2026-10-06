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

## OpenGL drawing

The Linux driver is `CVideoOpenGL` with its material renderers
([`src/daisy/video/OpenGL/`](../../src/daisy/video/OpenGL/)), all fixed-function OpenGL.

### 2D images

- The 2D matrices are identity; vertices are placed with the mapping above, so integer coordinates
  fall on pixel edges.
- `draw2DImage(texture, position, sourceRect, clipRect, color, useAlpha)` clips to the clip rectangle
  and then to the render size, as Irrlicht 0.7 does. Texture coordinates are inset by half a texel
  (`(sx + 0.5) / W` to `(sx + w − 0.5) / W`, the same for v); positions are not shifted; filtering is
  nearest.
- The colour-array variant uses texture coordinates `(sx + 0.5) / W` to `(sx + w + 0.5) / W`, shifts
  positions by +0.5 px, and assigns the colours to the corners in the order c[0], c[3], c[2], c[1]
  (default white).
- The corner variants take upper-left, upper-right, lower-left and lower-right colours (default
  `0x00FFFFFF`, transparent white) with half-texel-inset texture coordinates. The integer version
  shifts by +0.5 px and filters linearly only when the quad is an axis-aligned rectangle; the float
  version always filters linearly and does not shift. `drawScaled2DImage` uses the float corner
  version with one colour.
- Quads are batched, up to 1024 per batch. A batch is flushed when the texture, the alpha-channel
  setting or the filter changes, when it is full, or when the scissor changes. With an alpha channel
  a batch draws with `GL_MODULATE`, blending `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` and alpha test > 0;
  without one, blending stays on with whatever blend function was set last. `setForcePointSampling`
  forces nearest filtering.
- Untextured rectangles and lines blend with `SRC_ALPHA` only when the colour's alpha is below 255.
  Lines are always 1 px wide (see [original-bugs.md](original-bugs.md)).

### Render targets and scissor

- `setRenderTarget` returns true and does nothing: there are no render targets on Linux, so the
  minimap, which `CPlayState` renders "into" a texture, is drawn straight to the screen.
  `createScreenTexture` returns an empty texture.
- `setScissorRect` uses a wrong y (see [original-bugs.md](original-bugs.md)); the shuttle race's split
  screen is the only user and passes full-height rectangles.

### 3D state

- Setting the view or world matrix loads modelview = view · world; the projection matrix is loaded
  with its z column negated. `setViewPort` clips to the render size, with y measured from the bottom.
- Materials: `EMT_SOLID` uses `GL_DECAL`, so lighting does not show on solid textured objects. Every
  `EMT_TRANSPARENT_*` type blends `GL_ONE / GL_ONE_MINUS_SRC_COLOR` with depth writes off
  (`TRANSPARENT_VERTEX_ALPHA` is the same as `ADD_COLOR`); `ALPHA_CHANNEL` adds alpha test > 0. The
  two-layer, light-map and sphere-map types follow Irrlicht 0.7.
- Material flags: 6 sets `glFrontFace` to counter-clockwise (the menu atmosphere uses it), 7 is the
  magnification filter, 9 is fog, 10 and 11 are mirrored repeat in u and v.
- Lights are directional (w = 0) or positional (w = 1) with linear attenuation 1 / radius.

### Textures

- Uploaded padded to a power of two and always as RGBA8; the 16/32-bit creation flags are ignored.
  Filters are linear. Mip maps are built (`GL_LINEAR_MIPMAP_NEAREST`) only with flag `0x10`, which the
  game clears at start-up, so in practice there are none.

### Cg

On a change of material type the Cg material renderer calls the shader callback's
`OnSetConstants(services, 0)`, binds the programs with the latest profiles, applies the base material
and the basic render states; every draw calls `OnSetConstants(this, userData)` again. Programs compile
from source (`CG_SOURCE`), or from object code when the precompiled flag is set. See
[menu-scene.md](menu-scene.md) for the shaders.

## In the port

The port's renderer ([`port/src/video/CVideoGL.cpp`](../../port/src/video/CVideoGL.cpp), created by
`port::createVideoDriver`) is `CVideoOpenGL` function by function on OpenGL ES 3.0 / WebGL 2, or
OpenGL 3.3 core on the desktop (macOS has no ES). It derives from `CVideoNull`, keeps the 2D batch,
clipping, texel insets, half-pixel shifts, corner colour orders and per-batch filters above, and
issues the original's state changes in the original's order, so state leaks between draws the same
way.

- **Fixed function.** What core OpenGL lacks is kept by
  [`CFixedFunction`](../../port/src/video/CFixedFunction.h) as OpenGL 1.x would keep it and applied by
  one shader ([`shaders/fixed.vert`](../../port/src/video/shaders/fixed.vert), `fixed.frag`): the
  modelview and projection matrices, `GL_TEXTURE_2D` enables and texture environments per unit
  (`MODULATE`, `DECAL`, `REPLACE`, `ADD`, the light map's `COMBINE`), the alpha test (`GREATER`),
  per-vertex lighting (eight lights, no colour material, no `GL_NORMALIZE`, infinite viewer), sphere
  map texture generation and fog. Blending, depth, culling, front face, scissor, viewport, texture
  bindings and texture parameters stay in OpenGL, set where the original set them (including the
  active texture unit, so filter and wrap changes land on the same texture as in the original).
- **Draws.** Immediate mode and client arrays become one vertex array object with streamed
  buffers; vertices are uploaded in their `S3DVertex` layout and the shader reads the `SColor` bytes
  as BGRA. `GL_QUADS` become two triangles per quad, (0, 1, 2) and (0, 2, 3).
- **Leaked state is real.** `EMT_SOLID_2_LAYER` (the scattering ground materials' base) does not
  touch blending, so the planets draw with whatever blend the previous material left. In the menu
  the skybox's `EMT_SOLID` turns blending off first; without it the 2D GUI's blend would darken the
  low-level planets, whose alpha is below 1 on the night side.
- **Textures** have the image's size (no power-of-two padding; both APIs take any size), so
  `getSize` equals `getOriginalSize`; for the game's power-of-two images this changes nothing, and
  the original's read past a non-power-of-two A8R8G8B8 image cannot happen. Pixels stay A8R8G8B8
  for `lock` and upload as RGBA8. Mip maps only with `ETCF_CREATE_MIP_MAPS` (`glGenerateMipmap`,
  `GL_LINEAR_MIPMAP_NEAREST`). Uploads put back the texture binding they change.
- **Cg materials.** `addCgShaderMaterialFromFiles` still opens the files through the file system
  (a missing file fails with −1), but compiles the GLSL translation chosen by the file's name
  ([`shaders/scatter*.vert`/`.frag`](../../port/src/video/shaders/)). Uniforms keep Cg's separate
  vertex and pixel namespaces through the prefixes `vs_` and `ps_`; a name counts as found when the
  translation declares it, as Cg kept unreferenced parameters. Matrices upload column-major
  (`cgSetParameterValuefc`), samplers `DiffSpec`/`NormGlow` are units 0/1 (Cg's `TEX0`/`TEX1`), and
  the vertex shaders clamp `COLOR0`/`COLOR1` to [0, 1] as OpenGL's vertex colour clamping did. The
  translations compile as GLSL ES 3.00 (checked in WebGL 2) and GLSL 3.30. The material reports its
  base material's transparency. ARB assembly and GLSL 1.10 shader materials are not supported (the
  game uses neither).
- **Fixed original bugs:** the scissor's y is `height − LowerRightCorner.Y`; lights are limited to
  the eight the shader evaluates (`getMaximalDynamicLightAmount` returns 8, not the enum value
  `GL_MAX_LIGHTS`). Lines stay 1 px (OpenGL ES has no wide lines anyway).
- **Unchanged on purpose:** render targets do nothing and the minimap draws to the screen;
  `createScreenTexture` returns an empty texture; `setBasicRenderStates` reads colours, shininess,
  the filter and lighting from the driver's material; the vertex-shader-constant forwarding bug;
  the projection's element 12 (the x translation, 0 for the game's matrices) is negated; the Cg
  callback is never dropped.
- **Dropped:** polygon mode (wireframe; ES has none and the game never asks), stencil shadows (no
  stencil buffer; the game draws none), `queryFeature` reports what this renderer supports.
- **Screenshots.** `saveJpegScreenshot` picks the `Screen-yymmdd-NN.jpg` name at once and returns
  false like the original, but reads the frame at the next `endScene`, before presenting: the game
  calls it from an event handler, after the swap, when the back buffer is undefined on most
  platforms. JPEG quality 100 through stb_image_write, written through the file system.
- **Sizes.** `OnResize` and the creation size set the viewport, so the device should pass the
  drawable size in pixels (`SDL_GetWindowSizeInPixels`), which differs from the window size on
  high-density displays; the 2D mapping uses the render size and scales to the viewport.
