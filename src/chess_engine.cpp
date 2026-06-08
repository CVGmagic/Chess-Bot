#include "chess_engine.h"
#include "movegen.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <vector>
#include <random>

namespace godot {




ChessEngine::ChessEngine() {
    static bool sliding_lookup_initialised = false;

    if (!sliding_lookup_initialised) {
        init_sliders_database(); 
        sliding_lookup_initialised = true; 
    }
}


ChessEngine::~ChessEngine() {
}


// 3. MUST have ChessEngine:: right here!
void ChessEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_random_legal_move"), &ChessEngine::get_random_legal_move);
    ClassDB::bind_method(D_METHOD("set_board_from_array", "setup_board_array", "side_to_move", "castling_rights"), &ChessEngine::set_board_from_array);
    ClassDB::bind_method(D_METHOD("try_move", "from_rank", "from_file", "to_rank", "to_file", "promo_choice"), &ChessEngine::try_move);
}


void ChessEngine::generate_pseudo_legal_moves(const Board& board, std::vector<Move>& move_list) {
    // TODO Can probably be optimized by replacing occupancy[EMPTY] with ~occupancy[BOTH]
    // This requires less memory with only slightly more computation

    // Determine which side we are
    int us = board.side_to_move;
    int them = us ^ 1;

    // Map out the enemy and friendly pieces and all squares

    // GENERATE LEGAL PAWN MOVES
    uint64_t pawns = board.bitboards[us][PAWN];
    while (pawns) {
        int from_sq = pop_lsb(pawns);

        // Start with attacks
        uint64_t raw_attacks = get_pawn_attacks(us, from_sq);
        uint64_t standard_captures = raw_attacks & board.occupancy[them];
        while (standard_captures) {
            int to_sq = pop_lsb(standard_captures);
            // Check for promotion
            if ((us == WHITE && to_sq >= 56) || (us == BLACK && to_sq <= 7)) {
                move_list.push_back(Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_N));
                move_list.push_back(Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_B));
                move_list.push_back(Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_R));
                move_list.push_back(Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_Q));
            } else {
                move_list.push_back(Move(from_sq, to_sq, FLAG_CAPTURE));
            }
        }
        if (board.en_passant_square != -1) {
            uint64_t ep_bit = 1ULL << board.en_passant_square;
            if (raw_attacks & ep_bit) {
                move_list.push_back(Move(from_sq, board.en_passant_square, FLAG_EN_PASSANT));
            }
        }

        // Normal moves (with bounds checks to avoid undefined large shifts)
        if (us == WHITE) {
            int forward_sq = from_sq + 8;
            if (forward_sq < 64 && ((1ULL << forward_sq) & board.occupancy[NEITHER])) { // Square in front is empty
                if (forward_sq >= 56) { // Back rank
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_KNIGHT));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_BISHOP));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_ROOK));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_QUEEN));
                }
                else {
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_QUIET));
                    if (from_sq / 8 == 1) { // Starting square
                        int double_push = from_sq + 16;
                        if (double_push < 64 && ((1ULL << double_push) & board.occupancy[NEITHER])) {
                            move_list.push_back(Move(from_sq, double_push, FLAG_DOUBLE_PAWN_PUSH));
                        }
                    } 
                }
            }
        } else { // us == BLACK
            int forward_sq = from_sq - 8;
            if (forward_sq >= 0 && ((1ULL << forward_sq) & board.occupancy[NEITHER])) { // Square in front is empty
                if (forward_sq <= 7) { // Back rank
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_KNIGHT));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_BISHOP));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_ROOK));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_QUEEN));
                }
                else {
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_QUIET));
                    if (from_sq / 8 == 6) { // Starting square
                        int double_push = from_sq - 16;
                        if ((1ULL << double_push) & board.occupancy[NEITHER]) {
                            move_list.push_back(Move(from_sq, double_push, FLAG_DOUBLE_PAWN_PUSH));
                        }
                    } 
                }
            }
        }
    }

    // GENERATE LEGAL KNIGHT MOVES
    uint64_t knights = board.bitboards[us][KNIGHT];
    while (knights) {
        int from_sq = pop_lsb(knights);
        uint64_t raw_attacks = get_knight_attacks(from_sq, board.occupancy[USABLE]);

        uint64_t captures = raw_attacks & board.occupancy[them];
        uint64_t quiet_moves = raw_attacks & board.occupancy[NEITHER];
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL BISHOP MOVES
    uint64_t bishops = board.bitboards[us][BISHOP];
    while (bishops) {
        int from_sq = pop_lsb(bishops);
        uint64_t raw_attacks = get_bishop_attacks(from_sq, board.occupancy[BOTH], board.occupancy[USABLE]);

        uint64_t captures = raw_attacks & board.occupancy[them];
        uint64_t quiet_moves = raw_attacks & board.occupancy[NEITHER];
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL ROOK MOVES
    uint64_t rooks = board.bitboards[us][ROOK];
    while (rooks) {
        int from_sq = pop_lsb(rooks);
        uint64_t raw_attacks = get_rook_attacks(from_sq, board.occupancy[BOTH], board.occupancy[USABLE]);

        uint64_t captures = raw_attacks & board.occupancy[them];
        uint64_t quiet_moves = raw_attacks & board.occupancy[NEITHER];
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL QUEEN MOVES
    uint64_t queens = board.bitboards[us][QUEEN];
    while (queens) {
        int from_sq = pop_lsb(queens);
        uint64_t raw_attacks = get_queen_attacks(from_sq, board.occupancy[BOTH], board.occupancy[USABLE]);

        uint64_t captures = raw_attacks & board.occupancy[them];
        uint64_t quiet_moves = raw_attacks & board.occupancy[NEITHER];
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL KING MOVES
    uint64_t king_bitboard = board.bitboards[us][KING];
    if (king_bitboard) { // If the King exists
        int king_square = pop_lsb(king_bitboard);
        uint64_t raw_attacks = get_king_attacks(king_square, board.occupancy[USABLE]);

        uint64_t captures = raw_attacks & board.occupancy[them];
        uint64_t quiet_moves = raw_attacks & board.occupancy[NEITHER];
        while (captures) {move_list.push_back(Move(king_square, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(king_square, pop_lsb(quiet_moves), FLAG_QUIET));}
    }
}


void ChessEngine::set_board_from_array(const PackedInt32Array &setup_board_array, int32_t side_to_move, int32_t castling_rights) {
    
    board.clear();

    if (setup_board_array.size() != 64) {
        UtilityFunctions::printerr("ChessEngine Error: Board array must contain exactly 64 elements!");
        return;
    }

    for (int i = 0; i < setup_board_array.size(); i++) {
        int piece = setup_board_array[i];
        
        if (piece == WHITE_PAWN) {
            board.set_bit(board.bitboards[WHITE][PAWN], i);
            continue;
        }
        if (piece == WHITE_KNIGHT) {
            board.set_bit(board.bitboards[WHITE][KNIGHT], i);
            continue;
        }
        if (piece == WHITE_BISHOP) {
            board.set_bit(board.bitboards[WHITE][BISHOP], i);
            continue;
        }
        if (piece == WHITE_ROOK) {
            board.set_bit(board.bitboards[WHITE][ROOK], i);
            continue;
        }
        if (piece == WHITE_QUEEN) {
            board.set_bit(board.bitboards[WHITE][QUEEN], i);
            continue;
        }
        if (piece == WHITE_KING) {
            board.set_bit(board.bitboards[WHITE][KING], i);
            continue;
        }


        if (piece == BLACK_PAWN) {
            board.set_bit(board.bitboards[BLACK][PAWN], i);
            continue;
        }
        if (piece == BLACK_KNIGHT) {
            board.set_bit(board.bitboards[BLACK][KNIGHT], i);
            continue;
        }
        if (piece == BLACK_BISHOP) {
            board.set_bit(board.bitboards[BLACK][BISHOP], i);
            continue;
        }
        if (piece == BLACK_ROOK) {
            board.set_bit(board.bitboards[BLACK][ROOK], i);
            continue;
        }
        if (piece == BLACK_QUEEN) {
            board.set_bit(board.bitboards[BLACK][QUEEN], i);
            continue;
        }
        if (piece == BLACK_KING) {
            board.set_bit(board.bitboards[BLACK][KING], i);
            continue;
        }
    }

    board.side_to_move = side_to_move;
    board.castling_rights = castling_rights;

    int other_side = side_to_move ^ 1;

    for (int piece_type = 0; piece_type < 6; ++piece_type) {
        board.occupancy[side_to_move] |= board.bitboards[side_to_move][piece_type];
        board.occupancy[other_side] |= board.bitboards[other_side][piece_type];
    }
    board.occupancy[BOTH] = board.occupancy[side_to_move] | board.occupancy[other_side];
    board.occupancy[NEITHER]  = ~board.occupancy[BOTH];
    board.occupancy[USABLE]  = ~board.occupancy[side_to_move];
}


int ChessEngine::get_random_pseudo_legal_move() {
    std::vector<Move> move_list;
    generate_pseudo_legal_moves(board, move_list);

    if (move_list.empty()) {
        return 0;
    }

    // Pick random move
    std::uniform_int_distribution<size_t> dist(0, move_list.size() - 1);
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd());
    Move random_move = move_list[dist(gen)];

    return random_move.data; // Returns the move as an int
}


int ChessEngine::get_random_legal_move() {
    std::vector<Move> legal_moves;
    generate_legal_moves(board, legal_moves);

    if (legal_moves.empty()) {
        return 0;
    }

    // Pick random move
    std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd());
    Move random_move = legal_moves[dist(gen)];

    return random_move.data; // Returns the move as an int
}


bool ChessEngine::is_square_attacked(const Board& board, int square, int enemy_color) {
    int friendly_color = enemy_color ^ 1;

    // Cheap checks first
    if (get_pawn_attacks(friendly_color, square) & board.bitboards[enemy_color][PAWN]) {return true;}

    if (get_knight_attacks(square, ~0ULL) & board.bitboards[enemy_color][KNIGHT]) {return true;}
    
    if (get_king_attacks(square, ~0ULL) & board.bitboards[enemy_color][KING]) {return true;}

    // More expensive checks later
    // Queen check is built into Rook and Bishop checks
    if (get_bishop_attacks(square, board.occupancy[BOTH], ~0ULL) & (board.bitboards[enemy_color][BISHOP] | board.bitboards[enemy_color][QUEEN])) {return true;}

    if (get_rook_attacks(square, board.occupancy[BOTH], ~0ULL) & (board.bitboards[enemy_color][ROOK] | board.bitboards[enemy_color][QUEEN])) {return true;}

    return false;
}


bool ChessEngine::is_in_check(const Board& board, int color) {
    // Color refers to the color that's not moving, so might be in check
    uint64_t king_bb = board.bitboards[color][KING];
    
    // Falls der König (z.B. in Test-Setups) nicht existiert
    if (!king_bb) return false;

    int king_square;
    #if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, king_bb);
    king_square = index;
    #else
    king_square = __builtin_ctzll(king_bb);
    #endif

    // Prüfe, ob das Feld des Königs von den gegnerischen Figuren bedroht wird
    return is_square_attacked(board, king_square, color ^ 1);
}


void ChessEngine::generate_legal_moves(const Board& board, std::vector<Move>& move_list) {
    // TODO Optimize by running seperate functions if king is in check
    // and using X-ray masking for pinned pieces to avoid checking
    // legality of every move

    std::vector<Move> pseudo_moves;
    generate_pseudo_legal_moves(board, pseudo_moves);

    for (Move move : pseudo_moves) {
        // Copy the boards
        Board simulated_board = board;
        simulated_board.make_move(move);

        if (!is_in_check(simulated_board, board.side_to_move)) {
            move_list.push_back(move);
        }
    }
}


bool ChessEngine::try_move(int32_t from_rank, int32_t from_file, int32_t to_rank, int32_t to_file, int32_t promo_choice) {
    int from_sq = from_rank * 8 + from_file % 8;
    int to_sq = to_rank * 8 + to_file % 8;

    std::vector<Move> legal_moves;
    generate_legal_moves(board, legal_moves);


    for (const Move& move : legal_moves) {
        if (move.get_from() == from_sq && move.get_to() == to_sq) {
            // Handle promotion
            if (move.is_promotion()) {
                if (move.get_promo_piece_type() == promo_choice) {
                    board.make_move(move);
                    return true;
                }
                continue;
            }
            
            board.make_move(move);
            return true;
        }
    }
    return false;
}


}