//! Builds the modern port of Harvest from the recovered source in ../src and the port's own
//! sources in src/.
//!
//! The kept units are every unit in config/1.18-linux-amd64/units.toml except the platform units the
//! port replaces (`replaced` below), so newly recovered units join the build without edits here.
//! Port sources are every .cpp under src/ except src/main.cpp, so new platform code joins the same
//! way. Dependencies are pinned in build.zig.zon and built from source for the target.

const std = @import("std");

const units_toml = "../config/1.18-linux-amd64/units.toml";

/// Recovered units the port replaces with its own implementations, as path prefixes under src/.
const replaced = [_][]const u8{
    // Linux device (X11/SFML window and events) and OS operator (GTK clipboard): the SDL3 device
    // replaces them. The device stub, logger and os helpers are kept: the SDL3 device derives from
    // CIrrDeviceStub as the Linux device did.
    "daisy/other/CIrrDeviceLinux.cpp",
    "daisy/other/CLinuxOperator.cpp",
    // SFML joysticks: SDL3 gamepads replace them. The null driver is kept.
    "daisy/input/CJoystickLinuxDriver.cpp",
    // OpenAL/ALUT backend: miniaudio replaces it.
    "daisy/audio/COpenALDriver.cpp",
    // OpenGL 1.x driver, its textures and Cg/GLSL/ARB material renderers: the GLES3 renderer
    // replaces them.
    "daisy/video/OpenGL/",
    "daisy/video/Null/CCGMaterialRenderer.cpp",
    // The software renderer's z-buffer, never reached by the game.
    "daisy/video/Software/",
    // The blocking main loop: the port runs the frame step from SDL3's main callbacks.
    "HarvestFull/main.cpp",
};

/// Flags for the recovered C++ and the port's C++. GCC 4.4 defaulted to gnu++98, which needs no
/// source changes.
const cxx_flags = [_][]const u8{
    "-std=gnu++98",
    "-DHARVEST_PORT",
    // sprintf and friends in the original source, and string literals bound to char*.
    "-Wno-deprecated-declarations",
    "-Wno-c++11-compat-deprecated-writable-strings",
};

/// The core of zlib: the game only uses deflate and inflate on memory (no gz* file API).
const zlib_sources = [_][]const u8{
    "adler32.c", "compress.c", "crc32.c",   "deflate.c", "infback.c", "inffast.c",
    "inflate.c", "inftrees.c", "trees.c",   "uncompr.c", "zutil.c",
};

/// PUC Lua 5.1.5's core and standard libraries (src/ without the lua and luac programs).
const lua_sources = [_][]const u8{
    "lapi.c",    "lcode.c",   "ldebug.c",   "ldo.c",      "ldump.c",   "lfunc.c",   "lgc.c",
    "llex.c",    "lmem.c",    "lobject.c",  "lopcodes.c", "lparser.c", "lstate.c",  "lstring.c",
    "ltable.c",  "ltm.c",     "lundump.c",  "lvm.c",      "lzio.c",    "lauxlib.c", "lbaselib.c",
    "ldblib.c",  "liolib.c",  "lmathlib.c", "loslib.c",   "ltablib.c", "lstrlib.c", "loadlib.c",
    "linit.c",
};

const lua_headers = [_][]const u8{ "src/lua.h", "src/luaconf.h", "src/lualib.h", "src/lauxlib.h", "etc/lua.hpp" };

/// The third-party libraries every game module links.
const Libraries = struct {
    zlib: *std.Build.Step.Compile,
    lua: *std.Build.Step.Compile,
    stb_image: *std.Build.Step.Compile,
    miniaudio: *std.Build.Step.Compile,
    sdl: *std.Build.Step.Compile,

    fn all(libs: Libraries) [5]*std.Build.Step.Compile {
        return .{ libs.zlib, libs.lua, libs.stb_image, libs.miniaudio, libs.sdl };
    }
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const libs: Libraries = .{
        .zlib = buildZlib(b, target, optimize),
        .lua = buildLua(b, target, optimize),
        .stb_image = buildStbImage(b, target, optimize),
        .miniaudio = buildMiniaudio(b, target, optimize),
        .sdl = b.dependency("sdl", .{
            .target = target,
            .optimize = optimize,
            .preferred_linkage = .static,
        }).artifact("SDL3"),
    };
    for (libs.all()) |lib| b.installArtifact(lib);

    const units = keptUnits(b);
    const port_sources = portSources(b);

    // The recovered code and the port's sources as a library: the default step, which builds while
    // the platform seams are still missing.
    const harvest_lib = b.addLibrary(.{
        .name = "harvest",
        .root_module = gameModule(b, target, optimize, libs, units, port_sources),
    });
    b.installArtifact(harvest_lib);

    // The game. It links every kept object and port source, so it only links once every seam
    // exists; until then `zig build harvest` lists what is missing, like the census.
    const exe_module = gameModule(b, target, optimize, libs, units, port_sources);
    exe_module.addCSourceFile(.{ .file = b.path("src/main.cpp"), .flags = &cxx_flags });
    const exe = b.addExecutable(.{ .name = "harvest", .root_module = exe_module });
    const install_exe = b.addInstallArtifact(exe, .{});
    b.step("harvest", "Build the game executable").dependOn(&install_exe.step);

    const run = b.addRunArtifact(exe);
    run.step.dependOn(&install_exe.step);
    run.addPassthruArgs();
    b.step("run", "Run the game").dependOn(&run.step);

    // The renderer's test (tests/video_test.cpp): an SDL3 window with an OpenGL context, the GLES3
    // driver and a frame drawn from the original data. It links the library, so it needs only the
    // renderer's seam, not the device's.
    const video_test_module = b.createModule(.{ .target = target, .optimize = optimize, .link_libcpp = true });
    video_test_module.addIncludePath(b.path("../src"));
    video_test_module.addIncludePath(b.path("../src/HarvestFull"));
    video_test_module.addIncludePath(b.path("src"));
    video_test_module.addCSourceFile(.{ .file = b.path("tests/video_test.cpp"), .flags = &cxx_flags });
    video_test_module.linkLibrary(harvest_lib);
    for (libs.all()) |lib| video_test_module.linkLibrary(lib);
    const video_test = b.addExecutable(.{ .name = "video-test", .root_module = video_test_module });
    const install_video_test = b.addInstallArtifact(video_test, .{});
    b.step("video-test", "Build the renderer test").dependOn(&install_video_test.step);
    const run_video_test = b.addRunArtifact(video_test);
    run_video_test.step.dependOn(&install_video_test.step);
    run_video_test.addPassthruArgs();
    b.step("run-video-test", "Run the renderer test (arguments after --)").dependOn(&run_video_test.step);

    // The census links the same objects against the original's plain loop (census/main.cpp)
    // instead of the SDL3 entry point, so the linker lists exactly what the recovered code and the
    // port's sources still need.
    const census_module = gameModule(b, target, optimize, libs, units, port_sources);
    census_module.addCSourceFile(.{ .file = b.path("census/main.cpp"), .flags = &cxx_flags });
    const census = b.addExecutable(.{ .name = "harvest-census", .root_module = census_module });
    b.step("census", "Link every kept unit and report unresolved symbols").dependOn(&b.addInstallArtifact(census, .{}).step);

    // Test programs: tests/<name>.cpp links against the library (only the objects it needs) and
    // runs with `zig build test-<name> -- args`; `zig build tests` builds them all without running.
    const tests_step = b.step("tests", "Build the test programs");
    for (testSources(b)) |source| {
        const name = source[0 .. source.len - ".cpp".len];
        const module = b.createModule(.{ .target = target, .optimize = optimize, .link_libcpp = true });
        addIncludePaths(b, module);
        module.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "tests", source })), .flags = &cxx_flags });
        module.linkLibrary(harvest_lib);
        for (libs.all()) |lib| module.linkLibrary(lib);
        const test_exe = b.addExecutable(.{ .name = b.fmt("harvest-test-{s}", .{name}), .root_module = module });
        const install_test = b.addInstallArtifact(test_exe, .{});
        tests_step.dependOn(&install_test.step);
        const run_test = b.addRunArtifact(test_exe);
        run_test.step.dependOn(&install_test.step);
        run_test.addPassthruArgs();
        b.step(b.fmt("test-{s}", .{name}), b.fmt("Build and run tests/{s}", .{source})).dependOn(&run_test.step);
    }
}

fn addIncludePaths(b: *std.Build, module: *std.Build.Module) void {
    module.addIncludePath(b.path("../src"));
    module.addIncludePath(b.path("../src/HarvestFull"));
    module.addIncludePath(b.path("src"));
}

/// The .cpp files directly in tests/.
fn testSources(b: *std.Build) []const []const u8 {
    const io = b.graph.io;
    const root = b.root.join(b.allocator, "tests") catch @panic("OOM");
    var dir = root.root_dir.handle.openDir(io, root.sub_path, .{ .iterate = true }) catch return &.{};
    defer dir.close(io);
    b.dependOnDirectoryContents(b.path("tests"));

    var sources: std.ArrayList([]const u8) = .empty;
    var it = dir.iterate();
    while (it.next(io) catch |err| std.debug.panic("cannot list tests: {t}", .{err})) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".cpp"))
            sources.append(b.allocator, b.dupe(entry.name)) catch @panic("OOM");
    }
    std.mem.sort([]const u8, sources.items, {}, lessThan);
    return sources.items;
}

fn gameModule(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    libs: Libraries,
    units: []const []const u8,
    port_sources: []const []const u8,
) *std.Build.Module {
    const module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });
    addIncludePaths(b, module);
    module.addCSourceFiles(.{ .root = b.path("../src"), .files = units, .flags = &cxx_flags });
    module.addCSourceFiles(.{ .root = b.path("src"), .files = port_sources, .flags = &cxx_flags });
    for (libs.all()) |lib| module.linkLibrary(lib);
    return module;
}

fn cModule(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Module {
    return b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        // Third-party C keeps its own (well-defined in practice) undefined behaviour; UBSan stays
        // on for the game code.
        .sanitize_c = .off,
    });
}

fn buildZlib(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Step.Compile {
    const dep = b.dependency("zlib", .{});
    const module = cModule(b, target, optimize);
    module.addCSourceFiles(.{ .root = dep.path(""), .files = &zlib_sources, .flags = &.{"-std=c11"} });
    const lib = b.addLibrary(.{ .name = "z", .root_module = module });
    lib.installHeader(dep.path("zlib.h"), "zlib.h");
    lib.installHeader(dep.path("zconf.h"), "zconf.h");
    return lib;
}

fn buildLua(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Step.Compile {
    const dep = b.dependency("lua", .{});
    const module = cModule(b, target, optimize);
    // POSIX extras (mkstemp, popen, isatty) everywhere but Windows; no dynamic C modules.
    if (target.result.os.tag != .windows) module.addCMacro("LUA_USE_POSIX", "1");
    module.addCSourceFiles(.{ .root = dep.path("src"), .files = &lua_sources, .flags = &.{"-std=gnu99"} });
    const lib = b.addLibrary(.{ .name = "lua", .root_module = module });
    for (lua_headers) |header| lib.installHeader(dep.path(header), std.fs.path.basename(header));
    return lib;
}

fn buildStbImage(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Step.Compile {
    const dep = b.dependency("stb", .{});
    const module = cModule(b, target, optimize);
    module.addIncludePath(dep.path(""));
    module.addCSourceFile(.{ .file = b.path("src/thirdparty/stb_image.c"), .flags = &.{"-std=c99"} });
    module.addCSourceFile(.{ .file = b.path("src/thirdparty/stb_image_write.c"), .flags = &.{"-std=c99"} });
    const lib = b.addLibrary(.{ .name = "stb_image", .root_module = module });
    lib.installHeader(dep.path("stb_image.h"), "stb_image.h");
    lib.installHeader(dep.path("stb_image_write.h"), "stb_image_write.h");
    return lib;
}

fn buildMiniaudio(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Step.Compile {
    const dep = b.dependency("miniaudio", .{});
    const module = cModule(b, target, optimize);
    module.addIncludePath(dep.path(""));
    module.addCSourceFile(.{ .file = b.path("src/thirdparty/miniaudio.c"), .flags = &.{"-std=c99"} });
    // miniaudio loads the platform audio libraries at run time; on Linux that needs libdl.
    if (target.result.os.tag == .linux) {
        module.linkSystemLibrary("dl", .{});
        module.linkSystemLibrary("pthread", .{});
        module.linkSystemLibrary("m", .{});
    }
    const lib = b.addLibrary(.{ .name = "miniaudio", .root_module = module });
    lib.installHeader(dep.path("miniaudio.h"), "miniaudio.h");
    return lib;
}

/// The units listed in units.toml (`["path.cpp"]` table headers) minus the replaced ones.
fn keptUnits(b: *std.Build) []const []const u8 {
    // Reconfigure when the unit list changes instead of poisoning the configuration cache.
    b.dependOnFileContents(b.path(units_toml));
    const file = b.root.join(b.allocator, units_toml) catch @panic("OOM");
    const text = file.root_dir.handle.readFileAlloc(b.graph.io, file.sub_path, b.allocator, .limited(1 << 20)) catch |err|
        std.debug.panic("cannot read {s}: {t}", .{ units_toml, err });

    var units: std.ArrayList([]const u8) = .empty;
    var lines = std.mem.tokenizeScalar(u8, text, '\n');
    while (lines.next()) |raw| {
        const line = std.mem.trim(u8, raw, " \t\r");
        if (!std.mem.startsWith(u8, line, "[\"") or !std.mem.endsWith(u8, line, "\"]")) continue;
        const unit = line[2 .. line.len - 2];
        if (isReplaced(unit)) continue;
        units.append(b.allocator, unit) catch @panic("OOM");
    }
    return units.items;
}

fn isReplaced(unit: []const u8) bool {
    for (replaced) |prefix| {
        if (std.mem.startsWith(u8, unit, prefix)) return true;
    }
    return false;
}

/// Every .cpp file under src/ (paths relative to it), except the entry point.
fn portSources(b: *std.Build) []const []const u8 {
    const io = b.graph.io;
    const root = b.root.join(b.allocator, "src") catch @panic("OOM");
    var dir = root.root_dir.handle.openDir(io, root.sub_path, .{ .iterate = true }) catch |err|
        std.debug.panic("cannot open src: {t}", .{err});
    defer dir.close(io);
    b.dependOnDirectoryContents(b.path("src"));

    var sources: std.ArrayList([]const u8) = .empty;
    var walker = dir.walk(b.allocator) catch @panic("OOM");
    defer walker.deinit();
    while (walker.next(io) catch |err| std.debug.panic("cannot list src: {t}", .{err})) |entry| {
        const path = b.dupe(entry.path);
        switch (entry.kind) {
            // Reconfigure when files are added to or removed from any directory.
            .directory => b.dependOnDirectoryContents(b.path(b.pathJoin(&.{ "src", path }))),
            .file => if (std.mem.endsWith(u8, path, ".cpp") and !std.mem.eql(u8, path, "main.cpp"))
                sources.append(b.allocator, path) catch @panic("OOM"),
            else => {},
        }
    }
    // Directory order is unspecified; sort so the configuration is stable.
    std.mem.sort([]const u8, sources.items, {}, lessThan);
    return sources.items;
}

fn lessThan(_: void, a: []const u8, b: []const u8) bool {
    return std.mem.lessThan(u8, a, b);
}
