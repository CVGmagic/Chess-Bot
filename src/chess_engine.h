#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>
#include <intrin.h>
#include <cstdint>

// Compile with:
// scons platform=windows target=template_debug


namespace godot {

static const uint8_t castling_rights_update[64] = { // Allows fast catling rights updates
    13, 15, 15, 15, 12, 15, 15, 14,  // Row 1
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11   // Row 8
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
    EMPTY_PIECE,
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

enum Player { WHITE, BLACK, BOTH, NEITHER, USABLE};

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
    // Maps perfectly back to piece types: Knight (1), Bishop (2), Rook (3), Queen (4)
    inline int get_promo_piece_type() const {
        return (get_flag() & 0x3) + 1; 
    }

    inline bool is_double_pawn_push() const {return (data & 0xF000) == FLAG_DOUBLE_PAWN_PUSH << 12;}

    inline bool is_en_passant() const {return (data & 0xF000) == FLAG_EN_PASSANT << 12;}
};


struct Board {
    uint64_t bitboards[2][6] = {0};
    int side_to_move = 0; // 0 = White, 1 = Black
    int en_passant_square = -1; // -1 = None
    uint8_t castling_rights = 0b0000; // No castling allowed initially

    uint64_t occupancy[5] = {0}; // 0 = White, 1 = Black, 2 = Both, 3 = Empty, 4 = Friendly

    inline void set_bit(uint64_t &bitboard, int square) { bitboard |= (1ULL << square); }
    inline bool get_bit(uint64_t bitboard, int square) const { return (bitboard & (1ULL << square)) != 0; }
    inline void clear_bit(uint64_t &bitboard, int square) { bitboard &= ~(1ULL << square); }
    
    void clear() {
        for (int color = 0; color < 2; ++color) {
            for (int piece = 0; piece < 6; ++piece) {
                bitboards[color][piece] = 0ULL;
            }
        }

        side_to_move = 0;
        en_passant_square = -1;
        castling_rights = 0;
        for (int i = 0; i < 5; ++i) {
            occupancy[i] = 0ULL;
        }
    }

    void make_move(Move move) {
        int us = side_to_move;
        int them = us ^ 1;

        int from_sq = move.get_from();
        int to_sq = move.get_to();

        uint64_t from_mask = 1ULL << from_sq;
        uint64_t to_mask = 1ULL << to_sq;
        uint64_t move_mask = from_mask | to_mask;

        int piece = -1;
        for (int p = 0; p < 6; p++) {
            if (bitboards[us][p] & from_mask) {
                piece = p;
                break;
            }
        }

        if (piece == -1) {
            UtilityFunctions::push_error("Invalid piece type encountered when trying to make a move");
            return;
        }

        // Remove captured pieces 
        if (move.is_capture()) {
            for (int p = 0; p < 6; p++) {
                if (bitboards[them][p] & to_mask) {
                    bitboards[them][p] ^= to_mask;
                }
            }
        }

        // Remove moved piece from original location
        bitboards[us][piece] ^= from_mask;
        // Place piece at new location
        if (move.is_promotion()) {
            int promo_piece = move.get_promo_piece_type();
            bitboards[us][promo_piece] |= to_mask;
        } else {
            bitboards[us][piece] |= to_mask;
        }

        // Handle en passant
        if (move.is_en_passant()) {
            int ep_captured_sq = (us == WHITE) ? (to_sq - 8) : (to_sq + 8);
            uint64_t ep_mask = 1ULL << ep_captured_sq;
            bitboards[them][PAWN] ^= ep_mask;
        }

        // Handle double pawn push
        if (move.is_double_pawn_push()) {
            en_passant_square = (us == WHITE) ? (from_sq + 8) : (from_sq - 8);
        } else {
            en_passant_square = -1;
        }

        // Handle castling
        if (piece == KING && abs(to_sq - from_sq) == 2) {
        if (us == WHITE) {
            if (to_sq == G1)      { bitboards[WHITE][ROOK] ^= ((1ULL << H1) | (1ULL << F1)); } // Short
            else if (to_sq == C1) { bitboards[WHITE][ROOK] ^= ((1ULL << A1) | (1ULL << D1)); } // Long
        } else { // us == BLACK
            if (to_sq == G8)      { bitboards[BLACK][ROOK] ^= ((1ULL << H8) | (1ULL << F8)); } // Short
            else if (to_sq == C8) { bitboards[BLACK][ROOK] ^= ((1ULL << A8) | (1ULL << D8)); } // Long
        }
}
        // Update castling rights
        castling_rights &= castling_rights_update[from_sq];
        castling_rights &= castling_rights_update[to_sq];

        // Rebuild occupancy
        occupancy[WHITE] = 0ULL;
        occupancy[BLACK] = 0ULL;
        occupancy[BOTH] = 0ULL;
        occupancy[NEITHER] = 0ULL;
        occupancy[USABLE] = 0ULL;
        for (int p = 0; p < 6; p++) {
            occupancy[WHITE] |= bitboards[WHITE][p];
            occupancy[BLACK] |= bitboards[BLACK][p];
        }
        occupancy[BOTH] = occupancy[WHITE] | occupancy[BLACK];
        occupancy[NEITHER] = ~occupancy[BOTH];

        // Change current player
        side_to_move ^= 1;
        occupancy[USABLE] = ~occupancy[side_to_move];
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

    void generate_pseudo_legal_moves(const Board& board, std::vector<Move>& move_list);

    void generate_legal_moves(const Board& board, std::vector<Move>& move_list);

    bool is_square_attacked(const Board& board, int square, int enemy_color);

    bool is_in_check(const Board& board, int color);


    int32_t perft_rec(const Board& board, int32_t depth);


    Board board;

protected:
    static void _bind_methods(); // Exposes C++ functions to GDScript

public:
    ChessEngine();
    ~ChessEngine();


    void set_board_from_array(const PackedInt32Array& setup_board_array, int32_t side_to_move, int32_t castling_rights);

    int get_random_pseudo_legal_move();

    int get_random_legal_move();

    bool try_move(int32_t from_rank, int32_t from_file, int32_t to_rank, int32_t to_file, int32_t promo_choice);

    int32_t perft(int32_t depth) {return perft_rec(board, depth);};
};

}

#endif // CHESS_ENGINE_H