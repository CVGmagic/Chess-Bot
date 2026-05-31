#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class ChessEngine : public RefCounted {
    GDCLASS(ChessEngine, RefCounted); // Hooks this class into Godot's type system

private:
    uint64_t dummy_board_state; // This will hold your future 64-bit bitboards!

protected:
    static void _bind_methods(); // Exposes C++ functions to GDScript

public:
    ChessEngine();
    ~ChessEngine();

    int get_test_value() const;
};

} // namespace godot

#endif // CHESS_ENGINE_H