#include "chess_engine.h"
#include "movegen.h"
#include <vector>
#include <random>
#include <iostream>

namespace ChessCore {

ChessEngine::ChessEngine() {
    static bool sliding_lookup_initialised = false;

    if (!sliding_lookup_initialised) {
        init_sliders_database(); 
        sliding_lookup_initialised = true; 
    }
}


ChessEngine::~ChessEngine() {}


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

        // Handle castling
        if (us == WHITE) {
            if (board.castling_rights & 0b0001) { // O-O
                if (!board.get_bit(board.occupancy[BOTH], F1) && !board.get_bit(board.occupancy[BOTH], G1)) {
                    if (!is_square_attacked(board, E1, BLACK) && !is_square_attacked(board, F1, BLACK)) {
                        move_list.push_back(Move(E1, G1, FLAG_KING_CASTLE));
                    }
                }
            }
            if (board.castling_rights & 0b0010) { // O-O-O
                if (!board.get_bit(board.occupancy[BOTH], B1) && !board.get_bit(board.occupancy[BOTH], C1) && !board.get_bit(board.occupancy[BOTH], D1)) {
                    if (!is_square_attacked(board, D1, BLACK) && !is_square_attacked(board, E1, BLACK)) {
                        move_list.push_back(Move(E1, C1, FLAG_QUEEN_CASTLE));
                    }
                }
            }
        } else { // us == BLACK
            if (board.castling_rights & 0b0100) { // O-O
                if (!board.get_bit(board.occupancy[BOTH], F8) && !board.get_bit(board.occupancy[BOTH], G8)) {
                    if (!is_square_attacked(board, E8, WHITE) && !is_square_attacked(board, F8, WHITE)) {
                        move_list.push_back(Move(E8, G8, FLAG_KING_CASTLE));
                    }
                }
            }
            if (board.castling_rights & 0b1000) { // O-O-O
                if (!board.get_bit(board.occupancy[BOTH], B8) && !board.get_bit(board.occupancy[BOTH], C8) && !board.get_bit(board.occupancy[BOTH], D8)) {
                    if (!is_square_attacked(board, D8, WHITE) && !is_square_attacked(board, E8, WHITE)) {
                        move_list.push_back(Move(E8, C8, FLAG_QUEEN_CASTLE));
                    }
                }
            }
        }
    }
}


void ChessEngine::set_board_to_startpos() {
    static const Board START_BOARD = []() {
        Board b;
        b.clear(); // Ensure everything starts at 0

        // White pieces
        b.bitboards[WHITE][PAWN]   = 0x000000000000FF00ULL; // Rank 2
        b.bitboards[WHITE][ROOK]   = 0x0000000000000081ULL; // A1, H1
        b.bitboards[WHITE][KNIGHT] = 0x0000000000000042ULL; // B1, G1
        b.bitboards[WHITE][BISHOP] = 0x0000000000000024ULL; // C1, F1
        b.bitboards[WHITE][QUEEN]  = 0x0000000000000008ULL; // D1
        b.bitboards[WHITE][KING]   = 0x0000000000000010ULL; // E1

        // Black pieces
        b.bitboards[BLACK][PAWN]   = 0x00FF000000000000ULL; // Rank 7
        b.bitboards[BLACK][ROOK]   = 0x8100000000000000ULL; // A8, H8
        b.bitboards[BLACK][KNIGHT] = 0x4200000000000000ULL; // B8, G8
        b.bitboards[BLACK][BISHOP] = 0x2400000000000000ULL; // C8, F8
        b.bitboards[BLACK][QUEEN]  = 0x0800000000000000ULL; // D8
        b.bitboards[BLACK][KING]   = 0x1000000000000000ULL; // E8

        // Game rules state info
        b.side_to_move = WHITE;     // 0
        b.castling_rights = 0b1111; // Both sides can castle both ways
        b.en_passant_square = -1;   // None
        
        b.material = 0; // Equal material at the start

        // Rebuild all initial occupancy masks
        for (int p = 0; p < 6; p++) {
            b.occupancy[WHITE] |= b.bitboards[WHITE][p];
            b.occupancy[BLACK] |= b.bitboards[BLACK][p];
        }
        b.occupancy[BOTH]    = b.occupancy[WHITE] | b.occupancy[BLACK];
        b.occupancy[NEITHER] = ~b.occupancy[BOTH];
        b.occupancy[USABLE]  = ~b.occupancy[WHITE];

        return b;
    }(); // The () here executes the logic immediately to fill START_BOARD

    this->board = START_BOARD; // Overwrite board fast
}


void ChessEngine::set_board_from_array(const int32_t* raw_squares, int32_t side_to_move, int32_t castling_rights) {  
    board.clear();

    for (int i = 0; i < 64; i++) {
        int piece = raw_squares[i];
        
        if (piece == WHITE_PAWN) {
            board.set_bit(board.bitboards[WHITE][PAWN], i);
            board.material += piece_values[PAWN];
            continue;
        }
        if (piece == WHITE_KNIGHT) {
            board.set_bit(board.bitboards[WHITE][KNIGHT], i);
            board.material += piece_values[KNIGHT];
            continue;
        }
        if (piece == WHITE_BISHOP) {
            board.set_bit(board.bitboards[WHITE][BISHOP], i);
            board.material += piece_values[BISHOP];
            continue;
        }
        if (piece == WHITE_ROOK) {
            board.set_bit(board.bitboards[WHITE][ROOK], i);
            board.material += piece_values[ROOK];
            continue;
        }
        if (piece == WHITE_QUEEN) {
            board.set_bit(board.bitboards[WHITE][QUEEN], i);
            board.material += piece_values[QUEEN];
            continue;
        }
        if (piece == WHITE_KING) {
            board.set_bit(board.bitboards[WHITE][KING], i);
            board.material += piece_values[KING];
            continue;
        }


        if (piece == BLACK_PAWN) {
            board.set_bit(board.bitboards[BLACK][PAWN], i);
            board.material -= piece_values[PAWN];
            continue;
        }
        if (piece == BLACK_KNIGHT) {
            board.set_bit(board.bitboards[BLACK][KNIGHT], i);
            board.material -= piece_values[KNIGHT];
            continue;
        }
        if (piece == BLACK_BISHOP) {
            board.set_bit(board.bitboards[BLACK][BISHOP], i);
            board.material -= piece_values[BISHOP];
            continue;
        }
        if (piece == BLACK_ROOK) {
            board.set_bit(board.bitboards[BLACK][ROOK], i);
            board.material -= piece_values[ROOK];
            continue;
        }
        if (piece == BLACK_QUEEN) {
            board.set_bit(board.bitboards[BLACK][QUEEN], i);
            board.material -= piece_values[QUEEN];
            continue;
        }
        if (piece == BLACK_KING) {
            board.set_bit(board.bitboards[BLACK][KING], i);
            board.material -= piece_values[KING];
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


int ChessEngine::try_move(int32_t from_rank, int32_t from_file, int32_t to_rank, int32_t to_file, int32_t promo_choice) {
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
                    return move.data;
                }
                continue;
            }
            
            board.make_move(move);
            return move.data;
        }
    }
    return 0;
}


Move ChessEngine::get_opponent_move(int from_sq, int to_sq, int promo_choice) {
    // Gets the move flag for a move, without validating legality
    int us = board.side_to_move;
    int them = us ^ 1;

    uint64_t from_mask = 1ULL << from_sq;
    uint64_t to_mask = 1ULL << to_sq;


    // Identify the moving piece type
    int moved_piece = -1;
    for (int p = 0; p < 6; p++) {
        if (board.bitboards[us][p] & from_mask) {
            moved_piece = p;
            break;
        }
    }

    // Figure out if the move was a capture
    bool is_capture = false;
    for (int p = 0; p < 6; p++) {
        if (board.bitboards[them][p] & to_mask) {
            is_capture = true;
            break;
        }
    }

    // Check Promotion
    if (promo_choice != 0) {
        if (is_capture) {
            switch (promo_choice) {
                case 1 : return Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_N);
                case 2 : return Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_B);
                case 3 : return Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_R);
                case 4 : return Move(from_sq, to_sq, FLAG_PROMO_CAPTURE_Q);
                default : std::cerr << "Unexpected promotion type\n"; return Move(from_sq, to_sq); 
            }
        }
        else {
            switch (promo_choice) {
                case 1 : return Move(from_sq, to_sq, FLAG_PROMO_KNIGHT);
                case 2 : return Move(from_sq, to_sq, FLAG_PROMO_BISHOP);
                case 3 : return Move(from_sq, to_sq, FLAG_PROMO_ROOK);
                case 4 : return Move(from_sq, to_sq, FLAG_PROMO_QUEEN);
                default : std::cerr << "Unexpected promotion type\n"; return Move(from_sq, to_sq); 
            }
        }
    }

    // Check Castling
    if (moved_piece == KING && std::abs(from_sq - to_sq) == 2) {
        return (to_sq > from_sq) ? Move(from_sq, to_sq, FLAG_KING_CASTLE) : Move(from_sq, to_sq, FLAG_QUEEN_CASTLE);
    }

    // Check Double Pawn Push
    if (moved_piece == PAWN && std::abs(from_sq - to_sq) == 16) {
        return Move(from_sq, to_sq, FLAG_DOUBLE_PAWN_PUSH);
    }

    // Check En Passant
    if (moved_piece == PAWN && to_sq == board.en_passant_square) {
        return Move(from_sq, to_sq, FLAG_EN_PASSANT);
    }

    if (is_capture) {
        return Move(from_sq, to_sq, FLAG_CAPTURE);
    } 
    else {
        return Move(from_sq, to_sq, FLAG_QUIET);
    }
}


void ChessEngine::make_opponent_move(int from_sq, int to_sq, int promo_choice) {
    // Makes a move based on the given parameters without checking legality
    Move opponent_move = get_opponent_move(from_sq, to_sq, promo_choice);

    board.make_move(opponent_move);
}


int32_t ChessEngine::perft_rec(const Board& board, int32_t depth) {
    if (depth == 0) {
        return 1;
    }

    std::vector<Move> legal_moves;
    generate_legal_moves(board, legal_moves);

    int32_t nodes = 0;

    for (const Move& move : legal_moves) {
        Board simulated_board = board;
        simulated_board.make_move(move);

        nodes += perft_rec(simulated_board, depth - 1);
    }

    return nodes;
}


int ChessEngine::evaluate(const Board& board) {
    if (board.side_to_move == WHITE) {
        return board.material;
    } else {
        return -board.material;
    }
}


int ChessEngine::minmax(const Board& board, int depth) {
    // Returns the evaluation of the outcome if both sides play perfectly for depth moves
    if (depth == 0) {
        return evaluate(board);
    }

    std::vector<Move> legal_moves;
    generate_legal_moves(board, legal_moves);

    if (legal_moves.empty()) {
        if (is_in_check(board, board.side_to_move)) { // Checkmate
            return -29000;
        }
        return 0; // Stalemate
    }

    int max_score = -999999;
    for (const Move& move : legal_moves) {
        Board simulated_board = board;
        simulated_board.make_move(move);

        int score = -minmax(simulated_board, depth - 1);
        if (score > max_score) {
            max_score = score;
        }
    }
    return max_score;
}


Move ChessEngine::make_best_move(int depth) {
    // Plays the best move and returns its raw data
    std::pair<int, Move> res = search(board, depth, -INF, INF);

    return res.second;
}


std::pair<int, Move> ChessEngine::search(const Board& board, int depth, int alpha, int beta) {
    if (depth == 0) {
        return {evaluate(board), Move(0, 0)};
    }

    std::vector<Move> legal_moves;
    generate_legal_moves(board, legal_moves);

    if (legal_moves.empty()) {
        if (is_in_check(board, board.side_to_move)) { // Checkmate
            return {-29000 - depth, Move(0, 0)}; // Make further checkmates better
        }
        return {0, Move(0, 0)}; // Stalemate
    }

    Move best_move_at_this_node;
    int best_score = -INF;

    for (const Move& move : legal_moves) {
        Board simulated_board = board;
        simulated_board.make_move(move);

        int score = -search(simulated_board, depth - 1, -beta, -alpha).first;

        if (score > best_score) {
            best_score = score;
            best_move_at_this_node = move;
        }

        if (score >= beta) { // Opponent will never allow this
            return {score, move};
        }

        if (score > alpha) { // Remember what I have found (pruning occurs one recursion step further)
            alpha = score;
        }
    }

    return {best_score, best_move_at_this_node};
}


}