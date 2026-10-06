# Port census

What the recovered source needs from the port, as of milestone 1 (scaffold). Build instructions are in
[`port/README.md`](../../port/README.md); this page is regenerated in part by
`uv run python port/tools/census.py` (port census) and `... --original` (the matching build's view).

## The build

`port/build.zig` (zig 0.17.0) compiles the kept units into `libharvest.a` for the host with
`zig c++ -std=gnu++98 -DHARVEST_PORT`, plus `-Wno-deprecated-declarations` (`sprintf`) and
`-Wno-c++11-compat-deprecated-writable-strings` (string literals bound to `char*`). gnu++98 is GCC 4.4's
default, and with it **every kept unit compiles with clang/libc++ on macOS arm64**, in Debug (UBSan on)
and ReleaseFast. Nothing needed `-fno-sanitize=undefined`: UBSan only acts at run time, so the spots in
[original-bugs.md](original-bugs.md) will trap when reached, not when compiled.

`zig build census` links every kept object with a stub `main` (`port/census/main.cpp`, the loop of
`HarvestFull/main.cpp`) and fails with the unresolved symbols listed below.

### Kept units (119)

| Directory | Units |
|---|---|
| `HarvestFull/harvest` | CHarvestFullMain |
| `HarvestFull/harvest/entity` (17) | CAlienEntity, CBuildableItems, CBuildingEntity, CConstructionEntity, CCreativeEntity, CDefenseTowerEntity, CDropshipEntity, CEntityManager, CHarvestEntity, CMinerEntity, CMineralsEntity, CMissileTurretEntity, CPerimeterBomb, CShuttleEntity, CSparkEntity, CSparkMoverEntity, CSparkProducerEntity |
| `HarvestFull/harvest/game` (5) | CLuaManager, CScenario, CStatistics, CThreatLevel, CWorld |
| `HarvestFull/harvest/gfx` | CScatterShader |
| `HarvestFull/harvest/gui` (12) | CAchievementsScreen, CGuiEffects, CGuiInfoLines, CHighscoreScreen, CIngameMenuScreen, CMenuInfoDialog, CPriorityScreen, CProfileScreen, CSaveGameScreen, CSettingsScreen, CStatisticsScreen, CStoryScreen |
| `HarvestFull/harvest/settings` (5) | CAlienPriorities, CHarvestProfile, CProfileManager, CSavestateInfo, CSystemConfig |
| `HarvestFull/harvest/states` (5) | CIntroState, CLoadingScreen, CMainMenuState, CPlayState, CShuttleRaceState |
| `daisy/audio` | CAudioDriver |
| `daisy/gui` (25) | CGUIButton, CGUICheckBox, CGUIClickArea, CGUIComboBox, CGUIContextMenu, CGUIEditBox, CGUIEnvironment, CGUIFileOpenDialog, CGUIFont, CGUIImage, CGUIInOutFader, CGUIListBox, CGUIMenu, CGUIMeshViewer, CGUIMessageBox, CGUIModalScreen, CGUIPopupMenu, CGUIRadioList, CGUIScrollBar, CGUISkin, CGUIStaticText, CGUITabControl, CGUIToolBar, CGUIWindow, CUnicodeFont |
| `daisy/io` (8) | CFileList, CFileSystem, CLimitReadFile, CMemoryReadFile, CReadFile, CWriteFile, CZipFileList, CZipReader |
| `daisy/video` | CVideoModeList |
| `daisy/video/Null` (9) | CColorConverter, CFPSCounter, CParticlePackage, CParticleState, CSpriteAnimationImage, CSpriteAnimationImageBundle, CSpriteAnimationState, CSpritePackage, CVideoNull |
| `ox/algo` (5) | CBase64url, CRand, CRegulator, CSimplePress, CTimeCounter |
| `ox/core` (10) | AlwaysAppendToFile, CAes, CBasic, CCipherKey, CCriticalSection, CHiddenFloat, CHiddenInt, CMath, CStringFunctions, CThread |
| `ox/entity` (2) | COxEntity, COxEntityManager |
| `ox/event`, `ox/gui` | IEventReceiver, IGUIElement |
| `ox/game` (5) | CConfigBlock, CConfiguration, CGameMain, CGameState, CTextLocalization |
| `ox/io` (3) | CHelpIO, CMemReadFile, CMemWriteFile |
| `ox/net` (2) | CHTTPConnectionHandler, CVariablePacket |

### Replaced units (16)

The `replaced` list in `build.zig`:

| Unit | What it is | Replaced by |
|---|---|---|
| `daisy/other/CIrrDeviceLinux` | SFML window, events, video modes, cursor, swap; `IImagePresenter`, `ICursorControl` | SDL3 device |
| `daisy/other/CIrrDeviceStub` | device glue shared by the platform devices (see below: portable) | SDL3 device |
| `daisy/other/CLinuxOperator` | GTK clipboard, `system()` URL opening, application-support path | SDL3 |
| `daisy/other/CLogger` | `ILogger` to stdout (see below: portable) | port logger |
| `daisy/other/os.cpp` | `daisy::os::Printer` (log through the device logger) and `Timer` (`gettimeofday`) | port |
| `daisy/input/CJoystickLinuxDriver`, `CJoystickNullDriver` | SFML joysticks, and the no-joystick driver | SDL3 gamepads |
| `daisy/audio/COpenALDriver` | OpenAL/ALUT and libvorbisfile backend of `CAudioDriver` | miniaudio |
| `daisy/video/OpenGL/CVideoOpenGL`, `COpenGLTexture`, `COpenGLCGMaterialRenderer`, `COpenGLSLMaterialRenderer`, `COpenGLShaderMaterialRenderer` | OpenGL 1.x driver, textures, Cg/GLSL/ARB material renderers | GLES3 renderer |
| `daisy/video/Null/CCGMaterialRenderer` | Cg material renderer base (Cg runtime calls) | GLES3 renderer |
| `daisy/video/Software/CZBuffer` | software renderer z-buffer, never reached | dropped |
| `HarvestFull/main.cpp` | the blocking `init`/`update` loop | SDL3 main callbacks |

**Portable replaced units.** `CIrrDeviceStub`, `CLogger`, `os.cpp` and `CJoystickNullDriver` compile
unchanged with zig on macOS. `CIrrDeviceStub` is the platform-neutral half of the device (it owns the
file system, GUI environment, scene manager, network devices, logger, timer, joystick and audio
pointers; `CIrrDeviceLinux` derives from it). The SDL3 device can derive from it the same way, which
keeps its behaviour and removes `Printer`/`Timer`/`CLogger` from the work list; move those four out of
`replaced` when the device lands (`os.cpp`'s `gettimeofday` needs a Windows path).

## Source changes for the port

Both are inside `#ifdef HARVEST_PORT`, so the matching build preprocesses to the same tokens; the full
match is unchanged (4386 exact functions over 135 units before and after, every unit's count equal).

| File | Change | Why |
|---|---|---|
| [`daisy/io/CReadFile.cpp`](../../src/daisy/io/CReadFile.cpp) | `getModifiedDate` uses `struct stat`/`fstat` | `stat64`/`fstat64` are glibc's large-file API; macOS (and the other targets) have a 64-bit `stat` |
| [`daisy/gui/CGUIEnvironment.cpp`](../../src/daisy/gui/CGUIEnvironment.cpp) | includes `ox/gui/IGUIStaticTextInline.h` | `IGUIStaticText::breakText` is defined only in that header, which no unit includes, so nothing emitted it. The original emits it in this object (`0x500ca0`), so the matching build may want the include too |

## Compiler diagnostics worth knowing

clang flags these in kept units (all already present in the original):

- `CStringFunctions.h:28`, `CStringConversions.h:27`: scalar `delete` on `new wchar_t[]` (in the ledger).
- `COxEntity.cpp:23,28`: both constructors initialise `Position.Y` from the uninitialised member
  itself (commented in the source, not yet in [original-bugs.md](original-bugs.md)). Every entity
  starts with an indeterminate Y until something sets its position.
- `CVideoNull.cpp:775`: `getDynamicLight` returns a null reference (in the ledger).
- `CSystemConfig.cpp:340`: `printf(filename)` with a language file name as the format string.
- `CShuttleRaceState.cpp:273`: `if (Shuttles)` tests an array, always true.
- `-Wundefined-inline` for `IGUILayout` sorters and constructor in units that see only `IGUILayout.h`:
  harmless, the units that include `IGUILayoutInline.h` emit them.

## Link census

80 symbols are unresolved when every kept unit is linked on macOS (libc, libc++ and the C math
library resolve from the system). By owner:

### (a) Platform seams the port implements (5)

| Symbol | Callers | Suggestion |
|---|---|---|
| `ox::createDevice(E_DRIVER_TYPE, IEventReceiver*, const wchar_t*)` (`extern "C"`) | `CHarvestFullMain::init` | implement in port/: create the SDL3 device (a `CIrrDeviceStub` subclass) |
| `daisy::os::Printer::log(const char*, ELOG_LEVEL, ...)` | `CGUIEnvironment::loadBuidInFont`, `CGUIFont::loadTexture`, `readPositions16bit`, `readPositions32bit`, `CVideoNull::addShaderMaterial`, `addHighLevelShaderMaterial`, `checkPrimitiveCount`, `makeColorKeyTexture` | keep `os.cpp` |
| `daisy::os::Printer::log(const char*, const char*, ELOG_LEVEL)` | `CFileSystem::getZipReader`, `CZipReader::openFile`, `CVideoNull::getTexture`, `loadTextureFromFile`, `createImageFromFile`, `getSpritePackage`, `getParticlePackage`, `add*ShaderMaterialFromFiles` | keep `os.cpp` |
| `daisy::os::Printer::log(const wchar_t*, ELOG_LEVEL, ...)` | `CVideoNull::addCgShaderMaterial`, `allocatePPSurfaces`, `captureScreenBuffer`, `createScreenTexture`, `setInputTexture` | keep `os.cpp` |
| `daisy::os::Timer::getTime()` | `CAudioDriver::playSound`, `playOrientedSound`, `CGUIEditBox::draw`, `processKey`, `setFocus`, `CGUIInOutFader::draw`, `fadeIn`, `fadeOut`, `isReady`, `CGUIDetachableFrame::OnEvent`, `draw`, `CGUIMeshViewer::draw`, `CGUIStaticText::draw`, `activateProgressiveReveal`, `CVideoNull::endScene` | keep `os.cpp`, or implement over `SDL_GetTicks` (milliseconds) |

### (b) Unrecovered daisy/ox/game code the kept units reach (14)

| Symbol | Callers | Suggestion |
|---|---|---|
| `daisy::gui::BuildInFontData`, `BuildInFontDataSize` | `CGUIEnvironment::loadBuidInFont` | reuse: the 8310 bytes at `0x868fe0` are identical to `third_party/irrlicht-0.7/source/Irrlicht/BuildInFont.h`; define them in port/ from that file (as `unsigned char[]`/`int`, the types in `daisy/gui/BuildInFont.h`) |
| `daisy::video::CImage::CImage(ECOLOR_FORMAT, const CDimension2d<int>&)` | `CSpritePackage::readTexture`, `CVideoNull::addTexture` | implement in port/ from Irrlicht 0.7 `CImage.cpp` against the recovered `daisy/video/Null/CImage.h` (all the 2D textures are built through it) |
| `daisy::video::CImage::CImage(ECOLOR_FORMAT, const CDimension2d<int>&, void*)` | `CVideoNull::createImageFromData` | same |
| `daisy::video::createImageLoaderJPG()`, `createImageLoaderTGA()` | `CVideoNull::CVideoNull` | implement in port/ over stb_image with the rules in [textures.md](textures.md) (TGA always flipped) |
| `daisy::video::createImageLoaderBmp()`, `createImageLoaderPCX()`, `createImageLoaderPSD()` | `CVideoNull::CVideoNull` | stub: loaders that accept nothing (the game ships no such files), or reuse Irrlicht 0.7 |
| `daisy::video::CSoftwareTexture::CSoftwareTexture(IImage*)` | `CVideoNull::createDeviceDependentTexture` | stub or reuse Irrlicht 0.7 `CSoftwareTexture.cpp`; the renderer overrides `createDeviceDependentTexture`, so it is never called |
| `daisy::io::CTextReader::CTextReader(IReadFile*)`, `CXMLReader::CXMLReader(CTextReader*)` | `CFileSystem::createXMLReader` | stub: the game never uses XML (the headers are partial, sized only) |
| `daisy::io::CXMLWriter::CXMLWriter(IWriteFile*)` | `CFileSystem::createXMLWriter` | stub |
| `harvest::entity::g_nextEntityId` (`int`, `.data` `0x868e00`, initial value 1) | constructors of every entity (`CAlienEntity`, `CConstructionEntity`, `CCreativeEntity`, `CDefenseTowerEntity`, `CDropshipEntity`, `CMineralGatherEntity`, `CMineralsEntity`, `CMissileTurretEntity`, `CPerimeterBombExplosion`, `CShuttleEntity`, `CSparkEntity`, `CSparkMoverEntity`, `CSparkProducerEntity`), `CPlayState::initializeNewGame`, `readStateFromFile`, `writeStateToFile`, `CShuttleRaceState::secondInit` | declared in `CEntityManager.h`, defined by an unrecovered game unit; define `int g_nextEntityId = 1;` in port/ until that unit is recovered (it is saved in save games) |

### (c) Third-party libraries (61)

| Library | Symbols | Callers | Suggestion |
|---|---|---|---|
| Lua 5.1 C API (52) | `luaL_checkudata luaL_error luaL_getmetafield luaL_loadfile luaL_loadstring luaL_newmetatable luaL_newstate luaL_openlibs luaL_optlstring luaL_ref luaL_register luaL_typerror luaL_unref lua_call lua_checkstack lua_close lua_createtable lua_error lua_getfield lua_getinfo lua_getstack lua_gettable lua_gettop lua_insert lua_isnumber lua_isstring lua_newuserdata lua_next lua_pcall lua_pushboolean lua_pushcclosure lua_pushfstring lua_pushinteger lua_pushlightuserdata lua_pushlstring lua_pushnil lua_pushnumber lua_pushstring lua_pushvalue lua_rawgeti lua_rawset lua_remove lua_replace lua_setmetatable lua_settable lua_settop lua_toboolean lua_tointeger lua_tolstring lua_tonumber lua_touserdata lua_type` | `CLuaManager` (all, through `ox/lua/lunar.h`), `CCreativeEntity`, `CAlienEntity`, `CBuildingEntity` | vendor PUC Lua 5.1.5 (all are plain 5.1 API, nothing LuaJIT-specific); the build still points at LuaJIT's 5.1 headers in `third_party/luajit-2.0.0-beta8/include` until then |
| zlib (7) | `deflate deflateEnd deflateInit_ inflate inflateEnd inflateInit2_ inflateInit_` | `CFileSystem` (save-game streams), `CZipReader` (raw inflate) | vendor zlib (or miniz with its zlib API); the host build finds `zlib.h` in the macOS SDK, cross targets do not |
| Cg (2) | `cgCreateContext cgDestroyContext` | `CVideoNull::CVideoNull`, `~CVideoNull` | stub in port/ (return a null context): the context is only handed to the Cg material renderers, which are replaced |

### (d) Other

None. libc, POSIX and libc++ symbols all resolve on macOS; the ones that need porting elsewhere are
listed next.

## C library and POSIX calls in kept units

`census.py --libc` lists every C library call. Those that are not ISO C, or behave differently per
platform:

| Calls | Units | Concern |
|---|---|---|
| `glob`, `globfree` | `CFileList` | no `glob.h` on Windows (the cross build fails here); list directories with SDL3 (`SDL_EnumerateDirectory`/`SDL_GlobDirectory`) |
| `getcwd`, `chdir`, `mkdir` (unistd, `sys/stat`) | `CFileSystem` | Windows spellings differ; the web needs a virtual file system |
| `fileno`, `fstat` | `CReadFile` | fine on POSIX; `_fstat` on Windows |
| `pthread_create`, `pthread_join`, `usleep` | `CThread` | used by `CHTTPConnectionHandler` and by `CGameMain::update` (`CThread::sleep(20)` while the window is inactive); needs a Windows/web implementation (SDL3 threads, or no threads on the web) |
| `pthread_mutex_*` | `CCriticalSection` | same |
| `localtime`, `strftime`, `time` | `CBasic`, `CHighscoreScreen`, `CPlayState` | fine |
| `mbstowcs`, `wcstombs`, `wcstol`, `vswprintf` | `CConfiguration`, `CConfigBlock`, `CGUIEditBox`, `CGUIFileOpenDialog`, `CHTTPConnectionHandler`, `CSystemConfig`, `CAlienPriorities`, `CHarvestProfile`, `CHighscoreScreen` | `wchar_t` is 2 bytes on Windows; locale-dependent conversions |

Cross-compiling the library (`zig build -Dtarget=...`) today: `x86_64-linux-gnu` fails only on `zlib.h`;
`x86_64-windows-gnu` fails on `zlib.h` and `glob.h`. Linking pthreads on Windows is untested.

## Seams the linker cannot see

The game reaches most of the platform through interfaces handed out by the device, so these do not
show up as unresolved symbols. The replaced units implemented them; the port must too.

| Interface (`src/ox/...`) | Original implementation | Reached from | Port |
|---|---|---|---|
| `IOxDevice` | `CIrrDeviceLinux` : `CIrrDeviceStub` : `IDaisyDevice` | everything; `CGameState::firstInit` caches driver, GUI, scene manager, audio, joystick | SDL3 device |
| `ITimer` | `daisy::CTimer` (`daisy/other/CTimer.h`) over `os::Timer` | `CGameMain::getNewTimeStep`, `CHarvestFullMain::init` (random seed), `CLuaManager`, `CLoadingScreen`, `CPlayState` | keep, or SDL3 ticks |
| `event::ILogger` | `CLogger`, installed as `os::Printer::Logger` by `CIrrDeviceStub` | only through `os::Printer` | keep `CLogger` |
| `IOSOperator` | `CLinuxOperator` (GTK clipboard, `system()`) | `CHarvestFullMain::init` (`getApplicationSupportPath`), `CGUIEditBox::processKey` (`getTextFromClipboard`), `CGameMain` (`messageBox` on a state error), `CPlayState` (`openURL`, the buy-now link) | SDL3 clipboard, `SDL_GetPrefPath`, `SDL_ShowSimpleMessageBox`, `SDL_OpenURL`; the other methods are unused |
| `gui::ICursorControl` | `CIrrDeviceLinux` | `CPlayState`, `CPriorityScreen` | SDL3 |
| `video::IVideoModeList` | `CVideoModeList` (kept), filled by the device | `CSettingsScreen` | SDL3 display modes |
| `video::IVideoDriver`, `ITexture`, `IMaterialRenderer`, `IGPUProgrammingServices` | `CVideoOpenGL` : `CVideoNull`, `COpenGLTexture`, the material renderers | every state; `CScatterShader` and `CMainMenuState` use shader materials | GLES3 renderer deriving from `CVideoNull`, per [renderer.md](renderer.md) |
| `video::IImagePresenter` | `CIrrDeviceLinux` | the software driver only | not needed |
| `audio::IAudioDriver` | `COpenALDriver` : `CAudioDriver` (kept; the backend implements the `device*` pure virtuals) | `CGameMain::update` (`periodicStreamUpdate`), every state | miniaudio backend deriving from `CAudioDriver`, per [audio.md](audio.md) |
| `input::IJoystickDriver` | `CJoystickLinuxDriver` (SFML) / `CJoystickNullDriver` | cached by `CGameState::firstInit`, used by `CShuttleRaceState` (second player) | SDL3 gamepads |
| `scene::ISceneManager` and nodes | unrecovered `daisy::scene::createSceneManager` (`0x4c5290`), called by `CIrrDeviceStub::createGUIAndScene` | `CMainMenuState` (planets, atmosphere, light, camera, rotation animator), `CScatterShader` | the menu scene subset, per [menu-scene.md](menu-scene.md) |
| `net::INetworkDevice` | unrecovered `daisy::net::CWinsockNetworkDevice` (`CIrrDeviceStub::createNetworkDevice`) | `CHTTPConnectionHandler::init` (`createNetworkDevice("HTTPPrimDev", 1)`, then `grab()` without a null check) | a stub device whose connects fail; it must not be null |
| `IThreadPool` | `CIrrDeviceStub::initThreadPool` | not called by the kept units | none |

`census.py --original` shows the rest of what the replaced units pulled from unrecovered code and
libraries: `daisy::video::createSoftwareDriver` and `CImageLoaderJPG::saveImage` (JPEG screenshots,
`CVideoOpenGL::saveJpegScreenshot`), plus OpenGL/GLU/GLX, X11, OpenAL/ALUT, libvorbisfile, SFML, GTK
and the Cg runtime.

## Behaviour that will matter for the main-loop seam

- `CGameMain::setState` loops `renderFirst`/`secondInit` until a state finishes loading, inside one
  call; on the web that loop must yield between iterations.
- `CGameMain::update` sleeps 20 ms when the window is inactive (`SleepWhenInactive`).
- `CHarvestFullMain::init` creates the device with `EDT_OPENGL`, may call
  `createUserSelectedDeviceWindow` (the original's launcher dialog), then `createDeviceWindow`.
