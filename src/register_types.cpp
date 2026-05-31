#include "register_types.h"
#include "chess_engine.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_chess_module(ModuleInitializationLevel p_level) {
    // We only care about registering our game logic classes at the "SCENE" level
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Register your custom C++ class into Godot's engine database
    ClassDB::register_class<ChessEngine>();
}

void uninitialize_chess_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

// The master handshake function that Godot looks for when loading the DLL
extern "C" {
GDExtensionBool GDE_EXPORT chess_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_chess_module);
    init_obj.register_terminator(uninitialize_chess_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}