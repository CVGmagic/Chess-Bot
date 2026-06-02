#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <vector>
#include <intrin.h>
#include <cstdint>

// Compile with
// scons platform=windows target=template_debug

namespace godot {

struct Board {
    uint64_t bitboards[2][6] = {0};
    int side_to_move = 0;          // 0 = White, 1 = Black
    int en_passant_square = -1;    // -1 = None
    uint8_t castling_rights = 0;   

    inline void set_bit(uint64_t &bitboard, int square) { bitboard |= (1ULL << square); }
    inline bool get_bit(uint64_t bitboard, int square) const { return (bitboard & (1ULL << square)) != 0; }
    inline void clear_bit(uint64_t &bitboard, int square) { bitboard &= ~(1ULL << square); }
    
    // Updates the composite white, black, and total occupancy bitboards
};


enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

enum GodotPieces{
    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING,
    BLACK_PAWN,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING
};

enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };

enum Player { WHITE, BLACK };

enum MoveFlag {
    // Quiet Moves
    FLAG_QUIET              = 0x0, // 0000
    FLAG_DOUBLE_PAWN_PUSH   = 0x1, // 0001
    FLAG_KING_CASTLE        = 0x2, // 0010
    FLAG_QUEEN_CASTLE       = 0x3, // 0011

    // Captures
    FLAG_CAPTURE            = 0x4, // 0100
    FLAG_EN_PASSANT         = 0x5, // 0101

    // Quiet Promotions
    FLAG_PROMO_KNIGHT       = 0x8, // 1000
    FLAG_PROMO_BISHOP       = 0x9, // 1001
    FLAG_PROMO_ROOK         = 0xA, // 1010
    FLAG_PROMO_QUEEN        = 0xB, // 1011

    // Capturing Promotions
    FLAG_PROMO_CAPTURE_N    = 0xC, // 1100
    FLAG_PROMO_CAPTURE_B    = 0xD, // 1101
    FLAG_PROMO_CAPTURE_R    = 0xE, // 1110
    FLAG_PROMO_CAPTURE_Q    = 0xF  // 1111
};


struct Move {
    uint16_t data = 0;

    Move() : data(0) {}
    
    // Constructor maps: from (6 bits) | to (6 bits) | flag (4 bits)
    Move(int from, int to, MoveFlag flag = FLAG_QUIET) {
        data = from | (to << 6) | (flag << 12);
    }

    // Basic Getters
    inline int get_from() const { return data & 0x3F; }        // Mask: 00111111
    inline int get_to()   const { return (data >> 6) & 0x3F; } // Mask: 00111111
    inline int get_flag() const { return (data >> 12) & 0x0F; }// Mask: 00001111

    // State Checks
    inline bool is_none() const { return data == 0; }

    // If the 3rd bit of the flag is 1 (value 4), it's a capture scenario
    inline bool is_capture() const { 
        return (get_flag() & 0x4) != 0; 
    }

    // If the 4th bit of the flag is 1 (value 8), it's a promotion scenario
    inline bool is_promotion() const { 
        return (get_flag() & 0x8) != 0; 
    }

    // Helper to get the piece type for promotions if is_promotion() is true.
    // Maps perfectly back to piece types: Knight (0), Bishop (1), Rook (2), Queen (3)
    inline int get_promo_piece_type() const {
        return get_flag() & 0x3; 
    }
};


class ChessEngine : public RefCounted {
    GDCLASS(ChessEngine, RefCounted); // Hooks this class into Godot's type system

private:

    static const uint64_t KNIGHT_ATTACKS[64];

    inline int pop_lsb(uint64_t& bitboard) {
        #if defined(_MSC_VER)
        unsigned long index;
        _BitScanForward64(&index, bitboard);
        bitboard &= bitboard - 1; // Magic trick to clear the lowest set bit
        return index;
        #else
        int index = __builtin_ctzll(bitboard);
        bitboard &= bitboard - 1;
        return index;
        #endif
    }

    void generate_pseudo_legal_moves(std::vector<Move>& move_list);

    Board board;
protected:
    static void _bind_methods(); // Exposes C++ functions to GDScript

public:
    ChessEngine();
    ~ChessEngine();

    int get_test_value() const;

    void set_board_from_array(const PackedInt32Array &setup_board_array);

    int get_random_legal_move();

    
};

}

#endif // CHESS_ENGINE_H