#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <vector>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#include <cstdint>
#include <iostream>
#include <cmath>


// Compile with:
// scons platform=windows target=template_release


namespace ChessCore {

static const uint8_t castling_rights_update[64] = { // Allows fast castling rights updates
    13, 15, 15, 15, 12, 15, 15, 14,  // Row 1
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11   // Row 8
};

static const int piece_values[6] = {100, 300, 320, 500, 900, 10000};

static const int mg_table[6][64] = {
    // [0] PAWN
    {
          0,   0,   0,   0,   0,   0,   0,   0,  // Rank 1 (a1 - h1)
        -35,  -1, -20, -23, -15,  24,  38, -22,  // Rank 2
        -26,  -4,  -4, -10,   3,   3,  33, -12,  // Rank 3
        -27,  -2,  -5,  12,  17,   6,  10, -25,  // Rank 4
        -14,  13,   6,  21,  23,  12,  17, -23,  // Rank 5
         -6,   7,  26,  31,  65,  56,  25, -20,  // Rank 6
         98, 134,  61,  95,  68, 126,  34, -11,  // Rank 7
          0,   0,   0,   0,   0,   0,   0,   0   // Rank 8 (a8 - h8)
    },
    // [1] KNIGHT
    {
        -105, -21, -58, -33, -17, -28, -19, -23,
         -29, -53, -12,  -3,  -1,  18, -14, -19,
         -23,  -9,  12,  10,  19,  17,  25, -16,
         -13,   4,  16,  13,  28,  19,  21,  -8,
          -9,  17,  19,  53,  37,  69,  18,  22,
         -47,  60,  37,  65,  84, 129,  73,  44,
         -73, -41,  72,  36,  23,  62,   7, -17,
        -167, -89, -34, -49,  61, -97, -15, -107
    },
    // [2] BISHOP
    {
        -33,  -3, -14, -21, -13, -12, -39, -21,
          4,  15,  16,   0,   7,  21,  33,   1,
          0,  15,  15,  15,  14,  27,  18,  10,
         -6,  13,  13,  26,  34,  12,  10,   4,
         -4,   5,  19,  50,  37,  37,   7,  -2,
        -16,  37,  43,  40,  35,  50,  37,  -2,
        -26,  16, -18, -13,  30,  59,  18, -47,
        -29,   4, -82, -37, -25, -42,   7,  -8
    },
    // [3] ROOK
    {
        -19, -13,   1,  17, 16,   7, -37, -26,
        -44, -16, -20,  -9,  -1,  11,  -6, -71,
        -45, -25, -16, -17,   3,   0,  -5, -33,
        -36, -26, -12,  -1,   9,  -7,   6, -23,
        -24, -11,   7,  26, 24,  35,  -8, -20,
         -5,  19,  26,  36, 17,  45,  61,  16,
         27,  32,  58,  62, 80,  67,  26,  44,
         32,  42,  32,  51, 63,   9,  31,  43
    },
    // [4] QUEEN
    {
         -1, -18,  -9,  10, -15, -25, -31, -50,
        -35,  -8,  11,   2,   8,  15,  -3,   1,
        -14,   2, -11,  -2,  -5,   2,  14,   5,
         -9, -26,  -9, -10,  -2,  -4,   3,  -3,
        -27, -27, -16, -16,  -1,  17,  -2,   1,
        -13, -17,   7,   8,  29,  56,  47,  57,
        -24, -39,  -5,   1, -16,  57,  28,  54,
        -28,   0,  29,  12,  59,  44,  43,  45
    },
    // [5] KING
    {
        -15,  36,  12, -54,   8, -28,  24,  14,
          1,   7,  -8, -64, -43, -16,   9,   8,
        -14, -14, -22, -46, -44, -30, -15, -27,
        -49,  -1, -27, -39, -46, -44, -33, -51,
        -17, -20, -12, -27, -30, -25, -14, -36,
         -9,  24,   2, -16, -20,   6,  22, -22,
         29,  -1, -20,  -7,  -8,  -4, -38, -29,
        -65,  23,  16, -15, -56, -34,   2,  13
    }
};


const int eg_table[6][64] = {
    // [0] PAWN
    {
          0,   0,   0,   0,   0,   0,   0,   0,  // Rank 1 (a1 - h1)
         13,   8,   8,  10,  13,   0,   2,  -7,  // Rank 2
          4,   7,  -6,   1,   0,  -5,  -1,  -8,  // Rank 3
         13,   9,  -3,  -7,  -7,  -8,   3,  -1,  // Rank 4
         32,  24,  13,   5,  -2,   4,  17,  17,  // Rank 5
         94, 100,  85,  67,  56,  53,  82,  84,  // Rank 6
        178, 173, 158, 134, 147, 132, 165, 187,  // Rank 7
          0,   0,   0,   0,   0,   0,   0,   0   // Rank 8 (a8 - h8)
    },
    // [1] KNIGHT
    {
        -29, -51, -23, -15, -22, -18, -50, -64,
        -42, -20, -10,  -5,  -2, -20, -23, -44,
        -23,  -3,  -1,  15,  10,  -3, -20, -22,
        -18,  -6,  16,  25,  16,  17,   4, -18,
        -17,   3,  22,  22,  22,  11,   8, -18,
        -24, -20,  10,   9,  -1,  -9, -19, -41,
        -25,  -8, -25,  -2,  -9, -25, -24, -52,
        -58, -38, -13, -28, -31, -27, -63, -99
    },
    // [2] BISHOP
    {
        -23,  -9, -23,  -5,  -9, -16,  -5, -17,
        -14, -18,  -7,  -1,   4,  -9, -15, -27,
        -12,  -3,   8,  10,  13,   3,  -7, -15,
         -6,   3,  13,  19,   7,  10,  -3,  -9,
         -3,   9,  12,   9,  14,  10,   3,   2,
          2,  -8,   0,  -1,  -2,   6,   0,   4,
         -8,  -4,   7, -12,  -3, -13,  -4, -14,
        -14, -21, -11,  -8,  -7,  -9, -17, -24
    },
    // [3] ROOK
    {
         -9,   2,   3,  -1,  -5, -13,   4, -20,
         -6, -6,   0,   2,  -9,  -9, -11,  -3,
         -4,   0,  -5,  -1,  -7, -12,  -8, -16,
          3,   5,   8,   4,  -5,  -6,  -8, -11,
          4,   3,  13,   1,   2,   1,  -1,   2,
          7,   7,   7,   5,   4,  -3,  -5,  -3,
         11, 13, 13, 11,  -3,   3,   8,   3,
         13, 10, 18, 15, 12,  12,   8,   5
    },
    // [4] QUEEN
    {
        -33, -28, -22, -43,  -5, -32, -20, -41,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -16, -27,  15,   6,   9,  17,  10,   5,
        -18,  28,  19,  47,  31,  34,  39,  23,
          3,  22,  24,  45,  57,  40,  57,  36,
        -20,   6,   9,  49,  47,  35,  19,   9,
        -17,  20,  32,  41,  58,  25,  30,   0,
         -9,  22,  22,  27,  27,  19,  10,  20
    },
    // [5] KING
    {
        -53, -34, -21, -11, -28, -14, -24, -43,
        -27, -11,   4,  13,  14,   4,  -5, -17,
        -19,  -3,  11,  21,  23,  16,   7,  -9,
        -18,  -4,  21,  24,  27,  23,   9, -11,
         -8,  22,  24,  27,  26,  33,  26,   3,
         10,  17,  23,  15,  20,  45,  44,  13,
        -12,  17,  14,  17,  17,  38,  23,  11,
        -74, -35, -18, -18, -11,  15,   4, -17
    }
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

const int INF = 30000;

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
    // TODO Can probably be optimized by removing the &

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

    int material = 0;

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
            std::cerr << "Invalid piece type encountered when trying to make a move";
            return;
        }

        // Remove captured pieces 
        if (move.is_capture()) {
            for (int p = 0; p < 6; p++) {
                if (bitboards[them][p] & to_mask) {
                    bitboards[them][p] ^= to_mask;
                    // Remove the piece value from black's perspective
                    (us == WHITE) ? material += (piece_values[p] + mg_table[p][to_sq ^ 56]) : material -= (piece_values[p] + mg_table[p][to_sq]);
                }
            }
        }

        // Remove moved piece from original location
        bitboards[us][piece] ^= from_mask;
        // Place piece at new location
        if (move.is_promotion()) {
            int promo_piece = move.get_promo_piece_type();
            bitboards[us][promo_piece] |= to_mask;
            (us == WHITE) ? material += (-piece_values[piece] + piece_values[promo_piece] - mg_table[piece][from_sq] + mg_table[promo_piece][to_sq]) : material -= (-piece_values[piece] + piece_values[promo_piece] - mg_table[piece][from_sq ^ 56]+ mg_table[promo_piece][to_sq ^ 56]);
        } else {
            bitboards[us][piece] |= to_mask;
            (us == WHITE) ? material += (- mg_table[piece][from_sq] + mg_table[piece][to_sq]) : material -= (- mg_table[piece][from_sq ^ 56]+ mg_table[piece][to_sq ^ 56]);
        }

        // Handle en passant
        uint64_t ep_mask = 0ULL;
        if (move.is_en_passant()) {
            int ep_captured_sq = (us == WHITE) ? (to_sq - 8) : (to_sq + 8);
            ep_mask = 1ULL << ep_captured_sq;
            bitboards[them][PAWN] ^= ep_mask;
            (us == WHITE) ? material += (piece_values[PAWN] + mg_table[PAWN][(to_sq - 8) ^ 56]) : material -= (piece_values[PAWN] + mg_table[PAWN][to_sq + 8]);
        }

        // Handle double pawn push en passant
        if (move.is_double_pawn_push()) {
            en_passant_square = (us == WHITE) ? (from_sq + 8) : (from_sq - 8);
        } else {
            en_passant_square = -1;
        }

        // Handle castling
        bool is_castle = (piece == KING) && (abs(to_sq - from_sq) == 2);
        uint64_t rook_move_mask = 0ULL;
        if (is_castle) {
        if (us == WHITE) {
            if (to_sq == G1) { // Short
                rook_move_mask = ((1ULL << H1) | (1ULL << F1));
                bitboards[WHITE][ROOK] ^= rook_move_mask;
                material += (-mg_table[ROOK][H1] + mg_table[ROOK][F1]);
                
            }
            else if (to_sq == C1) { // Long
                rook_move_mask = ((1ULL << A1) | (1ULL << D1));
                bitboards[WHITE][ROOK] ^= rook_move_mask;
                material += (-mg_table[ROOK][A1] + mg_table[ROOK][D1]);
            }
        } else { // us == BLACK
            if (to_sq == G8) { // Short
                rook_move_mask = ((1ULL << H8) | (1ULL << F8));
                bitboards[BLACK][ROOK] ^= rook_move_mask;
                material -= (-mg_table[ROOK][H1] + mg_table[ROOK][F1]); // Use H1 and F1 because it's black's perspective
            }
            else if (to_sq == C8) { // Long
                rook_move_mask = ((1ULL << A8) | (1ULL << D8));
                bitboards[BLACK][ROOK] ^= rook_move_mask;
                material -= (-mg_table[ROOK][A1] + mg_table[ROOK][D1]);
            }
        }
}
        // Update castling rights
        castling_rights &= castling_rights_update[from_sq];
        castling_rights &= castling_rights_update[to_sq];

        // Rebuild occupancy
        occupancy[us] ^= (is_castle) ? (move_mask | rook_move_mask) : move_mask;

        if (move.is_capture()) {
            occupancy[them] ^= (move.is_en_passant()) ? ep_mask : to_mask;
        }

        occupancy[BOTH] = occupancy[WHITE] | occupancy[BLACK];
        occupancy[NEITHER] = ~occupancy[BOTH];

        side_to_move ^= 1;

        occupancy[USABLE] = ~occupancy[side_to_move];
    }
};


class ChessEngine {
private:
    static const uint64_t KNIGHT_ATTACKS[64];

    uint64_t between_matrix[64][64];

    void init_between_matrix() {
        // Clear the entire matrix
        for (int f = 0; f < 64; f++) {
            for (int t = 0; t < 64; t++) {
                between_matrix[f][t] = 0ULL;
            }
        }

        // Loop through every combination of "from" and "to" squares
        for (int f = 0; f < 64; f++) {
            for (int t = 0; t < 64; t++) {
                // Break the 0-63 indices down into 0-7 coordinates
                int f_rank = f / 8, f_file = f % 8;
                int t_rank = t / 8, t_file = t % 8;

                int rank_diff = t_rank - f_rank;
                int file_diff = t_file - f_file;

                // Determine alignment
                bool same_rank = (f_rank == t_rank);
                bool same_file = (f_file == t_file);
                bool same_diag = (std::abs(rank_diff) == std::abs(file_diff));

                // If they don't align perfectly, they don't share a ray. Keep it 0.
                if (!same_rank && !same_file && !same_diag) {
                    continue; 
                }

                // Calculate the directional step size
                // If delta is positive, step +1. If negative, step -1. If zero, step 0.
                int step_rank = (rank_diff == 0) ? 0 : (rank_diff > 0 ? 1 : -1);
                int step_file = (file_diff == 0) ? 0 : (file_diff > 0 ? 1 : -1);
                
                // Map the 2D direction back into a flat 1D array index step
                // e.g., North is +8, East is +1, Northeast is +9
                int step = (step_rank * 8) + step_file;

                // Walk from 'f' to 't' (exclusive) and build the bitmask
                uint64_t mask = 0ULL;
                int current = f + step;
                
                while (current != t) {
                    mask |= (1ULL << current);
                    current += step;
                }

                // Store the completed ray in your lookup table
                between_matrix[f][t] = mask;
            }
        }
    }

    inline int pop_lsb(uint64_t& bitboard) {
        if (bitboard == 0) {return -1;}

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

    uint64_t get_absolute_pins(const Board& board, int king_sq);

    int32_t perft_rec(const Board& board, int32_t depth);

    Move get_opponent_move(int from_sq, int to_sq, int promo_choice);

    int evaluate(const Board& board);

    std::pair<int, Move> minmax(const Board& board, int depth);

    std::pair<int, Move> search(const Board& board, int depth, int alpha, int beta);

    Board board;

public:
    ChessEngine();
    ~ChessEngine();

    void set_board_to_startpos();

    void set_board_from_array(const int32_t* setup_board_array, int32_t side_to_move, int32_t castling_rights);

    int get_random_pseudo_legal_move();

    int get_random_legal_move();

    int try_move(int32_t from_rank, int32_t from_file, int32_t to_rank, int32_t to_file, int32_t promo_choice);

    void make_opponent_move(int from_sq, int to_sq, int promo_choice);

    int32_t perft(int32_t depth) {return perft_rec(board, depth);};

    Move make_best_move(int depth);
};

}

#endif // CHESS_ENGINE_H