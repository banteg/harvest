//! Builds the modern port of Harvest from the recovered source in ../src.
//!
//! The kept units are every unit in config/1.18-linux-amd64/units.toml except the platform units the
//! port replaces (`replaced` below), so newly recovered units join the build without edits here.

const std = @import("std");

const units_toml = "../config/1.18-linux-amd64/units.toml";

/// Recovered units the port replaces with its own implementations, as path prefixes under src/.
const replaced = [_][]const u8{
    // Linux device (X11/SFML window and events), the device stub, OS operator (GTK clipboard),
    // logger and os helpers (printer, timer): the SDL3 device replaces them.
    "daisy/other/",
    // Linux joystick driver and the null driver: SDL3 gamepads replace them.
    "daisy/input/",
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

/// Flags for the recovered C++. GCC 4.4 defaulted to gnu++98, which needs no source changes.
const cxx_flags = [_][]const u8{
    "-std=gnu++98",
    "-DHARVEST_PORT",
    // sprintf and friends in the original source, and string literals bound to char*.
    "-Wno-deprecated-declarations",
    "-Wno-c++11-compat-deprecated-writable-strings",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const units = keptUnits(b);

    const harvest = b.addLibrary(.{
        .name = "harvest",
        .root_module = recoveredModule(b, target, optimize, units),
    });
    b.installArtifact(harvest);

    // The census links every kept object (not only those a main pulls from the archive) against a
    // stub entry point, so the linker lists every symbol the port still has to provide.
    const census_module = recoveredModule(b, target, optimize, units);
    census_module.addCSourceFile(.{ .file = b.path("census/main.cpp"), .flags = &cxx_flags });
    const census = b.addExecutable(.{ .name = "harvest-census", .root_module = census_module });
    b.step("census", "Link every kept unit and report unresolved symbols").dependOn(&b.addInstallArtifact(census, .{}).step);
}

fn recoveredModule(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    units: []const []const u8,
) *std.Build.Module {
    const module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });
    module.addIncludePath(b.path("../src"));
    module.addIncludePath(b.path("../src/HarvestFull"));
    // Lua 5.1 API headers; PUC Lua 5.1.5 replaces LuaJIT when it is vendored.
    module.addIncludePath(b.path("../third_party/luajit-2.0.0-beta8/include"));
    module.addCSourceFiles(.{ .root = b.path("../src"), .files = units, .flags = &cxx_flags });
    return module;
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
