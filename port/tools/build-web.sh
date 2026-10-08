#!/usr/bin/env bash
# Cloudflare Pages' Linux build image. Keep these versions in sync with the web CI workflow.
set -euo pipefail
cd "$(dirname "$0")/../.."
tools_dir="$PWD/port/.web-toolchain"
mkdir -p "$tools_dir"
if [[ ! -x "$tools_dir/zig/zig" ]]; then
  curl --fail --location --retry 3 https://ziglang.org/download/0.17.0/zig-x86_64-linux-0.17.0.tar.xz -o "$tools_dir/zig.tar.xz"
  echo "1cbe9df9f27e6b78d14ccbca43b6703a404ef79ef1c463de901d7f088d4e2026  $tools_dir/zig.tar.xz" | sha256sum --check
  mkdir -p "$tools_dir/zig"
  tar -xJf "$tools_dir/zig.tar.xz" --strip-components=1 -C "$tools_dir/zig"
fi
if [[ ! -d "$tools_dir/emsdk" ]]; then
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$tools_dir/emsdk"
fi
"$tools_dir/emsdk/emsdk" install 6.0.10
"$tools_dir/emsdk/emsdk" activate 6.0.10
source "$tools_dir/emsdk/emsdk_env.sh"
export PATH="$tools_dir/zig:$PATH"
cd port
zig build harvest -Dtarget=wasm32-emscripten -Doptimize=ReleaseFast -p zig-out/web
