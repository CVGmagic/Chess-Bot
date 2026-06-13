#ifndef GODOT_ENGINE_WRAPPER_H
#define GODOT_ENGINE_WRAPPER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "../src/core/chess_engine.h"

namespace godot {

class GodotChessEngineV1 : public RefCounted {
    GDCLASS(GodotChessEngineV1, RefCounted);

private:
    ChessCore::ChessEngine native_engine;

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("make_best_move", "depth"), &GodotChessEngineV1::make_best_move);
        ClassDB::bind_method(D_METHOD("try_move", "from_rank", "from_file", "to_rank", "to_file", "promo_choice"), &GodotChessEngineV1::try_move);
        ClassDB::bind_method(D_METHOD("set_board_from_array", "setup_board_array", "side_to_move", "castling_rights"), &GodotChessEngineV1::set_board_from_array);
    }

public:
    GodotChessEngineV1() {}
    ~GodotChessEngineV1() {}

    int make_best_move(int depth) {
        // Call the decoupled C++ search algorithm directly
        ChessCore::Move best = native_engine.make_best_move(depth);
        
        // Pass the move back out to your GDScript visual manager
        return best.data; 
    }


    int try_move(int32_t from_rank, int32_t from_file, int32_t to_rank, int32_t to_file, int32_t promo_choice) {
        return native_engine.try_move(from_rank, from_file, to_rank, to_file, promo_choice);
    }


    void set_board_from_array(const PackedInt32Array& setup_board_array, int32_t side_to_move, int32_t castling_rights) {
        if (setup_board_array.size() != 64) {
            UtilityFunctions::printerr("ChessEngine Error: Board array must contain exactly 64 elements!");
            return;
        }

        const int32_t* raw_ptr = setup_board_array.ptr();
        native_engine.set_board_from_array(raw_ptr, side_to_move, castling_rights);
    }
};

}

#endif