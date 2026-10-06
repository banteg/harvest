# The main menu's 3D scene

The main menu is the only part of the game that uses daisy's scene graph. Everything in play is
2D. The scene is three textured planets with atmospheric scattering, a skybox, a sun flare and a
camera that drifts around the planets and flies into the one the player picks. It is built by
`CMainMenuState::secondInit` steps 8 to 12
([`CMainMenuState.cpp`](../../src/HarvestFull/harvest/states/CMainMenuState.cpp), Linux `0x499c10`),
animated by `updateState` (`0x498fb0`), and torn down by the destructor, which calls
`ISceneManager::clear()` and removes the skybox and planet textures by name.

daisy's scene code is Irrlicht 0.7 with few changes, so `third_party/irrlicht-0.7/source/Irrlicht/` is the
reference for anything not listed here. The points below were checked against the Mac 1.18 build
(Binary Ninja HLIL, symbols) and, where an address is given as "Linux", against the Linux amd64
target.

## Coordinates and units

- World space is Irrlicht's left-handed system with +y up. The camera's view matrix comes from
  `buildCameraLookAtMatrixLH` (Mac `CCameraSceneNode::OnPreRender` `0xfc162`), and the projection
  is Irrlicht 0.7's left-handed perspective matrix (see [Camera](#camera)).
- From the skybox texture assignment (see [Skybox](#skybox)): +x is east, −x west, +z north,
  −z south, +y the roof. The sun is due east at (800, 0, 0). The planets sit around x = −200,
  and Atrum lies far to the west.
- One unit is one mesh unit. Planets have radius 10 and atmospheres radius 10.25, which are the
  `INNER_RADIUS` and `OUTER_RADIUS` the scattering shader assumes
  ([`CScatterShader.cpp`](../../src/HarvestFull/harvest/gfx/CScatterShader.cpp)). Every node has scale 1.
- Rotations are in degrees, as in Irrlicht.

## Scene contents

All nodes hang off the scene manager's root unless a parent is given.

| Node | Id | Created by | Position | Content | Material |
|---|---|---|---|---|---|
| Hephaestus | 1458 | `addAnimatedMeshSceneNode` | (−250, 0, 0) | `planetSphere_24.obj` | see [Shader levels](#shader-levels) |
| Poseidon | 1459 | same | (−200, 20, 80) | same | same |
| Ares | 1460 | same | (−150, −10, −30) | same | same |
| atmosphere *i* (high shaders only) | same id as its planet | child of planet *i* | (0, 0, 0) relative | `atmoSphere_48.obj` | Cg `scatterAtmoCG` over `EMT_TRANSPARENT_ADD_COLOR` |
| Atrum | 1461 | `addAnimatedMeshSceneNode` | (−900, −35, 25) | `planetSphere_24.obj`, texture `skyboxRoof.jpg` | `EMT_SOLID` |
| camera | −1 | `addCameraSceneNode` | animated | — | — |
| light | −1 | `addLightSceneNode` | (800, 0, 0) | — | — |
| skybox | −1 | `addSkyBoxSceneNode` | follows the camera | six `skybox*.jpg` | built in, see [Skybox](#skybox) |
| sun | −1 | `addBillboardSceneNode` | (800, 0, 0) | `particlewhite.jpg`, 500 × 500 | `EMT_TRANSPARENT_ADD_COLOR`, lighting off, z-buffer off |

The ids are the `CMainMenuState` enum: `ID_FIRST_PLANET` = 1458, `ID_ATRUM` = 1461. Atrum is the
easter-egg planet. Clicking it asks "Would you like to start the WICKED AWESOME GAME?", and *yes*
starts the shuttle race. If `skyboxRoof.jpg` cannot be loaded, Atrum gets a 64 × 64 black 16-bit
image registered as the texture `AtrumBlack` (colour format 1; see [textures.md](textures.md)).

### Shader levels

`settings:shaderlevel` in `harvest.cfg` (`CSystemConfig::getShaderLevel`) selects the planet
materials:

| Level | Planet textures | Planet material | Atmosphere |
|---|---|---|---|
| 0 (`ESL_NONE`) | `shaders/<P>NoShader.jpg` on layer 0 | `EMT_SOLID` | none |
| 1 (`ESL_LOW`) | `<P>DiffSpec.tga` layer 0, `<P>NormGlow.tga` layer 1 | Cg `scatterGroundSCG` over `EMT_SOLID_2_LAYER` | none |
| 2 (`ESL_HIGH`) | same | Cg `scatterGroundCG` over `EMT_SOLID_2_LAYER` | Cg `scatterAtmoCG` |

`<P>` is `Heph`, `Pos` or `Ares`. Step 8 first sets every planet to `EMT_SOLID`. Step 10 then
creates a `CScatterShader` per node and replaces the material type with the new Cg material id.
Only if that succeeds does `initAtmo` make the atmosphere node visible. If Cg fails at level 1 or 2,
the planets therefore stay `EMT_SOLID`, showing the DiffSpec texture, and the atmospheres stay
hidden.

### Rotation animators

Each planet gets `createRotationAnimator((0, −(i + 5) × 0.001, 0))` (i = 0, 1, 2). daisy's
`CSceneNodeAnimatorRotation::animateNode` (Linux `0x4cc1a0`, Mac `0x11727a`) is Irrlicht 0.7's: it
adds `Rotation × (now − last) / 10` to the node's rotation and stores `now`, with times in
milliseconds from `os::Timer`. The planets therefore spin about +y at −0.5, −0.6 and −0.7 degrees
per second. Atmospheres are children, so they turn with their planet. Animators run in
`OnPostRender` at the end of `drawAll`.

### Light

`setAmbientLight(32, 32, 32)` sets the GL light-model ambient, and `addLightSceneNode(0, (800, 0, 0),
white, 100)` adds the light, whose radius is then raised to 7500 and diffuse colour set to
117/255 grey. Both colours come from an `SColor` with alpha 0, so their alpha is 0. daisy's light
keeps the constructor colour as specular, which Irrlicht 0.7 does not (Mac `CLightSceneNode` ctor
`0xfe1ea`). The rest of the light is Irrlicht's `SLight` default: ambient (0, 0, 0, 1), shadows on,
and daisy's `Directional` flag off. Each frame the light's `render` sets its position to its
absolute position and calls `addDynamicLight`, after `drawAll` has called `deleteAllDynamicLights`. `CVideoOpenGL::addDynamicLight` (Mac `0x1571d0`) uses
constant attenuation 0, linear 1/radius and quadratic 0.

The light has no visible effect in this scene. The Cg materials do not read fixed-function
lighting, and daisy's `EMT_SOLID` renderer sets `GL_TEXTURE_ENV_MODE` to `GL_DECAL` (Linux
`0x4f2b70`). With opaque textures, `GL_DECAL` replaces the lit vertex colour with the texture
colour. Both the shader-less planets and Atrum are therefore drawn unlit, at full texture
brightness. This last part is inferred from the GL state: daisy's `setTexture` only switches the
active texture unit when the binding changes, so on some calls the `GL_DECAL` lands on unit 1
instead of unit 0.

## Meshes and the OBJ loader

`ISceneManager::getMesh` (Linux `0x4c6490`, Mac `0x111520`) lowercases the name, looks it up in the
mesh cache, and otherwise opens the file through the file system and asks each mesh loader whether
it handles the lowercased name's extension. `.obj` goes to `CStaticMeshOBJ::loadFile` (Linux
`0x57d1b0`, Mac `0x11a434`). The menu asks for the planet sphere by its `$GAME_RESOURCES$` name and
for the atmosphere by the alias-resolved absolute path. Both load, and they are cached under
different keys.

The loader is Irrlicht 0.7's, unchanged:

- **Lines.** It walks the file word by word. `v x y z`, `vt u v` and `vn x y z` fill three arrays.
  `f` lines are read to the end of the line. Lines starting with `#`, `u` (`usemtl`) or `g` are
  skipped. Anything else (`s 1`, `o`, `mtllib`) is ignored word by word. CRLF line ends are
  harmless. The file is read into a buffer of exactly its size with no terminating zero (Mac:
  `operator new[](size)`), so the walk runs on into whatever heap bytes follow until it meets a
  zero; with the shipped files that only adds ignored words. The port zero-terminates.
- **Numbers** go through Irrlicht's `fast_atof` (Mac uses its `fast_atof_table`): the integer
  part and the fraction digits are each read with `strtol` and the fraction is scaled by a table
  of powers of ten, all in float. The results can differ from `strtod` in the last bit, so a port
  that wants identical vertices uses the same routine (the port's loader does).
- **Faces.** Each corner is `v`, `v/vt` or `v/vt/vn`, with 1-based indices. Negative (relative)
  indices are not supported, and an index that is out of range or missing gives a zero position,
  UV or normal. More than 39 corners rejects the whole file. Every corner becomes its own vertex:
  nothing is shared or welded. A face with n corners is fanned as (0, 1, n−1), then
  (1, n−2−k, n−1−k) for k = 0 … n−4, so a quad becomes (0, 1, 3), (1, 2, 3).
- **Axes.** Positions are copied as they are, with no axis flip. The files were exported from
  Maya ("The units used in this file are centimeters"), which is right-handed. Read into
  Irrlicht's left-handed space without a flip, the meshes come out mirrored. For spheres this only
  mirrors the texture, and the game was authored against that result.
- **Texture coordinates.** U is copied, and V is negated (`TCoords.Y = −vt`; in Mac HLIL it is the
  XOR with the float sign mask). Textures wrap with `GL_REPEAT` (the material's mirror-wrap flags
  are off), so this samples t = 1 − v. With images stored top-down (both image loaders produce
  top-down rows), the top of the image ends up at OBJ v = 1, the north pole (+y).
- **Normals** are copied as they are. **Vertex colour** is opaque white. The mesh has one buffer,
  one default material and 16-bit indices.
- **Winding.** In the shipped files every face is counter-clockwise around its outward normal in
  right-handed terms. daisy draws 3D with `glFrontFace(GL_CW)` by default (see
  [Materials](#materials-and-render-states)) and a left-handed projection, which mirrors the image,
  so the outside of each sphere is front-facing. Back faces are culled, because
  `BackfaceCulling` is on by default.

| File | `v` | `vt` | `vn` | Faces | Radius | Vertices after loading | Indices |
|---|---|---|---|---|---|---|---|
| `planetSphere_24.obj` | 554 | 577 | 554 | 1104 triangles | 10.0 | 3312 | 3312 |
| `atmoSphere_48.obj` | 2258 | 2305 | 2258 | 2208 quads, 96 triangles | 10.25 | 9120 | 13536 |

Both are UV spheres with v = 0 at the south pole (y = −r) and v = 1 at the north pole. U goes
around the y axis, and the seam (u = 0 and 1) sits near +x. The planet has 24 segments, the
atmosphere 48. Normals are unit length and point outward.

## Materials and render states

These are daisy's OpenGL material renderers as the menu uses them. Mac addresses are in
`daisy/video/OpenGL/CVideoOpenGL.cpp`; Linux addresses are in the second column. Texture filtering
and the full render-state model belong to the `CVideoOpenGL` notes.

| Material | Linux | Blending | Depth | Textures |
|---|---|---|---|---|
| `EMT_SOLID` | `0x4f2b70` | `GL_BLEND` and `GL_ALPHA_TEST` off | per material: test and write on by default | layer 0 on unit 0, `GL_DECAL` (Irrlicht 0.7 uses `GL_MODULATE` here); units ≥ 1 off |
| `EMT_SOLID_2_LAYER` | `0x4f2c30` | not touched | per material | layer 0 on unit 0, layer 1 on unit 1, no texture environment set. In Irrlicht this is a blend between the layers; daisy reduces it to binding both, which is all the Cg ground shaders need |
| `EMT_TRANSPARENT_ADD_COLOR` | `0x4f2cc0` | `GL_BLEND` with `glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR)`, alpha test off, unit 1 off, `GL_MODULATE` | forces `ZWriteEnable = false`; the depth test follows the material | layer 0 if any |
| Cg (`COpenGLCGMaterialRenderer`) | `0x5a86a0` | the base material's | the base material's | the base material's |

`EMT_TRANSPARENT_ADD_COLOR` gives `dst = src + dst × (1 − src)`: additive, but saturating like a
"screen" blend. Its `isTransparent()` returns true, so these nodes go to the transparent list.

`CVideoOpenGL::setBasicRenderStates` (Linux `0x4ec4c0`, Mac `0x156c48`) maps the material flags as
follows. On Linux the flags start at offset `0x28` of `SMaterial`, after the two 8-byte texture
pointers.

| Flag | Linux offset | GL state |
|---|---|---|
| 0 | `0x28` | `glPolygonMode` fill or line (wireframe) |
| 2 | `0x2a` | `GL_LIGHTING` |
| 3 | `0x2b` | `GL_DEPTH_TEST` |
| 4 | `0x2c` | `glDepthMask` |
| 5 | `0x2d` | `GL_CULL_FACE` |
| 6 | `0x2e` | `glFrontFace(GL_CW)` when 0, `GL_CCW` when 1 |
| 9 | `0x31` | `GL_FOG` |
| 10, 11 | `0x32`, `0x33` | wrap S and T: `GL_REPEAT` when 0, `GL_MIRRORED_REPEAT` when 1 |

Flag 6 matters for the atmosphere. The recovered [`SMaterial.h`](../../src/ox/video/SMaterial.h)
calls it `BilinearFilter`, after Irrlicht's field order, and `CScatterShader::initAtmo` sets it
under that name (Linux `0x4648b4`: `mov byte [rax+0x2e], 1`). In daisy the flag selects the front
face instead. With culling on, the atmosphere therefore draws its far hemisphere, the inside of the
shell seen through the planet's limb. This is the usual "sky from space" set-up for O'Neil's
shader, and a port must reproduce it: cull front faces for the atmosphere, not back faces.
`setRenderStates3DMode` (Mac `0x154496`) applies the same flag when render states are reset.

### Draw order

`CSceneManager::drawAll` (Linux `0x4c3000`, Mac `0x1126e0`) is Irrlicht 0.7's, with `std::vector`
render lists sorted by `std::sort`:

1. It stores the active camera's absolute position, then calls `OnPreRender` on the root. Nodes
   register in tree order (children right after their parent): camera and light at time 0
   (`SNRT_LIGHT_AND_CAMERA`), skybox at 1 (`SNRT_SKY_BOX`), the rest at 2 (`SNRT_DEFAULT`); 3 is
   `SNRT_SHADOW`. The camera uploads the projection here (see [Camera](#camera)).
2. `deleteAllDynamicLights`, then the camera's `render` sets the view matrix and the light's
   `render` adds the light.
3. The skybox is drawn.
4. Opaque nodes are sorted by the first texture of their first material (the pointer value,
   unsigned) and drawn. The order between planets is therefore arbitrary, which only matters
   for equal depths.
5. Shadow volumes would be drawn here; there are none.
6. Transparent nodes are drawn: any node with a material whose renderer reports
   `isTransparent()`. They are sorted by the distance from the stored camera position to the
   translation of the node's absolute transformation, computed in double and stored as float,
   **nearest first** (daisy's `operator<` compares the distances, Mac `__insertion_sort` at
   `0x114224`).
7. `OnPostRender(os::Timer::getTime())` runs the animators, then the deletion queue is emptied.

Registration at time 2 also drops nodes whose transformed bounding box misses the view frustum's
axis-aligned bounding box (`isCulled`, Mac `0x1122ee`; only nodes with automatic culling on, so
never the sun). Planets are opaque. Atmospheres (base material `EMT_TRANSPARENT_ADD_COLOR`) and
the sun are transparent. `CMainMenuState::render` clears to black, calls `drawAll`, then draws the
logo sprite (neutral mode only), the GUI and the fade rectangle.

**Transform updates.** daisy moved the absolute-transformation update: `ISceneNode::OnPreRender`
(Mac `0x1136dc`) updates a visible node's absolute transformation before recursing into its
children, the camera's `OnPreRender` updates its own first, and `CAnimatedMeshSceneNode::OnPostRender`
(Mac `0xf59ea`) runs the animators and the children without updating it (Irrlicht 0.7 updates in
`OnPostRender` only). A node registers before its own update, so culling and the transparent
distance use the node's transformation from the previous frame (rotation included), the camera
position from the previous frame and, for nodes ahead of the camera in the tree (all the menu's
planets and atmospheres), the frustum from the previous frame; what is drawn uses this frame's. In the first `drawAll` after `secondInit` the planets still have the transformation
of their constructor (at the origin, since `setPosition` came after) and the camera's frustum is
the default one, whose box is (−1, −1, −1)–(1, 1, 1). The planets' boxes at the origin intersect
it, so nothing is culled in that frame, and all transparent distances but the sun's are 0.

Per frame, the driver therefore sees: `setTransform(PROJECTION)`, `deleteAllDynamicLights`,
`setTransform(VIEW)`, `addDynamicLight`; the skybox's `setTransform(WORLD)` and six
`setMaterial` + `drawIndexedTriangleList` (4 vertices, 2 triangles); per opaque node
`setTransform(WORLD, absolute)`, `setMaterial` and `drawMeshBuffer`; the same for the
atmospheres; and the sun's `setTransform(WORLD, identity)`, `setMaterial` and
`drawIndexedTriangleList` with world-space vertices. `port/tests/menu_scene.cpp` prints this
sequence with all matrices, materials and shader constants.

## Skybox

`addSkyBoxSceneNode(top, bottom, left, right, front, back)` gets the textures
`(Roof, Floor, East, West, South, North)`. The node is Irrlicht 0.7's (`CSkyBoxSceneNode`, Mac ctor
`0x11930a`, render `0x11a0b4`) apart from the texture coordinates:

- It is a cube of half-size 10 with six quads, indices (0, 1, 2), (0, 2, 3), drawn every frame with
  the world matrix set to a translation to the camera's absolute position. It is drawn before
  everything else, with no depth test or depth write, so it always stays behind.
- Material: lighting off, z-buffer off, z-write off, back-face culling on, front face `GL_CW`, and
  the remaining flags at daisy's `SMaterial` defaults.
- The bounding box is (−1, −1, −1)–(1, 1, 1), not empty.
- Texture coordinates use `o = 1 / (1.5 × W)` and `t = 1 − o`, where W is `getSize().Width` (the
  texture's size, not the image's) of the first non-null texture of front, left, back, right,
  top and bottom; without any texture o = 0. For the shipped 1024² JPEGs, o = 1/1536. The
  half-pixel inset hides seams with clamping off.
- Normals point into the cube: front (0, 0, 1), left (−1, 0, 0), back (0, 0, −1), right
  (1, 0, 0), top (0, −1, 0), bottom (0, 1, 0). Vertex colours are white. The six materials are
  `getMaterial(0..5)` in the order front, left, back, right, top, bottom, one draw each.

| Face | Texture | Corners (x, y, z) | daisy UVs | Irrlicht 0.7 UVs |
|---|---|---|---|---|
| front, z = −10 | South | (−,−,−) (+,−,−) (+,+,−) (−,+,−) | (t,t) (o,t) (o,o) (t,o) | (o,t) (t,t) (t,o) (o,o) |
| left, x = +10 | East | (+,−,−) (+,−,+) (+,+,+) (+,+,−) | (t,t) (o,t) (o,o) (t,o) | (o,t) (t,t) (t,o) (o,o) |
| back, z = +10 | North | (+,−,+) (−,−,+) (−,+,+) (+,+,+) | (t,t) (o,t) (o,o) (t,o) | (o,t) (t,t) (t,o) (o,o) |
| right, x = −10 | West | (−,−,+) (−,−,−) (−,+,−) (−,+,+) | (t,t) (o,t) (o,o) (t,o) | (o,t) (t,t) (t,o) (o,o) |
| top, y = +10 | Roof | (+,+,+) (−,+,+) (−,+,−) (+,+,−) | (t,t) (o,t) (o,o) (t,o) | (o,o) (o,t) (t,t) (t,o) |
| bottom, y = −10 | Floor | (−,−,+) (+,−,+) (+,−,−) (−,−,−) | (o,o) (t,o) (t,t) (o,t) | (o,o) (o,t) (t,t) (t,o) |

daisy mirrors U on the four side faces. Its top face is Irrlicht's reflected across the
anti-diagonal, (u, v) → (1 − v, 1 − u), and its bottom face is Irrlicht's transposed,
(u, v) → (v, u). These changes undo the mirroring that the left-handed world puts on images
authored for a right-handed viewer. To port the skybox, take the table's daisy column as the
specification.

## Sun

The sun is a `CBillboardSceneNode` (Linux render `0x54a770`, Mac ctor `0xf65ba`) with Irrlicht
0.7's geometry. It is a camera-facing quad of the node's size, built from the camera's target and
up vector, with UVs (0,0) (0,1) (1,1) (1,0) and indices (0, 2, 1), (0, 3, 2). The camera
must therefore not be looking straight along its up vector. Automatic culling is off, so
`getAutomaticCulling()` is always false and the occlusion code below always runs.

Because the sun has no depth test, it would draw over the planets. `updateState` hides it by hand
every frame. For each planet it takes the squared distance d from the planet's centre to the
closest point on the segment from the camera to the sun:

- d < 100, which means the line passes within the planet's radius 10: the sun is hidden.
- 100 ≤ d < 121: the sun is scaled by `1 − (11 − √d)`, shrinking from full size at distance 11 to
  0 at distance 10.
- Otherwise it is shown at 500 × 500. `SUN_INNER_FLARE_SIZE` (20 × 20) is never used.

## Camera

`addCameraSceneNode()` creates the camera with position (0, 0, 0), target (0, 0, 100) and id −1.
The constructor (Linux `0x554980`, Mac `0xfb9ca`) is Irrlicht 0.7's:

- FOV π/2.5 (1.2566371 rad, 72°), near 1, far 3000, up (0, 1, 0).
- Aspect: the constructor first sets 4/3, then replaces it with **height / width** of
  `IVideoDriver::getScreenSize()` at creation. Irrlicht 0.7 has this inverted ratio too. The
  aspect is not updated on resize.
- `recalculateProjectionMatrix` (Linux `0x5519a0`, called by `setFOV`, `setAspectRatio`,
  `setNearValue` and `setFarValue`) builds Irrlicht 0.7's `buildProjectionMatrixPerspectiveFovLH`.
  With h = cot(fov/2) and w = h / aspect, as indices into the 16 floats `M[]` of `CMatrix4`:
  `M[0] = 2n/w`, `M[5] = 2n/h`, `M[10] = f/(f−n)`, `M[11] = 1`, `M[14] = n·f/(n−f)`, and 0
  elsewhere (`M[15]` too). In Irrlicht's `operator()(row, col)`, which reads `M[col·4 + row]`,
  the last two are `(3,2)` and `(2,3)`. At 1024 × 768 the default camera's matrix is
  `M[0] = 1.0898137`, `M[5] = 1.4530849`, `M[10] = 1.0003334`, `M[14] = −1.0003334`.
  This is not a standard perspective matrix. The y scale is 2n·tan(fov/2), where a standard matrix
  has cot(fov/2), and the x scale is the y scale times height/width. With n = 1 the effective
  vertical field of view is `2·atan(1 / (2·tan(fov/2)))`:

  | `setFOV` value | Effective vertical FOV |
  |---|---|
  | 72° (default) | 69.1° |
  | 90° | 53.1° |
  | 120° (end of the start-game zoom) | 32.2° |

  A larger FOV value therefore zooms in. A port should either reproduce this matrix or convert
  each FOV value with the formula above.
- Each frame `OnPreRender` (Mac `0xfc162`) first updates the camera's absolute transformation
  (daisy; Irrlicht 0.7 does not), so the position the game set during `updateState` is used in
  the same frame. If the camera is the active one, it builds the view with
  `buildCameraLookAtMatrixLH(absolutePosition, target, up)`, adding 1 to `up.X` of the normalised
  up vector if the view direction is within 1e-4 of parallel to it, rebuilds the view frustum
  from projection × view, then uploads the projection (Irrlicht 0.7 uploads it before building
  the view) and registers for rendering. `render` sets the view transform.
- The view frustum is Irrlicht 0.7's `SViewFrustrum`: six planes taken from projection × view,
  the camera position, and the axis-aligned box around the camera position and the four far
  corners. Culling and picking only use that box and the far corners.

### Animation

`CMainMenuState` drives the camera directly with two pairs of point and velocity:
`CameraPosition`/`CameraSpeed` towards `CameraPositionTarget`, and the look-at point
`CameraTarget`/`CameraTargetSpeed` towards `CameraTargetTarget`. Both move with
`C3dRegulator::fakeSpeedRegulation(speed, pos, target, _, factor, dt)`
([`CRegulator.cpp`](../../src/ox/algo/CRegulator.cpp)): the desired velocity is
`(target − pos) × factor`. Speed accelerates towards it at 20 units/s², each component is capped at
the desired component once it passes it in the same direction, and `pos += speed × dt`. The frame
time is clamped to [0, 0.06] s.

The initial state (step 9) is position (−200, −20, −150), look-at (−350, 0, −70), look-at velocity
(−20, 0, 0), position target (−200, 40, −100) and look-at target (−200, 0, 0).

| Mode | Position target | Look-at target | Factors |
|---|---|---|---|
| neutral | `C + (100·cos(a + π/2), −10·cos(3.1·a), −90·sin(a + π/2))`, with `a = time · π · 0.005` (a full circle in 400 s) | C = (−200, 5, 25) | look-at 0.5, position 0.5 |
| planet select (*New game*) | (−200, 40, −100) | (−200, 0, 0) | same |
| after picking a planet | planet + (13, 0, −10) | planet + (13, 0, 0) | same |
| start game | unchanged | the planet's position | look-at 0.8, and only while farther than 3 units; position 0.5 |

`time` accumulates the clamped frame times. The regulator's fourth argument (20 for the position,
30 for the look-at point) is unused.

**Fly-over.** Before the position step, `updateState` looks at the segment from the current
position to the position target. For each planet whose centre is within 11 units (squared distance
< 121) of the closest point on that segment, it moves the *target's* y to
`CameraPosition.Y − 10·√d` if the planet is above the target, or `+ 10·√d` otherwise, so the
camera arcs over or under the planet.

**Start-game zoom.** In start-game mode the FOV goes from π/2.5 to π/1.5 over 3 s
(`fov = π / (2.5 − t/3)`). Given the projection above, this narrows the view onto the planet. A
black rectangle fades in with alpha `0.25 × t × 255` over 4 s, after which the state switches to
play.

**Ambience.** Each frame, for each planet, d is the distance from the camera to the planet. If
d < 100, `ambience<i+1>.ogg` plays looped at volume `1 − d/100` (`updateMusic`, else `playMusic`).
Otherwise it is stopped.

## Picking

- **Hover.** On mouse move, when planet selection is allowed,
  `getSceneNodeFromScreenCoordinatesBB(mouse)` (Linux `0x575370`, Mac `0x10d9d6`) is Irrlicht 0.7's.
  It builds a ray from the camera position to the point on the far plane under the mouse,
  interpolating the frustum's far-left-up, far-right-up and far-left-down corners by
  `x / screenWidth` and `y / screenHeight`. It then walks the whole tree, including invisible
  nodes' children, and tests each visible node's world-space bounding box: the local box
  transformed by the absolute transformation, then re-boxed. The id mask is 0, so every node is a
  candidate. Of the boxes the ray hits, it returns the node whose **absolute position** is closest
  to the ray start, not the closest hit.
  - Id 1458 to 1460 selects that planet and shows the info popup above it.
  - Id 1461 (Atrum) marks it hovered.
  - No hit clears the hover.
  - Any other node leaves the hover state unchanged. This includes the sun, whose billboard and
    light both have unit boxes at (800, 0, 0).

  An atmosphere has the same id as its planet. As the planet's child it is tested after it at the
  same distance, so it never replaces it (the comparison is strict).
- **Popup position.** `getScreenCoordinatesFrom3DPosition(planet)` (Linux `0x574800`, Mac
  `0x10feb0`) is Irrlicht 0.7's. It multiplies (x, y, z, 1) by projection × view, returns
  (−10000, −10000) if w < 0, and otherwise returns
  `(W/2 · x/w + W/2, H/2 − H/2 · y/w)` with W and H from `getScreenSize()`. Without a camera it
  returns (−1000, −1000).
- **Click.** A left click on a hovered planet checks the profile's achievement score. Poseidon
  needs 46 and Ares 84; below that the click plays `BtnDenial.ogg` and shows the locked-planet box.
  Otherwise the camera targets the planet and the game-mode window opens.

## The Cg scattering shaders

The six files in `gfx/shaders/` are Sean O'Neil's atmospheric scattering from *GPU Gems 2*,
chapter 16, in Cg. Each `.vsh` and `.psh` pair is one material:

| Material | Vertex | Pixel | Used for |
|---|---|---|---|
| `scatterAtmoCG` | sky from space: 2-sample in-scattering along the ray from the camera to the vertex | Rayleigh and Mie phase functions | atmospheres, high level |
| `scatterGroundCG` | ground from space: 2-sample scattering and ground attenuation | diffuse, specular, glow, scattering | planets, high level |
| `scatterGroundSCG` | transform only | the same lighting without scattering (`+ 0.1` ambient) | planets, low level |

### Inputs

| Shader | Vertex attributes | Varyings (vertex → pixel) |
|---|---|---|
| atmosphere | `POSITION` | `COLOR0` Rayleigh colour (rgb), `COLOR1` Mie colour (rgb), `TEXCOORD0` camera − vertex |
| ground | `POSITION`, `TEXCOORD0`, `NORMAL` | `COLOR0` in-scattered light, `COLOR1` attenuation (rgb, a = 1), `TEXCOORD0` UV, `TEXCOORD1` normal |
| ground, simple | `POSITION`, `TEXCOORD0`, `NORMAL` | `TEXCOORD0` UV, `TEXCOORD1` normal |

The vertex data is daisy's `S3DVertex`: position, normal, colour and one UV set, from the OBJ
loader above.

### Uniforms and how `CScatterShader::OnSetConstants` fills them

`OnSetConstants` (Linux `0x464a20`) runs from the material renderer's `OnRender` before every
draw of the node, and once more from `OnSetMaterial`. P is the node's absolute position, which for
an atmosphere is its planet's, and C is the camera's absolute position.

| Uniform | Stage | Value | Set when |
|---|---|---|---|
| `v3CameraPos` | vertex | C − P | scattering |
| `v3LightPos` | vertex | normalize(−P): the direction from the planet to the world origin | scattering |
| `v3InvWavelength` | vertex | (5.602046, 9.473284, 19.643805) = 1/λ⁴ for λ = 0.650, 0.570, 0.475 µm | scattering |
| `fCameraHeight2` | vertex | \|C − P\|² | scattering |
| `fInnerRadius`, `fOuterRadius`, `fOuterRadius2` | vertex | 10, 10.25, 105.0625 | scattering |
| `fScale`, `fScaleOverScaleDepth` | vertex | 1/0.25 = 4, 4/0.25 = 16 | scattering |
| `matRot` | vertex | `T(−P) · World`: the node's world matrix without its translation | always |
| `matViewProjection` | vertex | `Projection · View · World` from the driver's current transforms | always |
| `matWorldInverseTranspose` | vertex | `transpose(inverse(matRot))` | ground |
| `v3CameraPos` | pixel | C − P | ground |
| `v3LightPos` | pixel | normalize(−P) | always |

"Scattering" is false only for the low-level ground material (`initGround(false)`). The
atmosphere always scatters. `fCameraHeight` is computed but not uploaded.

The light direction does not point at the sun billboard (800, 0, 0) but at the origin. With the
planets near x = −200 the two are a few degrees apart. The scattering constants that are not
uniforms are fixed in the shaders: Kr·ESun = 0.0375, Km·ESun = 0.0225, Kr·4π = 0.0314159,
Km·4π = 0.0188496, scale depth 0.25, Mie g = −0.95, and two samples.

### Matrix convention

Matrices are `ox::core::CMatrix4`, which is Irrlicht's `matrix4`: 16 floats with the translation in
`M[12..14]`. `operator*` multiplies them as column-major matrices
([`CMatrix4.h`](../../src/ox/core/CMatrix4.h)). `CCGMaterialRenderer::setVariable` (Linux
`0x5d7df0`, Mac `0x13977a`) looks the name up with `cgGetNamedParameter` on the vertex or pixel
program and uploads with `cgSetParameterValuefc` (the **c**olumn-major variant) and the given
count. The shaders' `mul(M, v)` is therefore the usual column-vector product. In GLSL, upload the
same 16 floats with `glUniformMatrix4fv(loc, 1, GL_FALSE, M)` and write `M * v`. For a name that is
not found, the renderer logs "CG variable not found" and the program's parameter list, once per
material.

`matRot` holds only rotation (the nodes have unit scale), so `matWorldInverseTranspose` equals
`matRot` here. Normals are transformed by it as 4-vectors, with the attribute's w (1 in the GL
profiles), and only `.xyz` is used.

### Texture units

`EMT_SOLID_2_LAYER` binds material layer 0 to unit 0 and layer 1 to unit 1, and the pixel shaders
declare `sampler2D DiffSpec : TEX0` and `NormGlow : TEX1`:

- `<P>DiffSpec.tga` on unit 0: rgb is the diffuse colour and alpha the specular strength. Alpha
  ranges 0–119 (Ares), 44–167 (Hephaestus) and 14–217 (Poseidon).
- `<P>NormGlow.tga` on unit 1: alpha is the glow (city lights) mask. The rgb channels hold a
  tangent-space normal map for Poseidon and Ares, and a copy of the diffuse rgb for Hephaestus,
  but **neither ground shader reads them**. Lighting uses the interpolated vertex normal.

All six maps are 1024 × 512, 32-bit, uncompressed TGA with a bottom-left origin.

### Profiles and binding

`addCgShaderMaterialFromFiles` (Linux `0x4e18f0`) opens both files through the file system, so the
`$GAME_RESOURCES$` alias works, and calls `CVideoOpenGL::addCgShaderMaterial`. That creates a
`COpenGLCGMaterialRenderer` around the base material's renderer and registers it as a new material
id, which is returned, or −1 on failure.

- **Compile.** `createCGVertexShader` and `createCGPixelShader` (Linux `0x5a8550`, `0x5a8490`) each
  call `cgGLGetLatestProfile(CG_GL_VERTEX` or `CG_GL_FRAGMENT)`, then `cgGLSetOptimalOptions`, then
  `cgCreateProgram(context, CG_SOURCE, text, profile, "main", NULL)`, then `cgGLLoadProgram`. The
  profile is whatever the driver reports as best at run time, so the original ran these through
  several different back ends.
- **Set material** (Linux `0x5a86a0`), when the material type changes or states are reset:
  1. Call the callback's `OnSetConstants(services, 0)`.
  2. `cgGLBindProgram` and `cgGLEnableProfile` for the vertex program, then the pixel program.
  3. Call the base renderer's `OnSetMaterial(material, material, true, services)`.

  It then sets basic render states. `isTransparent` is the base renderer's.
- **Unset.** `cgGLUnbindProgram` and `cgGLDisableProfile` for both, then the base renderer's
  `OnUnsetMaterial`.
- **Render.** `OnRender` calls `OnSetConstants(services, userData)` before each draw.

### Notes for GLSL

- Attributes: position as `vec4(pos, 1)`, normal as `vec4(n, 1)`, and UV as stored (the V is
  already negated).
- `float3`/`float4` become `vec3`/`vec4`, `mul(M, v)` becomes `M * v`, `saturate(x)` becomes
  `clamp(x, 0.0, 1.0)`, `tex2D` becomes `texture`, and `lerp` (unused) would be `mix`. `pow` with a
  negative base is undefined in both languages. `pow(1 − fTotalDiffuseScale, 5)` in the ground
  shader stays non-negative because `fTotalDiffuseScale ≤ 1`.
- The vertex shaders write `COLOR0` and `COLOR1`. Under the GL profiles these are the
  primary and secondary colour, which OpenGL clamps to [0, 1] by default (vertex colour clamping).
  The atmosphere shader's own `min(…, 100)` suggests the authors did not rely on that, but what
  the original showed is the clamped value. If you use plain varyings, clamp `FrontColor` and
  `SecondaryColor` to [0, 1] to match. This is inferred from GL semantics and was not checked on
  hardware.
- The ground pixel shaders use `normalize(v3CameraPos)`, the direction from the planet centre to
  the camera, as a constant view vector for specular, and the unnormalized interpolated normal.
  Keep both to match the look.
- Blending and depth come from the base materials. For the atmosphere: blend
  `ONE, ONE_MINUS_SRC_COLOR`, depth test on, depth write off, cull front faces (flag 6), drawn after
  opaque geometry. For the ground: opaque, depth test and write on, cull back faces.

## The port's scene manager

`port/src/scene/` implements `daisy::scene::createSceneManager` with the subset above, adapted from
the Irrlicht 0.7 sources with daisy's changes: the scene manager and root node, `ISceneNode`'s
bodies (declared under `HARVEST_PORT` in [`ISceneNode.h`](../../src/ox/scene/ISceneNode.h)), the
animated mesh, camera, light, billboard and skybox nodes, the rotation animator, bounding-box
picking and projection, and the OBJ loader. Every other `ISceneManager` function returns null or
does nothing. Rendering goes through `IVideoDriver` only.

`port/tests/menu_scene.cpp` builds the scene as `secondInit` does on a recording null driver and
prints the meshes, every driver call of three `drawAll` frames (with the constants
`CScatterShader` sets for each draw), the picking and projection results and the teardown:

    cd port && zig build test-menu_scene -- <directory with harvestClientData> [shader level] [width height]

The planets' rotation animators run on the wall clock, so the test sets the planet rotations
before each frame. The opaque draw order follows texture addresses and can differ between runs
on other platforms.
