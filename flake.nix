{
  description = "Harvest: Massive Encounter, the modern port of the recovered source";

  # The port's third-party libraries, pinned to the versions in port/build.zig.zon.
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    zlib = {
      url = "https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz";
      flake = false;
    };
    lua = {
      url = "https://www.lua.org/ftp/lua-5.1.5.tar.gz";
      flake = false;
    };
    stb = {
      url = "github:nothings/stb/2c980bb59875b0d32144a71867fbdebb2f77cd20";
      flake = false;
    };
    miniaudio = {
      url = "github:mackron/miniaudio/0.11.25";
      flake = false;
    };
    sdl = {
      url = "github:libsdl-org/SDL/release-3.4.16";
      flake = false;
    };
  };

  outputs =
    { self, nixpkgs, ... }@inputs:
    let
      systems = [
        "aarch64-darwin"
        "x86_64-darwin"
        "aarch64-linux"
        "x86_64-linux"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (
        pkgs:
        let
          inherit (pkgs) lib;

          # nixpkgs' SDL3 recipe (its platform backends and dependencies) on the pinned source.
          sdl3 = pkgs.sdl3.overrideAttrs { src = inputs.sdl; };

          # The port embeds its shaders with #embed, which needs clang (or GCC 15).
          stdenv = if pkgs.stdenv.cc.isClang then pkgs.stdenv else pkgs.clangStdenv;

          harvest = stdenv.mkDerivation {
            pname = "harvest";
            version = "0.1.0";

            src = lib.fileset.toSource {
              root = ./.;
              fileset = lib.fileset.unions [
                ./src
                ./port/src
                ./port/CMakeLists.txt
                ./port/web
                ./config/1.18-linux-amd64/units.toml
                # the GUI's built-in font bitmap, which port/src/gui/BuildInFont.cpp includes
                ./third_party/irrlicht-0.7/source/Irrlicht/BuildInFont.h
              ];
            };
            cmakeDir = "../port";

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.ninja
            ];
            buildInputs = [ sdl3 ];

            # The recovered source passes non-literal format strings to sprintf and must stay as the
            # original wrote it; -Werror=format-security would reject it.
            hardeningDisable = [ "format" ];

            cmakeFlags = [
              (lib.cmakeFeature "HARVEST_ZLIB_SOURCE" "${inputs.zlib}")
              (lib.cmakeFeature "HARVEST_LUA_SOURCE" "${inputs.lua}")
              (lib.cmakeFeature "HARVEST_STB_SOURCE" "${inputs.stb}")
              (lib.cmakeFeature "HARVEST_MINIAUDIO_SOURCE" "${inputs.miniaudio}")
            ];

            meta = {
              description = "Modern port of Harvest: Massive Encounter (needs the original game's data)";
              mainProgram = "harvest";
              platforms = systems;
            };
          };
          # The web build: SDL3 and the game compiled with Emscripten. The emscripten package's cache
          # is read-only, so each build works on a writable copy of it.
          emscriptenEnv = ''
            export HOME=$TMPDIR
            export EM_CACHE=$TMPDIR/emcache
            cp -r ${pkgs.emscripten}/share/emscripten/cache $EM_CACHE
            chmod -R u+w $EM_CACHE
          '';

          sdl3-web = pkgs.stdenvNoCC.mkDerivation {
            pname = "sdl3-web";
            version = "3.4.16";
            src = inputs.sdl;
            nativeBuildInputs = [
              pkgs.emscripten
              pkgs.cmake
              pkgs.ninja
            ];
            dontUseCmakeConfigure = true;
            configurePhase = ''
              runHook preConfigure
              ${emscriptenEnv}
              emcmake cmake -S . -B build -G Ninja \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_INSTALL_PREFIX=$out \
                -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
              runHook postConfigure
            '';
            buildPhase = "ninja -C build";
            installPhase = "ninja -C build install";
          };

          web = pkgs.stdenvNoCC.mkDerivation {
            pname = "harvest-web";
            version = "0.1.0";
            inherit (harvest) src;
            nativeBuildInputs = [
              pkgs.emscripten
              pkgs.cmake
              pkgs.ninja
            ];
            dontUseCmakeConfigure = true;
            configurePhase = ''
              runHook preConfigure
              ${emscriptenEnv}
              emcmake cmake -S port -B build -G Ninja \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_INSTALL_PREFIX=$out \
                -DCMAKE_FIND_ROOT_PATH=${sdl3-web} \
                -DSDL3_DIR=${sdl3-web}/lib/cmake/SDL3 \
                ${lib.concatStringsSep " " harvest.cmakeFlags}
              runHook postConfigure
            '';
            buildPhase = "ninja -C build";
            installPhase = "ninja -C build install";
          };
        in
        {
          inherit harvest sdl3 sdl3-web web;
          default = harvest;
        }
      );

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.harvest ];
        };
      });
    };
}
