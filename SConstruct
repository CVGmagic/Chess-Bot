#!/usr/bin/env python
import os
import sys

if not os.path.exists("godot-cpp"):
    print("Error: godot-cpp submodule not found.")
    sys.exit(1)

# =============================================================================
# TARGET 1: GODOT GDEXTENSION (.DLL / .WASM)
# =============================================================================

env = SConscript("godot-cpp/SConstruct")
env.Append(CPPPATH=["src"])

godot_sources = [
    "src/godot/register_types.cpp",
    "src/core/chess_engine.cpp",
    "src/core/movegen.cpp"
]

target_path = "godot_project/bin/libchess" + env["suffix"] + env["SHLIBSUFFIX"]
library = env.SharedLibrary(target_path, source=godot_sources)


# =============================================================================
# TARGET 2: STANDALONE UCI CLI / WASM ENGINE
# =============================================================================

platform = ARGUMENTS.get("platform", sys.platform)

if platform == "web":
    # Force GCC-style argument syntax for Emscripten
    cli_env = Environment(tools=['gcc', 'g++', 'link'], ENV=os.environ.copy())
    cli_env.Append(CPPPATH=["src"])
    
    cli_env.Replace(
        CC="emcc",
        CXX="em++",
        LINK="em++",
        PROGSUFFIX=".js"
    )
    
    cli_env.Append(CCFLAGS=["-O3", "-std=c++20"])
    cli_env.Append(LINKFLAGS=[
        "-O3",
        "-s", "WASM=1",
        "-s", "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
        "-s", "INVOKE_RUN=1",
        "-s", "EXIT_RUNTIME=0"
    ])
    
    cli_target_name = "bin/my_chess_engine_wasm"

else:
    # Native compilation setup
    cli_env = Environment(ENV=os.environ.copy())
    cli_env.Append(CPPPATH=["src"])
    
    if sys.platform == "win32":
        cli_env.Append(CCFLAGS=["/O2", "/std:c++20", "/EHsc"])
    else:
        cli_env.Append(CCFLAGS=["-O3", "-std=c++20"])
        
    cli_target_name = "bin/my_chess_engine"

# Redirect CLI build artifacts to a separate build folder to prevent collisions
cli_env.VariantDir("build/cli", "src", duplicate=0)

cli_sources = [
    "build/cli/cli/uci_interface.cpp",
    "build/cli/core/chess_engine.cpp",
    "build/cli/core/movegen.cpp"
]

cli_exe = cli_env.Program(target=cli_target_name, source=cli_sources)


# =============================================================================
# MASTER BUILD TRIGGER
# =============================================================================

Default([library, cli_exe])