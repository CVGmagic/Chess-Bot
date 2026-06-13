#!/usr/bin/env python
import os
import sys

# Ensure scons can find the godot-cpp build system
if not os.path.exists("godot-cpp"):
    print("Error: godot-cpp submodule not found.")
    sys.exit(1)

# =============================================================================
# TARGET 1: GODOT GDEXTENSION (.DLL)
# =============================================================================

# Capture the fully configured environment from godot-cpp
env = SConscript("godot-cpp/SConstruct")

# Tell the compiler where your custom header files live
env.Append(CPPPATH=["src"])

# Gather source files specifically needed for the Godot GDExtension wrapper
godot_sources = [
    "src/godot/register_types.cpp",
    "src/core/chess_engine.cpp",
    "src/core/movegen.cpp"
]

# Construct the path using Godot's suffix AND SCons's native shared library extension (.dll)
target_path = "godot_project/bin/libchess" + env["suffix"] + env["SHLIBSUFFIX"]

# Compile your custom shared library
library = env.SharedLibrary(target_path, source=godot_sources)


# =============================================================================
# TARGET 2: STANDALONE UCI CLI ENGINE (.EXE)
# =============================================================================

# Create a brand new, clean compiler environment completely isolated from Godot
cli_env = Environment()

# Tell it where your custom header files live
cli_env.Append(CPPPATH=["src"])

# Inject high-performance compiler flags for maximizing search speed (NPS)
if sys.platform == "win32":
    cli_env.Append(CCFLAGS=["/O2", "/std:c++17", "/EHsc"])
else:
    cli_env.Append(CCFLAGS=["-O3", "-std=c++17"])

# Gather the files needed for the pure terminal version (Excluding Godot wrappers)
cli_sources = [
    "src/cli/uci_interface.cpp",
    "src/core/chess_engine.cpp",
    "src/core/movegen.cpp"
    # Add any extra logic cpp files like src/movegen.cpp here if needed
]

# Build the native console executable file inside a "bin" folder
cli_exe = cli_env.Program(target="bin/my_chess_engine", source=cli_sources)


# =============================================================================
# MASTER BUILD TRIGGER
# =============================================================================

# Tells SCons to build BOTH the Godot library and the standalone CLI tool simultaneously
Default([library, cli_exe])