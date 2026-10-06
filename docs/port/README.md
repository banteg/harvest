# Notes for a modern port

The recovered source is organised by the port-relevance layers in
[`config/1.18-linux-amd64/layers.toml`](../../config/1.18-linux-amd64/layers.toml): game logic and the
engine core are kept as source, the GUI toolkit is kept for its behaviour, and the platform layer is
what a port replaces. This section covers what a port still has to know about the platform layer, and
the data formats the game ships.

The plan for the port itself (targets, libraries, milestones) is in
[porting-plan.md](porting-plan.md). The port's build lives in [`port/`](../../port/README.md), and
[census.md](census.md) lists which units it compiles and every symbol and interface the platform
layer still has to provide. Release packages are described in [packaging.md](packaging.md).

## Data the game ships

| Data | Format | Where it is described |
|---|---|---|
| `gfx/*.dat`, mod `.dat` | Oxeye sprite package | [`CSpritePackage.h`](../../src/daisy/video/Null/CSpritePackage.h) |
| `gfx/particles.pfx` | Oxeye particle package | [`CParticlePackage.cpp`](../../src/daisy/video/Null/CParticlePackage.cpp) |
| `gfx/*.fnt` | sprite-package font, loaded as `CUnicodeFont` | [`CUnicodeFont.h`](../../src/daisy/gui/CUnicodeFont.h) |
| `lang/*.cfg`, `*.hmd`, profiles, settings | configuration text format | [`CConfiguration.h`](../../src/ox/game/CConfiguration.h) |
| save games (`*.hsg`), profiles | a plain header, then the `CPlayState` payload as a zlib stream encrypted with AES-256 under a fixed key; profiles are configuration text | [save-games.md](save-games.md) |
| `mods/*.zip` | zip archives holding a `.hmd`, `main.lua`, an icon and data | [mods-and-files.md](mods-and-files.md), [zip-archives.md](zip-archives.md) |
| `sfx/*.ogg` | Ogg Vorbis, decoded with libvorbisfile | [audio.md](audio.md) |
| `gfx/*.jpg`, `*.tga` | JPEG (libjpeg) and Irrlicht's TGA loader | [textures.md](textures.md) |
| `gfx/*.obj` | the two planet spheres, Irrlicht's OBJ loader | [menu-scene.md](menu-scene.md) |
| `gfx/shaders/*.vsh`, `*.psh` | Cg atmospheric scattering (Sean O'Neil, GPU Gems 2) | [menu-scene.md](menu-scene.md) |

Paths, aliases and file lists: [file-system.md](file-system.md). Driver caches, screen size and 2D
coordinates: [renderer.md](renderer.md). Keys, mouse, joysticks, window modes and timing:
[input-and-window.md](input-and-window.md).

## The platform layer

About 793 KB of the executable is the daisy platform layer (Irrlicht 0.7 derived). The game uses a
small part of it:

- **Recovered as source**, because it defines behaviour a port must reproduce: the 2D renderer and its
  texture and package caches (`CVideoNull`, `CVideoOpenGL`), the file system (aliases, paths into zip
  archives, file lists), the device glue (SFML events to ox events, timer, window modes, clipboard)
  and the OpenAL audio backend.
- **Documented here** rather than matched: the main menu's 3D scene and its Cg shaders, texture
  loading conventions, and the highscore HTTP protocol.
- **Never reached** by the game, engine or toolkit, and safe to drop (about 420 KB): the X, MS3D, MD2,
  3DS and Quake 3 loaders, terrain, octree, shadow volumes, the particle-system scene node, water,
  text and test nodes, FPS and Maya cameras, most animators and triangle selectors, the software
  renderer and its rasterizers, the XML reader and writer, the PSD, PCX and BMP loaders, stencil
  shadows, fog and post-processing shaders.

The executable links these libraries dynamically, so their work is not in the binary: zlib, libjpeg,
libogg, libvorbis and libvorbisfile, OpenAL and ALUT, Cg and CgGL, OpenGL and GLU, SFML 2 (window,
input, joysticks), GTK 2 (clipboard only), X11 and LuaJIT.

## Original bugs

[original-bugs.md](original-bugs.md) lists behaviours of the original executables that read as bugs,
what they affect, and what a port should do about each.
