# Texture loading

This page covers how image files become textures: names, the texture cache, loader selection, the
JPEG and TGA loaders, and colour formats. Turning an image into a GL texture (format, power-of-two
handling, mipmaps), filtering and blending are covered in the `CVideoNull`/`CVideoOpenGL` notes.

daisy's texture code is Irrlicht 0.7's `CVideoNull` with a sorted cache. Addresses are from the
Linux target and the Mac 1.18 build (symbols); the Irrlicht 0.7 sources are in
`third_party/irrlicht-0.7/source/Irrlicht/`.

## Names and the cache

| Call | Linux | What it does |
|---|---|---|
| `getTexture(const char* name)` | `0x4e4a00` | looks `name` up in the cache; otherwise opens it with `IFileSystem::createAndOpenFile(name)`, loads it, and caches it under `name` |
| `getTexture(IReadFile*)` | `0x4e4940` | the same, keyed by `file->getFileName()` |
| `addTexture(name, IImage*)` | `0x4e48c0` | makes a texture from an image already in memory and caches it under `name` |
| `addTexture(size, name, format)` | `0x4e47e0` | the same for a new blank image |
| `removeTexture(name)` | `0x4e3870` | drops the cache's reference to the first entry whose name matches, ignoring case |
| `removeTexture(ITexture*)` | `0x4e37d0` | the same, by pointer |

The cache (Mac `findTexture` `0x14a634`, `addTexture` `0x14a784`) is a vector of
`{name, ITexture*}` kept sorted by name and searched with a binary search. Rules a port should
keep:

- **The key is the name as passed, lowercased** (`CStringFunctions::ansiMakeLower`). Nothing else
  is normalized. The `$GAME_RESOURCES$` alias stays in the key, and slashes are not normalized.
  `$GAME_RESOURCES$/harvestClientData/gfx/a.jpg` and the same file by its absolute path are two
  entries, and two loads.
- Aliases are resolved only when the file is opened, by the file system
  (see [mods-and-files.md](mods-and-files.md#file-system)). A name may also point into a zip
  archive, as mod icons do: `<mod path>/favicon.tga`.
- The cache holds one reference. `getTexture` returns a borrowed pointer, and textures live until
  `removeTexture` or `removeAllTextures`. The main menu removes its skybox and planet textures by
  name when it is destroyed. Everything else stays loaded.
- Textures that are not files share the same name space. `AtrumBlack` is the main menu's fallback
  image. Sprite packages register their texture planes as `"<index>#<package file name>"`
  ([`CSpritePackage.cpp`](../../src/daisy/video/Null/CSpritePackage.cpp)), so `0#…/ingame.dat` and
  so on.
- Failures are logged ("Could not open file of texture", "Could not load texture") and return null.

## Choosing a loader

`createImageFromFile(IReadFile*)` (Linux `0x4dfb90`, Mac `0x14bb74`) is Irrlicht 0.7's two-pass
search over the loaders. The `CVideoNull` constructor (Linux `0x4e21d0`) creates them in the order
BMP, JPG, TGA, PSD, PCX.

1. **By name.** Each loader's `isALoadableFileExtension(fileName)` is a case-sensitive `strstr` for
   `.bmp`, `.jpg`, `.tga`, `.psd` or `.pcx` anywhere in the name. `.JPG`, `.jpeg` and `.TGA` do not
   match.
2. **By content**, if no loader in pass 1 produced an image. The stream is rewound before each try.
   JPG accepts the four bytes `JFIF` at offset 6, so Exif-only JPEGs are refused. TGA accepts byte 2
   (the image type) equal to 2, so compressed TGAs are refused here, though pass 1 loads them.

All game textures have lowercase `.jpg` or `.tga` names, so pass 1 always decides. It has to:
`particlewhite.jpg` is an Exif JPEG (`Exif` at offset 6), so the content test would refuse it.

## JPEG

`CImageLoaderJPG::loadImage` (Linux `0x59aae0`, Mac `0x13ca24`) is Irrlicht 0.7's, linked against
the system libjpeg (`jpeg_CreateDecompress` with version 62, i.e. libjpeg 6b):

- The whole file is read into memory and fed through a memory source manager. Default output
  parameters are used, so a colour JPEG decodes to RGB.
- The result is a `CImage` of format 2 (R8G8B8): rows top-down, bytes in R, G, B order.
- Grayscale JPEGs are not handled. The row buffer is `width × num_components` bytes, but the image
  is declared R8G8B8, so a one-component file would come out garbled (from the code; the game
  ships none). Every shipped JPEG is a 3-component baseline (SOF0) image.
- The loader can also save. `saveImage` (Mac `0x13cc8e`) writes RGB at quality 100. Ctrl+T saves a
  screenshot through it into `$HARVEST_USERDATA$/screenshots/`
  ([`CHarvestFullMain.cpp`](../../src/HarvestFull/harvest/CHarvestFullMain.cpp)).

## TGA

`CImageLoaderTGA::loadImage` (Linux `0x59c010`, Mac `0x13e156`) is Irrlicht 0.7's:

- **Header.** The standard 18-byte header is read field by field, little-endian. The image id
  (`IdLength` bytes) is skipped. If `ColorMapType` ≠ 0, `ColorMapLength × ColorMapEntrySize/8`
  bytes of colour map are skipped. The TGA 2.0 footer (`TRUEVISION-XFILE`) and extension area are
  ignored.
- **Image types.** Type 2 (uncompressed true-colour) is read directly. Type 10 (RLE true-colour)
  goes through `loadCompressedImage`. Anything else logs "Unsupported TGA file type" and fails, so
  colour-mapped (1, 9) and grayscale (3, 11) files are not supported.
- **RLE.** A packet header byte h with the high bit set means one pixel repeated `(h & 0x7f) + 1`
  times. Otherwise `h + 1` raw pixels follow. Packets may cross scanlines. There is no bounds check,
  so a bad file can overrun the buffer.
- **Pixel depth.** `PixelDepth / 8` bytes per pixel:

  | Depth | Image format | Conversion |
  |---|---|---|
  | 8 | — | "Unsupported TGA format, 8 bit", no image |
  | 16 | 0, A1R5G5B5 | 16-bit words copied |
  | 24 | 2, R8G8B8 | file B, G, R reordered to R, G, B |
  | 32 | `0x08101800`, A8R8G8B8 | file B, G, R, A copied as 32-bit words, i.e. `0xAARRGGBB` |

- **Orientation.** The three `…FlipMirror` converters write the output backwards while reading
  each source row backwards. The net effect is a vertical flip only. Every TGA is taken as
  bottom-up, and **the descriptor's origin bits (4 and 5) are ignored**: a top-left-origin TGA
  loads upside down, and a right-origin TGA is not mirrored. The alpha-depth bits are ignored too.
  Output rows are top-down, like the JPEG loader's.

The shipped TGAs are all 32-bit, uncompressed, origin bottom-left (descriptor `0x08`): the six
1024 × 512 planet maps in `gfx/shaders/` and the 32 × 32 mod icons (`favicon.tga`).

**In the port** ([`port/src/video/ImageLoaderStb.cpp`](../../port/src/video/ImageLoaderStb.cpp)) both
loaders decode with stb_image and keep the rules above: the name and content tests, R8G8B8 JPEGs, TGA
types 2 and 10 at 16/24/32 bits only, and the always-bottom-up orientation (a top-left-origin file
is flipped back to how the original shows it). Grayscale JPEGs decode correctly, and 16-bit TGAs
load opaque. The BMP loader ([`ImageLoaderBmp.cpp`](../../port/src/video/ImageLoaderBmp.cpp)) is
Irrlicht 0.7's: `.bmp` name test, `BM` content test, uncompressed 1/4/8-bit (to A1R5G5B5 through the
palette) and 24-bit files; its only user is the GUI's built-in font, a 128 × 128 4-bit BMP loaded from
memory as `#DefaultFont`, which only the content test finds. The PSD and PCX loaders accept nothing.

## Colour formats

daisy keeps Irrlicht's `ECOLOR_FORMAT` numbers for the 16-bit and 24-bit formats, but stores
A8R8G8B8 as `0x08101800`, not 3. The bytes 8, 16, 24 and 0 look like the green, red, alpha and blue
bit shifts; that reading is inferred. The binaries compare against this value
([`IVideoDriver.h`](../../src/ox/video/IVideoDriver.h), Mac `COpenGLTexture::getImageData` `0x152d68`).

| Value | Format | Used by |
|---|---|---|
| 0 | A1R5G5B5 | 16-bit TGA |
| 1 | 16-bit, R5G6B5 in Irrlicht's numbering (inferred) | the main menu's black `AtrumBlack` fallback, written as 16-bit zeros |
| 2 | R8G8B8 | JPEG, 24-bit TGA, opaque sprite package planes |
| `0x08101800` | A8R8G8B8 | 32-bit TGA, sprite package planes with alpha |

## Texture creation flags

`setTextureCreationFlag(flag, enabled)` (Linux `0x4dfab0`) is Irrlicht 0.7's, and so are the flag
values:

| Value | Irrlicht name |
|---|---|
| 1 | `ETCF_ALWAYS_16_BIT` |
| 2 | `ETCF_ALWAYS_32_BIT` |
| 4 | `ETCF_OPTIMIZED_FOR_QUALITY` |
| 8 | `ETCF_OPTIMIZED_FOR_SPEED` |
| `0x10` | `ETCF_CREATE_MIP_MAPS` |

The first four are exclusive: enabling one clears the other three.

- **Defaults.** The `CVideoNull` constructor (Linux `0x4e21d0`) enables
  `ETCF_OPTIMIZED_FOR_SPEED` and **disables** mip maps. Irrlicht 0.7 enables them.
- **Who changes them.** The main menu enables `ETCF_OPTIMIZED_FOR_QUALITY` on its first load step.
  `CSpritePackage::readTexture` enables `OPTIMIZED_FOR_SPEED` before building an opaque plane and
  `OPTIMIZED_FOR_QUALITY` before an alpha plane. The flags are global state, so they stay as the
  last caller left them.
- **Who reads them.** The OpenGL driver's `createDeviceDependentTexture` (Linux `0x4eb630`) reads
  only `ETCF_CREATE_MIP_MAPS`, and the recovered `CVideoNull` confirms nothing else reads the
  16/32-bit and quality/speed flags, so the main menu's and sprite packages' flag changes have no
  effect on the textures they create.

## Textures the game loads by name

| Name | Loaded by |
|---|---|
| `$GAME_RESOURCES$/harvestClientData/gfx/skybox{Roof,North,West,East,South,Floor}.jpg` | main menu skybox; Roof is also Atrum's texture |
| `$GAME_RESOURCES$/harvestClientData/gfx/particlewhite.jpg` | main menu sun |
| `$GAME_RESOURCES$/harvestClientData/gfx/shaders/{Heph,Pos,Ares}{DiffSpec,NormGlow}.tga` | planets with shaders |
| `$GAME_RESOURCES$/harvestClientData/gfx/shaders/{Heph,Pos,Ares}NoShader.jpg` | planets without shaders |
| `<mod path>favicon.tga` | the mod list's icons |
| `AtrumBlack` | added from memory when `skyboxRoof.jpg` fails to load |

Everything else the game draws comes from the sprite packages (`.dat`, `.fnt`), whose planes go
through `addTexture` as described above. See [menu-scene.md](menu-scene.md) for how the planet
maps are sampled.
