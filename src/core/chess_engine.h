#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <vector>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#include <cstdint>
#include <iostream>
#include <cmath>
#include <random>
#include <chrono>


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

static const int mg_table_pure[6][64] = {
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

static const int eg_table_pure[6][64] = {
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

static const int mg_table[6][64] = {
    // PAWN [0]
    {
        100, 100, 100, 100, 100, 100, 100, 100,
        65, 99, 80, 77, 85, 124, 138, 78,
        74, 96, 96, 90, 103, 103, 133, 88,
        73, 98, 95, 112, 117, 106, 110, 75,
        86, 113, 106, 121, 123, 112, 117, 77,
        94, 107, 126, 131, 165, 156, 125, 80,
        198, 234, 161, 195, 168, 226, 134, 89,
        100, 100, 100, 100, 100, 100, 100, 100
    },
    // KNIGHT [1]
    {
        195, 279, 242, 267, 283, 272, 281, 277,
        271, 247, 288, 297, 299, 318, 286, 281,
        277, 291, 312, 310, 319, 317, 325, 284,
        287, 304, 316, 313, 328, 319, 321, 292,
        291, 317, 319, 353, 337, 369, 318, 322,
        253, 360, 337, 365, 384, 429, 373, 344,
        227, 259, 372, 336, 323, 362, 307, 283,
        133, 211, 266, 251, 361, 203, 285, 193
    },
    // BISHOP [2]
    {
        287, 317, 306, 299, 307, 308, 281, 299,
        324, 335, 336, 320, 327, 341, 353, 321,
        320, 335, 335, 335, 334, 347, 338, 330,
        314, 333, 333, 346, 354, 332, 330, 324,
        316, 325, 339, 370, 357, 357, 327, 318,
        304, 357, 363, 360, 355, 370, 357, 318,
        294, 336, 302, 307, 350, 379, 338, 273,
        291, 324, 238, 283, 295, 278, 327, 312
    },
    // ROOK[3]
    {
        481, 487, 501, 517, 516, 507, 463, 474,
        456, 484, 480, 491, 499, 511, 494, 429,
        455, 475, 484, 483, 503, 500, 495, 467,
        464, 474, 488, 499, 509, 493, 506, 477,
        476, 489, 507, 526, 524, 535, 492, 480,
        495, 519, 526, 536, 517, 545, 561, 516,
        527, 532, 558, 562, 580, 567, 526, 544,
        532, 542, 532, 551, 563, 509, 531, 543
    },
    //QUEEN [4]
    {
        899, 882, 891, 910, 885, 875, 869, 850,
        865, 892, 911, 902, 908, 915, 897, 901,
        886, 902, 889, 898, 895, 902, 914, 905,
        891, 874, 891, 890, 898, 896, 903, 897,
        873, 873, 884, 884, 899, 917, 898, 901,
        887, 883, 907, 908, 929, 956, 947, 957,
        876, 861, 895, 901, 884, 957, 928, 954,
        872, 900, 929, 912, 959, 944, 943, 945
    },
    // KING [5]
    {
        9985, 10036, 10012, 9946, 10008, 9972, 10024, 10014,
        10001, 10007, 9992, 9936, 9957, 9984, 10009, 10008,
        9986, 9986, 9978, 9954, 9956, 9970, 9985, 9973,
        9951, 9999, 9973, 9961, 9954, 9956, 9967, 9949,
        9983, 9980, 9988, 9973, 9970, 9975, 9986, 9964,
        9991, 10024, 10002, 9984, 9980, 10006, 10022, 9978,
        10029, 9999, 9980, 9993, 9992, 9996, 9962, 9971,
        9935, 10023, 10016, 9985, 9944, 9966, 10002, 10013
    }
};

static const int eg_table[6][64] = {
    // PAWN [0]
    {
        100, 100, 100, 100, 100, 100, 100, 100,
        113, 108, 108, 110, 113, 100, 102, 93,
        104, 107, 94, 101, 100, 95, 99, 92,
        113, 109, 97, 93, 93, 92, 103, 99,
        132, 124, 113, 105, 98, 104, 117, 117,
        194, 200, 185, 167, 156, 153, 182, 184,
        278, 273, 258, 234, 247, 232, 265, 287,
        100, 100, 100, 100, 100, 100, 100, 100
    },
    // KNIGHT [1]
    {
        271, 249, 277, 285, 278, 282, 250, 236,
        258, 280, 290, 295, 298, 280, 277, 256,
        277, 297, 299, 315, 310, 297, 280, 278,
        282, 294, 316, 325, 316, 317, 304, 282,
        283, 303, 322, 322, 322, 311, 308, 282,
        276, 280, 310, 309, 299, 291, 281, 259,
        275, 292, 275, 298, 291, 275, 276, 248,
        242, 262, 287, 272, 269, 273, 237, 201
    },
    // BISHOP [2]
    {
        297, 311, 297, 315, 311, 304, 315, 303,
        306, 302, 313, 319, 324, 311, 305, 293,
        308, 317, 328, 330, 333, 323, 313, 305,
        314, 323, 333, 339, 327, 330, 317, 311,
        317, 329, 332, 329, 334, 330, 323, 322,
        322, 312, 320, 319, 318, 326, 320, 324,
        312, 316, 327, 308, 317, 307, 316, 306,
        306, 299, 309, 312, 313, 311, 303, 296
    },
    // ROOK [3]
    {
        491, 502, 503, 499, 495, 487, 504, 480,
        494, 494, 500, 502, 491, 491, 489, 497,
        496, 500, 495, 499, 493, 488, 492, 484,
        503, 505, 508, 504, 495, 494, 492, 489,
        504, 503, 513, 501, 502, 501, 499, 502,
        507, 507, 507, 505, 504, 497, 495, 497,
        511, 513, 513, 511, 497, 503, 508, 503,
        513, 510, 518, 515, 512, 512, 508, 505
    },
    // QUEEN [4]
    {
        867, 872, 878, 857, 895, 868, 880, 859,
        878, 877, 870, 884, 884, 877, 864, 868,
        884, 873, 915, 906, 909, 917, 910, 905,
        882, 928, 919, 947, 931, 934, 939, 923,
        903, 922, 924, 945, 957, 940, 957, 936,
        880, 906, 909, 949, 947, 935, 919, 909,
        883, 920, 932, 941, 958, 925, 930, 900,
        891, 922, 922, 927, 927, 919, 910, 920
    },
    // KING [5]
    {
        9947, 9966, 9979, 9989, 9972, 9986, 9976, 9957,
        9973, 9989, 10004, 10013, 10014, 10004, 9995, 9983,
        9981, 9997, 10011, 10021, 10023, 10016, 10007, 9991,
        9982, 9996, 10021, 10024, 10027, 10023, 10009, 9989,
        9992, 10022, 10024, 10027, 10026, 10033, 10026, 10003,
        10010, 10017, 10023, 10015, 10020, 10045, 10044, 10013,
        9988, 10017, 10014, 10017, 10017, 10038, 10023, 10011,
        9926, 9965, 9982, 9982, 9989, 10015, 10004, 9983
    }
};

static const int PHASE_VALUES[6] = {0, 1, 1, 2, 4, 0};

static const int SORT_VALUES[6] = { 100, 200, 300, 400, 500, 600 };

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


class Zobrist {
public:
    // The random number arrays
    inline static uint64_t pieces[2][6][64];
    inline static uint64_t side_to_move;
    inline static uint64_t castling[16];
    inline static uint64_t en_passant[8];

    // Call this once at engine startup
    static void initialize() {
        // Use a 64-bit Mersenne Twister engine with a fixed seed for consistency
        std::mt19937_64 rng(1070372); // Any fixed seed works

        for (int color = 0; color < 2; color++) {
            for (int piece = 0; piece < 6; piece++) {
                for (int sq = 0; sq < 64; sq++) {
                    pieces[color][piece][sq] = rng();
                }
            }
        }

        side_to_move = rng();

        for (int i = 0; i < 16; i++) {
            castling[i] = rng();
        }

        for (int i = 0; i < 8; i++) {
            en_passant[i] = rng();
        }
    }
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
    // TODO Maybe optimize by swapping some datatypes for faster copy
    uint64_t bitboards[2][6] = {0};
    int side_to_move = 0; // 0 = White, 1 = Black
    int en_passant_square = -1; // -1 = None
    uint8_t castling_rights = 0b0000; // No castling allowed initially

    uint64_t occupancy[5] = {0}; // 0 = White, 1 = Black, 2 = Both, 3 = Empty, 4 = Friendly

    int mg_score = 0;
    int eg_score = 0;

    int game_phase = 0; // Goes from 0 (no pieces) to 24 (all pieces)

    uint64_t zobrist_hash = 0;

    int halfmove_clock = 0;
    // Stores only hashes since last reversible move
    std::vector<uint64_t> reversible_history;

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

        // This works because a ^ b ^ b = a
        zobrist_hash ^= Zobrist::pieces[us][piece][from_sq];
        // New piece is added later because of possible promotion

        zobrist_hash ^= Zobrist::side_to_move;

        // Remove captured pieces 
        if (move.is_capture()) {
            for (int p = 0; p < 6; p++) {
                if (bitboards[them][p] & to_mask) {
                    bitboards[them][p] ^= to_mask;
                    // Remove the piece value from black's perspective
                    mg_score += (us == WHITE) ? (mg_table[p][to_sq ^ 56]) : -(mg_table[p][to_sq]);
                    eg_score += (us == WHITE) ? (eg_table[p][to_sq ^ 56]) : -(eg_table[p][to_sq]);
                    game_phase -= PHASE_VALUES[p];
                    zobrist_hash ^= Zobrist::pieces[them][p][to_sq];
                }
            }
        }

        // Remove moved piece from original location
        bitboards[us][piece] ^= from_mask;
        // Place piece at new location
        if (move.is_promotion()) {
            int promo_piece = move.get_promo_piece_type();
            bitboards[us][promo_piece] |= to_mask;
            mg_score += (us == WHITE) ? (-mg_table[PAWN][from_sq] + mg_table[promo_piece][to_sq]) : -(-mg_table[PAWN][from_sq ^ 56] + mg_table[promo_piece][to_sq ^ 56]);
            eg_score += (us == WHITE) ? (-eg_table[PAWN][from_sq] + eg_table[promo_piece][to_sq]) : -(-eg_table[PAWN][from_sq ^ 56] + eg_table[promo_piece][to_sq ^ 56]);
            game_phase += (-PHASE_VALUES[PAWN] + PHASE_VALUES[promo_piece]);
            zobrist_hash ^= Zobrist::pieces[us][promo_piece][to_sq];
        } else {
            bitboards[us][piece] |= to_mask;
            mg_score += (us == WHITE) ? (-mg_table[piece][from_sq] + mg_table[piece][to_sq]) : -(-mg_table[piece][from_sq ^ 56] + mg_table[piece][to_sq ^ 56]);
            eg_score += (us == WHITE) ? (-eg_table[piece][from_sq] + eg_table[piece][to_sq]) : -(-eg_table[piece][from_sq ^ 56] + eg_table[piece][to_sq ^ 56]);
            zobrist_hash ^= Zobrist::pieces[us][piece][to_sq];
        }

        // Handle en passant
        uint64_t ep_mask = 0ULL;
        if (move.is_en_passant()) {
            int ep_captured_sq = (us == WHITE) ? (to_sq - 8) : (to_sq + 8);
            ep_mask = 1ULL << ep_captured_sq;
            bitboards[them][PAWN] ^= ep_mask;
            mg_score += (us == WHITE) ? (mg_table[PAWN][ep_captured_sq ^ 56]) : -(mg_table[PAWN][ep_captured_sq]);
            eg_score += (us == WHITE) ? (eg_table[PAWN][ep_captured_sq ^ 56]) : -(eg_table[PAWN][ep_captured_sq]);
            game_phase -= PHASE_VALUES[PAWN];
            zobrist_hash ^= Zobrist::pieces[them][PAWN][ep_captured_sq];
        }

        // Handle double pawn push en passant
        if (en_passant_square != -1) {
            zobrist_hash ^= Zobrist::en_passant[en_passant_square % 8];
        }
        if (move.is_double_pawn_push()) {
            en_passant_square = (us == WHITE) ? (from_sq + 8) : (from_sq - 8);
        } else {
            en_passant_square = -1;
        }
        if (en_passant_square != -1) {
            zobrist_hash ^= Zobrist::en_passant[en_passant_square % 8];
        }
        

        // Handle castling
        bool is_castle = (piece == KING) && (abs(to_sq - from_sq) == 2);
        uint64_t rook_move_mask = 0ULL;
        if (is_castle) {
        if (us == WHITE) {
            if (to_sq == G1) { // Short
                rook_move_mask = ((1ULL << H1) | (1ULL << F1));
                bitboards[WHITE][ROOK] ^= rook_move_mask;
                mg_score += (-mg_table[ROOK][H1] + mg_table[ROOK][F1]);
                eg_score += (-eg_table[ROOK][H1] + eg_table[ROOK][F1]);
                zobrist_hash ^= Zobrist::pieces[WHITE][ROOK][H1];
                zobrist_hash ^= Zobrist::pieces[WHITE][ROOK][F1];
            }
            else if (to_sq == C1) { // Long
                rook_move_mask = ((1ULL << A1) | (1ULL << D1));
                bitboards[WHITE][ROOK] ^= rook_move_mask;
                mg_score += (-mg_table[ROOK][A1] + mg_table[ROOK][D1]);
                eg_score += (-eg_table[ROOK][A1] + eg_table[ROOK][D1]);
                zobrist_hash ^= Zobrist::pieces[WHITE][ROOK][A1];
                zobrist_hash ^= Zobrist::pieces[WHITE][ROOK][D1];
            }
        } else { // us == BLACK
            if (to_sq == G8) { // Short
                rook_move_mask = ((1ULL << H8) | (1ULL << F8));
                bitboards[BLACK][ROOK] ^= rook_move_mask;
                mg_score -= (-mg_table[ROOK][H1] + mg_table[ROOK][F1]); // Use H1 and F1 because it's black's perspective
                eg_score -= (-eg_table[ROOK][H1] + eg_table[ROOK][F1]);
                zobrist_hash ^= Zobrist::pieces[BLACK][ROOK][H8];
                zobrist_hash ^= Zobrist::pieces[BLACK][ROOK][F8];
            }
            else if (to_sq == C8) { // Long
                rook_move_mask = ((1ULL << A8) | (1ULL << D8));
                bitboards[BLACK][ROOK] ^= rook_move_mask;
                mg_score -= (-mg_table[ROOK][A1] + mg_table[ROOK][D1]);
                eg_score -= (-eg_table[ROOK][A1] + eg_table[ROOK][D1]);
                zobrist_hash ^= Zobrist::pieces[BLACK][ROOK][A8];
                zobrist_hash ^= Zobrist::pieces[BLACK][ROOK][D8];
            }
        }
}
        // Update castling rights
        int castling_rights_before = castling_rights;
        
        castling_rights &= castling_rights_update[from_sq];
        castling_rights &= castling_rights_update[to_sq];
        
        bool castling_rights_changed = (castling_rights_before != castling_rights);
        
        if (castling_rights_changed) {
            zobrist_hash ^= Zobrist::castling[castling_rights_before];
            zobrist_hash ^= Zobrist::castling[castling_rights];
        }

        // Rebuild occupancy
        occupancy[us] ^= (is_castle) ? (move_mask | rook_move_mask) : move_mask;

        if (move.is_capture()) {
            occupancy[them] ^= (move.is_en_passant()) ? ep_mask : to_mask;
        }

        occupancy[BOTH] = occupancy[WHITE] | occupancy[BLACK];
        occupancy[NEITHER] = ~occupancy[BOTH];

        side_to_move ^= 1;

        occupancy[USABLE] = ~occupancy[side_to_move];
        
        // Update clock and move history for draw by repetition and 50 move rule
        if (piece == PAWN || move.is_capture()) {
            reversible_history.clear();
            halfmove_clock = -1;
        } else if (castling_rights_changed) {
            reversible_history.clear();
        }
        reversible_history.push_back(zobrist_hash);
        halfmove_clock++;
    }

    uint64_t compute_zobrist_hash() const {
        uint64_t hash = 0;

        // 1. XOR all pieces currently on the board
        for (int sq = 0; sq < 64; sq++) {
            uint64_t mask = 1ULL << sq;
            int found_piece = -1;
            int found_color = -1;

            // Scan your bitboards to see what is on this square
            for (int c = 0; c < 2; c++) {
                for (int p = 0; p < 6; p++) {
                    if (bitboards[c][p] & mask) {
                        found_piece = p;
                        found_color = c;
                        break;
                    }
                }
            }

            // If a piece is there, XOR its unique random number into the hash
            if (found_piece != -1) {
                hash ^= Zobrist::pieces[found_color][found_piece][sq];
            }
        }

        // 2. XOR the side to move if it's Black
        if (side_to_move == BLACK) {
            hash ^= Zobrist::side_to_move;
        }

        // 3. XOR castling rights (0 to 15 index depending on rights flags)
        hash ^= Zobrist::castling[castling_rights];

        // 4. XOR en passant file if a capture is legally available
        if (en_passant_square != -1) {
            int file = en_passant_square % 8;
            hash ^= Zobrist::en_passant[file];
        }

        return hash;
    }
};


struct ScoredMove {
    Move move;
    int16_t score;
};


enum TTFlag : uint8_t {
    TT_EXACT,
    TT_ALPHA, // Upper bound
    TT_BETA   // Lower bound
};


struct TTEntry {
    // Stores all important information for a transposition table entry

    uint64_t key;       // 8 bytes: Full Zobrist Hash
    int32_t  score;     // 4 bytes: Calculated evaluation
    uint16_t move_raw;  // 2 bytes: Raw integer representation of the Move
    uint8_t  depth;     // 1 byte:  Search depth achieved
    uint8_t  flag;      // 1 byte:  TT_EXACT, TT_ALPHA, or TT_BETA
};


class TranspositionTable {
    private:
        std::vector<TTEntry> table;
        size_t entry_count = 0;

    public:
        // Size should be a power of 2 for ultra-fast bitwise masking!
        // A table size of 2^20 entries takes roughly 16 Megabytes of RAM.
        void resize(size_t mega_bytes) {
            size_t bytes = mega_bytes * 1024 * 1024;
            entry_count = bytes / sizeof(TTEntry);
            
            // Round down to the nearest power of 2
            size_t p2 = 1;
            while ((p2 << 1) <= entry_count) {
                p2 <<= 1; // p2 *= 2
            }
            entry_count = p2;

            table.assign(entry_count, TTEntry{0, 0, 0, 0, 0});
        }


        void clear() {
            table.assign(entry_count, TTEntry{0, 0, 0, 0, 0});
        }

        // 3. Retrieve a cached position
        bool lookup(uint64_t hash, int& score, Move& move, int depth, int alpha, int beta) {
            size_t index = hash & (entry_count - 1); // Squishes all possible hashes in the range entry_count
            const TTEntry& entry = table[index];

            // Cache Miss: Hash mismatch means either empty slot or a collision
            if (entry.key != hash) {
                return false; 
            }

            // Expose the best move found here regardless of depth (Great for Move Ordering!)
            move.data = entry.move_raw;

            // Only use the score if the cached depth is deep enough
            if (entry.depth >= depth) {
                if (entry.flag == TT_EXACT) {
                    score = entry.score;
                    return true;
                }
                // If it's an Alpha bound, it's only useful if it's worse than our current alpha
                if (entry.flag == TT_ALPHA && entry.score <= alpha) {
                    score = alpha;
                    return true;
                }
                // If it's a Beta bound, it's only useful if it's better than our current beta
                if (entry.flag == TT_BETA && entry.score >= beta) {
                    score = beta;
                    return true;
                }
            }
            return false; // Depth was too shallow to use the score directly
        }

        // 4. Save a newly calculated position
        void store(uint64_t hash, int score, const Move& move, int depth, uint8_t flag) {
            size_t index = hash & (entry_count - 1);
            
            // Simple replacement strategy: Overwrite if the new search was deeper
            // or if the existing slot belongs to a completely different layout.
            if (table[index].key != hash || depth >= table[index].depth) {
                table[index].key = hash;
                table[index].score = score;
                table[index].move_raw = move.data;
                table[index].depth = depth;
                table[index].flag = flag;
            }
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

    void init_piece_value_tables();

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

    std::pair<int, Move> search(const Board& board, int ply, int depth, int alpha, int beta);

    void generate_ordered_moves(const Board& board, std::vector<ScoredMove>& ordered_list, Move& move_guess);

    uint64_t nodes_searched = 0;
    bool search_aborted = false;
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int max_time = 0;

    TranspositionTable tt;

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

    Move make_best_move(int max_depth, int max_time_ms);

    Move find_best_move(int max_depth, int max_time_ms);
};

}

#endif // CHESS_ENGINE_H