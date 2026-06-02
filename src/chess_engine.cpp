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
    ClassDB::bind_method(D_METHOD("get_test_value"), &ChessEngine::get_test_value);
}


void ChessEngine::generate_pseudo_legal_moves(std::vector<Move>& move_list) {
    // Determine which side we are
    int us = board.side_to_move;
    int them = us ^ 1;

    // Map out the enemy and friendly pieces and all squares
    uint64_t friendly_pieces = 0ULL;
    uint64_t enemy_pieces    = 0ULL;
    for (int piece_type = 0; piece_type < 6; ++piece_type) {
        friendly_pieces |= board.bitboards[us][piece_type];
        enemy_pieces    |= board.bitboards[them][piece_type];
    }
    uint64_t total_occupancy = friendly_pieces | enemy_pieces;
    uint64_t empty_squares   = ~total_occupancy;
    uint64_t usable_squares  = ~friendly_pieces;

    // GENERATE LEGAL PAWN MOVES
    uint64_t pawns = board.bitboards[us][PAWN];
    while (pawns) {
        int from_sq = pop_lsb(pawns);

        // Start with attacks
        uint64_t raw_attacks = get_pawn_attacks(us, from_sq);
        uint64_t standard_captures = raw_attacks & enemy_pieces;
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

        // Normal moves
        if (us == WHITE) {
            int forward_sq = from_sq + 8;
            if ((1ULL << forward_sq) & empty_squares) { // Square in front is empty
                if (forward_sq >= 56) { // Back rank
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_KNIGHT));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_BISHOP));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_ROOK));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_QUEEN));
                }
                else {
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_QUIET));
                    if (from_sq / 8 == 1) { // Starting square
                        // forward_sq was empty, so no need to check again
                        int double_push = from_sq + 16;
                        if ((1ULL << double_push) & empty_squares) {
                            move_list.push_back(Move(from_sq, double_push, FLAG_DOUBLE_PAWN_PUSH));
                        }
                    } 
                }
            }
        } else { // us == BLACK
            int forward_sq = from_sq - 8;
            if ((1ULL << forward_sq) & empty_squares) { // Square in front is empty
                if (forward_sq <= 7) { // Back rank
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_KNIGHT));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_BISHOP));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_ROOK));
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_PROMO_QUEEN));
                }
                else {
                    move_list.push_back(Move(from_sq, forward_sq, FLAG_QUIET));
                    if (from_sq / 8 == 6) { // Starting square
                        // forward_sq was empty, so no need to check again
                        int double_push = from_sq - 16;
                        if ((1ULL << double_push) & empty_squares) {
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
        uint64_t raw_attacks = get_knight_attacks(from_sq, usable_squares);

        uint64_t captures = raw_attacks & enemy_pieces;
        uint64_t quiet_moves = raw_attacks & empty_squares;
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL BISHOP MOVES
    uint64_t bishops = board.bitboards[us][BISHOP];
    while (bishops) {
        int from_sq = pop_lsb(bishops);
        uint64_t raw_attacks = get_bishop_attacks(from_sq, total_occupancy, usable_squares);

        uint64_t captures = raw_attacks & enemy_pieces;
        uint64_t quiet_moves = raw_attacks & empty_squares;
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL ROOK MOVES
    uint64_t rooks = board.bitboards[us][ROOK];
    while (rooks) {
        int from_sq = pop_lsb(rooks);
        uint64_t raw_attacks = get_rook_attacks(from_sq, total_occupancy, usable_squares);

        uint64_t captures = raw_attacks & enemy_pieces;
        uint64_t quiet_moves = raw_attacks & empty_squares;
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL QUEEN MOVES
    uint64_t queens = board.bitboards[us][QUEEN];
    while (queens) {
        int from_sq = pop_lsb(queens);
        uint64_t raw_attacks = get_queen_attacks(from_sq, total_occupancy, usable_squares);

        uint64_t captures = raw_attacks & enemy_pieces;
        uint64_t quiet_moves = raw_attacks & empty_squares;
        while (captures) {move_list.push_back(Move(from_sq, pop_lsb(captures), FLAG_CAPTURE));}
        while (quiet_moves) {move_list.push_back(Move(from_sq, pop_lsb(quiet_moves), FLAG_QUIET));}
    }

    // GENERATE LEGAL KING MOVES
    uint64_t king_bitboard = board.bitboards[us][KING];
    int king_square = pop_lsb(king_bitboard);
    uint64_t raw_attacks = get_king_attacks(king_square, usable_squares);

    uint64_t captures = raw_attacks & enemy_pieces;
    uint64_t quiet_moves = raw_attacks & empty_squares;
    while (captures) {move_list.push_back(Move(king_square, pop_lsb(captures), FLAG_CAPTURE));}
    while (quiet_moves) {move_list.push_back(Move(king_square, pop_lsb(quiet_moves), FLAG_QUIET));}
}


void ChessEngine::set_board_from_array(const PackedInt32Array &setup_board_array) {
    
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
}


int ChessEngine::get_random_legal_move() {
    std::vector<Move> move_list;
    generate_pseudo_legal_moves(move_list);

    // Pick random move
    std::uniform_int_distribution<size_t> dist(0, move_list.size() - 1);
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd());
    Move random_move = move_list[dist(gen)];

    return random_move.data; // Returns the move as an int
}


}