#!/usr/bin/env python
import os
import sys

# Ensure scons can find the godot-cpp build system
if not os.path.exists("godot-cpp"):
    print("Error: godot-cpp submodule not found.")
    sys.exit(1)

# Capture the fully configured environment from godot-cpp
env = SConscript("godot-cpp/SConstruct")

# Tell the compiler where your custom header files live
env.Append(CPPPATH=["src"])

# Gather all C++ source files inside your src directory
"""
sources = Glob("src/*.cpp")

# Exclude magic_number_generator from compilation
target_to_remove = os.path.normpath("src/magic_number_generator.cpp")
sources.remove(target_to_remove)
"""
sources = [
    "src/register_types.cpp",
    "src/chess_engine.cpp",
    "src/movegen.cpp"
]

# Construct the path using Godot's suffix AND SCons's native shared library extension (.dll)
target_path = "godot_project/bin/libchess" + env["suffix"] + env["SHLIBSUFFIX"]

# Compile your custom shared library
library = env.SharedLibrary(target_path, source=sources)

# Set it as the default build target
Default(library)