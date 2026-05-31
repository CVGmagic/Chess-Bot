#include "chess_engine.h"
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// 1. MUST have ChessEngine:: before the constructor name!
ChessEngine::ChessEngine() {
    dummy_board_state = 12345; 
}

// 2. MUST have ChessEngine:: before the destructor name!
ChessEngine::~ChessEngine() {
}

// 3. MUST have ChessEngine:: right here!
void ChessEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_test_value"), &ChessEngine::get_test_value);
}

// 4. MUST have ChessEngine:: right here too!
int ChessEngine::get_test_value() const {
    return dummy_board_state;
}

} // namespace godot