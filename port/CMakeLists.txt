# Builds the modern port of Harvest without zig, for distributions and Nix (see ../flake.nix).
#
# The same sources as build.zig: every unit in config/1.18-linux-amd64/units.toml except the
# platform units the port replaces, plus every .cpp under src/. The third-party libraries other
# than SDL3 are compiled from source trees passed in as cache variables; SDL3 comes from
# find_package.
#
#   cmake -S port -B build/cmake -DHARVEST_ZLIB_SOURCE=... -DHARVEST_LUA_SOURCE=... \
#     -DHARVEST_STB_SOURCE=... -DHARVEST_MINIAUDIO_SOURCE=...

cmake_minimum_required(VERSION 3.24)
project(harvest VERSION 0.1.0 LANGUAGES C CXX)

set(HARVEST_ZLIB_SOURCE "" CACHE PATH "zlib 1.3.2 source tree")
set(HARVEST_LUA_SOURCE "" CACHE PATH "PUC Lua 5.1.5 source tree")
set(HARVEST_STB_SOURCE "" CACHE PATH "nothings/stb source tree")
set(HARVEST_MINIAUDIO_SOURCE "" CACHE PATH "miniaudio 0.11.25 source tree")
foreach(dep ZLIB LUA STB MINIAUDIO)
  if(NOT IS_DIRECTORY "${HARVEST_${dep}_SOURCE}")
    message(FATAL_ERROR "set HARVEST_${dep}_SOURCE to the dependency's source tree")
  endif()
endforeach()

find_package(SDL3 REQUIRED CONFIG)

set(repo "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Third-party C, without the game's flags.

add_library(harvest_zlib STATIC)
foreach(f adler32 compress crc32 deflate infback inffast inflate inftrees trees uncompr zutil)
  target_sources(harvest_zlib PRIVATE "${HARVEST_ZLIB_SOURCE}/${f}.c")
endforeach()
target_include_directories(harvest_zlib PUBLIC "${HARVEST_ZLIB_SOURCE}")

add_library(harvest_lua STATIC)
foreach(f lapi lcode ldebug ldo ldump lfunc lgc llex lmem lobject lopcodes lparser lstate lstring
          ltable ltm lundump lvm lzio lauxlib lbaselib ldblib liolib lmathlib loslib ltablib
          lstrlib loadlib linit)
  target_sources(harvest_lua PRIVATE "${HARVEST_LUA_SOURCE}/src/${f}.c")
endforeach()
target_include_directories(harvest_lua PUBLIC "${HARVEST_LUA_SOURCE}/src" "${HARVEST_LUA_SOURCE}/etc")
if(NOT WIN32)
  # POSIX extras (mkstemp, popen, isatty); no dynamic C modules.
  target_compile_definitions(harvest_lua PRIVATE LUA_USE_POSIX)
endif()

add_library(harvest_stb STATIC
  src/thirdparty/stb_image.c
  src/thirdparty/stb_image_write.c)
target_include_directories(harvest_stb PUBLIC "${HARVEST_STB_SOURCE}")

add_library(harvest_miniaudio STATIC src/thirdparty/miniaudio.c)
target_include_directories(harvest_miniaudio PUBLIC "${HARVEST_MINIAUDIO_SOURCE}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # miniaudio loads the platform audio libraries at run time.
  target_link_libraries(harvest_miniaudio PUBLIC dl pthread m)
endif()

# The kept units: the `["path.cpp"]` table headers of units.toml minus the replaced ones. Keep this
# list in step with `replaced` in build.zig.
set(replaced
  "daisy/other/CIrrDeviceLinux.cpp"
  "daisy/other/CLinuxOperator.cpp"
  "daisy/input/CJoystickLinuxDriver.cpp"
  "daisy/audio/COpenALDriver.cpp"
  "daisy/video/OpenGL/"
  "daisy/video/Null/CCGMaterialRenderer.cpp"
  "daisy/video/Software/"
  "HarvestFull/main.cpp")

set(units_toml "${repo}/config/1.18-linux-amd64/units.toml")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${units_toml}")
file(STRINGS "${units_toml}" unit_lines REGEX "^\\[\".*\"\\]$")
set(units)
foreach(line IN LISTS unit_lines)
  string(REGEX REPLACE "^\\[\"(.*)\"\\]$" "\\1" unit "${line}")
  set(keep TRUE)
  foreach(prefix IN LISTS replaced)
    string(FIND "${unit}" "${prefix}" at)
    if(at EQUAL 0)
      set(keep FALSE)
    endif()
  endforeach()
  if(keep)
    list(APPEND units "${repo}/src/${unit}")
  endif()
endforeach()

file(GLOB_RECURSE port_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM port_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_executable(harvest ${units} ${port_sources} src/main.cpp)
# GCC 4.4's default dialect, which the recovered code needs no changes for.
set_target_properties(harvest PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS ON)
target_compile_definitions(harvest PRIVATE HARVEST_PORT)
target_include_directories(harvest PRIVATE "${repo}/src" "${repo}/src/HarvestFull" src)
# The original relies on two's-complement wrapping in its integer arithmetic.
target_compile_options(harvest PRIVATE -fwrapv)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(harvest PRIVATE
    -Wno-deprecated-declarations
    -Wno-c++11-compat-deprecated-writable-strings)
endif()
target_link_libraries(harvest PRIVATE
  harvest_zlib harvest_lua harvest_stb harvest_miniaudio SDL3::SDL3)

if(EMSCRIPTEN)
  # The web build: harvest.js and harvest.wasm next to web/index.html, which takes the player's game
  # data into the in-memory file system and then calls main (see web/index.html).
  set_target_properties(harvest PROPERTIES SUFFIX ".js")
  target_link_options(harvest PRIVATE
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    # The renderer loads every GL entry point through SDL_GL_GetProcAddress.
    -sGL_ENABLE_GET_PROC_ADDRESS=1
    -sALLOW_MEMORY_GROWTH=1
    -sSTACK_SIZE=1MB
    -sFORCE_FILESYSTEM=1
    -sINVOKE_RUN=0
    -sEXIT_RUNTIME=0
    -sEXPORTED_RUNTIME_METHODS=callMain,FS
    -lidbfs.js)
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/harvest.js"
    "${CMAKE_CURRENT_BINARY_DIR}/harvest.wasm"
    web/index.html
    DESTINATION .)
  install(FILES web/assets/cd-rom.webp web/assets/load-flow.webp DESTINATION assets)
else()
  install(TARGETS harvest RUNTIME DESTINATION bin)
endif()
