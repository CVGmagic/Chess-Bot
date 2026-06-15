#include "chess_engine.h"
#include "movegen.h"
#include <vector>
#include <random>
#include <iostream>
#include <chrono>

namespace ChessCore {

ChessEngine::ChessEngine() {
    static bool sliding_lookup_initialised = false;

    if (!sliding_lookup_initialised) {
        init_sliders_database(); 
        sliding_lookup_initialised = true; 
    }

    // Initialize between matrix
    static bool between_matrix_initialised = false;
    if (!between_matrix_initialised) {
        init_between_matrix();
        between_matrix_initialised = true;
    }

    // Initialze zobrist arrays
    Zobrist::initialize();

    // Allocate transposition table memory
    tt.resize(16); // TODO make this changeble through config.yml
}


ChessEngine::~ChessEngine() {}


void ChessEngine::init_piece_value_tables() {
    for (int p = 0; p < 6; p++) {
        std::cerr << "Piece " << p << std::endl; 
        for (int sq = 0; sq < 64; sq++) {
            std::cerr << eg_table_pure[p][sq] + piece_values[p] << ", ";
            
            if ((sq + 1) % 8 == 0) {
                std::cerr << std::endl;
            }
        }
        std::cerr << std::endl;
    }
}


void ChessEngine::generate_pseudo_legal_moves(const Board& board, std::vector<Move>& move_list) {
    // TODO Can probably be optimized by replacing occupancy[NEITHER] with ~occupancy[BOTH]
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
        
        b.mg_score = 0; // Equal material at the start
        b.eg_score = 0;

        for (int p = 0; p < 6; p++) {
            b.game_phase += PHASE_VALUES[p] * 2; // For the 2 players
        }

        b.zobrist_hash = b.compute_zobrist_hash();

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
            board.mg_score += mg_table[PAWN][i];
            board.eg_score += eg_table[PAWN][i];
            board.game_phase += PHASE_VALUES[PAWN];
            continue;
        }
        if (piece == WHITE_KNIGHT) {
            board.set_bit(board.bitboards[WHITE][KNIGHT], i);
            board.mg_score += mg_table[KNIGHT][i];
            board.eg_score += eg_table[KNIGHT][i];
            board.game_phase += PHASE_VALUES[KNIGHT];
            continue;
        }
        if (piece == WHITE_BISHOP) {
            board.set_bit(board.bitboards[WHITE][BISHOP], i);
            board.mg_score += mg_table[BISHOP][i];
            board.eg_score += eg_table[BISHOP][i];
            board.game_phase += PHASE_VALUES[BISHOP];
            continue;
        }
        if (piece == WHITE_ROOK) {
            board.set_bit(board.bitboards[WHITE][ROOK], i);
            board.mg_score += mg_table[ROOK][i];
            board.eg_score += eg_table[ROOK][i];
            board.game_phase += PHASE_VALUES[ROOK];
            continue;
        }
        if (piece == WHITE_QUEEN) {
            board.set_bit(board.bitboards[WHITE][QUEEN], i);
            board.mg_score += mg_table[QUEEN][i];
            board.eg_score += eg_table[QUEEN][i];
            board.game_phase += PHASE_VALUES[QUEEN];
            continue;
        }
        if (piece == WHITE_KING) {
            board.set_bit(board.bitboards[WHITE][KING], i);
            board.mg_score += mg_table[KING][i];
            board.eg_score += eg_table[KING][i];
            board.game_phase += PHASE_VALUES[KING];
            continue;
        }


        if (piece == BLACK_PAWN) {
            board.set_bit(board.bitboards[BLACK][PAWN], i);
            board.mg_score -= mg_table[PAWN][i];
            board.eg_score -= eg_table[PAWN][i];
            board.game_phase += PHASE_VALUES[PAWN];
            continue;
        }
        if (piece == BLACK_KNIGHT) {
            board.set_bit(board.bitboards[BLACK][KNIGHT], i);
            board.mg_score -= mg_table[KNIGHT][i];
            board.eg_score -= eg_table[KNIGHT][i];
            board.game_phase += PHASE_VALUES[KNIGHT];
            continue;
        }
        if (piece == BLACK_BISHOP) {
            board.set_bit(board.bitboards[BLACK][BISHOP], i);
            board.mg_score -= mg_table[BISHOP][i];
            board.eg_score -= eg_table[BISHOP][i];
            board.game_phase += PHASE_VALUES[BISHOP];
            continue;
        }
        if (piece == BLACK_ROOK) {
            board.set_bit(board.bitboards[BLACK][ROOK], i);
            board.mg_score -= mg_table[ROOK][i];
            board.eg_score -= eg_table[ROOK][i];
            board.game_phase += PHASE_VALUES[ROOK];
            continue;
        }
        if (piece == BLACK_QUEEN) {
            board.set_bit(board.bitboards[BLACK][QUEEN], i);
            board.mg_score -= mg_table[QUEEN][i];
            board.eg_score -= eg_table[QUEEN][i];
            board.game_phase += PHASE_VALUES[QUEEN];
            continue;
        }
        if (piece == BLACK_KING) {
            board.set_bit(board.bitboards[BLACK][KING], i);
            board.mg_score -= mg_table[KING][i];
            board.eg_score -= eg_table[KING][i];
            board.game_phase += PHASE_VALUES[KING];
            continue;
        }
    }

    board.side_to_move = side_to_move;
    board.castling_rights = castling_rights;

    board.zobrist_hash = board.compute_zobrist_hash();

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


uint64_t ChessEngine::get_absolute_pins(const Board& board, int king_sq) {
    // Returns a mask with all pinned pieces
    int us = board.side_to_move;
    int them = us ^ 1;

    uint64_t pinned_mask = 0;

    // Pretend like only their pieces are on the board
    uint64_t bishop_pinners = get_bishop_attacks(king_sq, board.occupancy[them], ~0ULL) & (board.bitboards[them][BISHOP] | board.bitboards[them][QUEEN]);
    uint64_t rook_pinners = get_rook_attacks(king_sq, board.occupancy[them], ~0ULL) & (board.bitboards[them][ROOK] | board.bitboards[them][QUEEN]);

    uint64_t all_pinners = bishop_pinners | rook_pinners;

    while (all_pinners) {
        int pinner_sq = pop_lsb(all_pinners); // Get next attacking slider square
        
        uint64_t squares_between = between_matrix[king_sq][pinner_sq];
        
        uint64_t blockers = squares_between & board.occupancy[BOTH];
        
        if (blockers && (blockers & (blockers - 1)) == 0) { // Exactly one piece between king and enemy
            if (blockers & board.occupancy[us]) { // That piece belongs to us
                pinned_mask |= blockers;
            }
        }
    }

    return pinned_mask;
}


void ChessEngine::generate_legal_moves(const Board& board, std::vector<Move>& move_list) {
    // TODO Optimize by running seperate functions if king is in check

    uint64_t king_bitboard = board.bitboards[board.side_to_move][KING];
    int king_sq = pop_lsb(king_bitboard);
    uint64_t pin_mask = get_absolute_pins(board, king_sq);
    bool in_check = is_in_check(board, board.side_to_move);

    std::vector<Move> pseudo_moves;
    generate_pseudo_legal_moves(board, pseudo_moves);

    for (Move move : pseudo_moves) {
        // Not pinned piece and not king move and not currently in check and not en-passant (because en passant can remove two pieces from a rank at once)
        if (!(pin_mask & 1ULL << move.get_from()) && (move.get_from() != king_sq) && (!in_check) && (move.get_flag() != FLAG_EN_PASSANT)) {
            move_list.push_back(move);
        }
        // Safe Fallback
        else {
            Board simulated_board = board;
            simulated_board.make_move(move);
            if (!is_in_check(simulated_board, board.side_to_move)) {
                move_list.push_back(move);
            }
        }
    }
}


void ChessEngine::generate_ordered_moves(const Board& board, std::vector<ScoredMove>& ordered_list, Move& move_guess) {
    // Generates and orders all legal moves for a given board
    // TODO Generate pseudo legal first, and only check legality of not pruned
    std::vector<Move> raw_moves;
    generate_legal_moves(board, raw_moves);
    ordered_list.reserve(raw_moves.size());

    int us = board.side_to_move;
    int them = us ^ 1;

    for (const Move& move : raw_moves) {
        ScoredMove scored_move;
        scored_move.move = move;
        scored_move.score = 0;
    
        int from_sq = move.get_from();
        int to_sq = move.get_to();
        uint64_t to_mask = 1ULL << to_sq;
        uint64_t from_mask = 1ULL << from_sq;

        if (move.data == move_guess.data) {
            scored_move.score = 32000; // Almost the int16_t max
        }
        else if (move.is_capture()) {
            int attacker = -1;
            int victim = -1;

            if (move.is_en_passant()) {
                attacker = PAWN;
                victim = PAWN;
            }
            else {
                for (int p = 0; p < 6; p++) {
                    if (board.bitboards[us][p] & from_mask) {
                        attacker = p;
                        break;
                    }
                }

                for (int p = 0; p < 6; p++) {
                    if (board.bitboards[them][p] & to_mask) {
                        victim = p;
                        break;
                    }
                }
            }
            scored_move.score = 10000 + SORT_VALUES[victim] * 10 - SORT_VALUES[attacker];
        }
        else if (move.is_promotion()) {
            scored_move.score = 9000 + SORT_VALUES[move.get_promo_piece_type()];
        }
        else {
            int piece = -1;
            for (int p = 0; p < 6; p++) {
                if (board.bitboards[us][p] & from_mask) {
                    piece = p;
                    break;
                }
            }
            
            int mg_diff = 0;
            int eg_diff = 0;
            int phase = board.game_phase;

            // Clamp phase to be safe
            if (phase > 24) phase = 24;
            if (phase < 0)  phase = 0;

            if (us == WHITE) {
                mg_diff = mg_table[piece][to_sq] - mg_table[piece][from_sq];
                eg_diff = eg_table[piece][to_sq] - eg_table[piece][from_sq];
            } else {
                // Adjust indices for Black's perspective
                mg_diff = mg_table[piece][to_sq ^ 56] - mg_table[piece][from_sq ^ 56];
                eg_diff = eg_table[piece][to_sq ^ 56] - eg_table[piece][from_sq ^ 56];
            }
            // Blend positional differences
            scored_move.score = ((phase * mg_diff) + ((24 - phase) * eg_diff)) / 24;
        }
        ordered_list.push_back(scored_move);
    }

    std::sort(ordered_list.begin(), ordered_list.end(), [](const ScoredMove& a, const ScoredMove& b) {
        return a.score > b.score;
    });
}


int ChessEngine::try_move(int32_t from_rank, int32_t from_file, int32_t to_rank, int32_t to_file, int32_t promo_choice) {
    int from_sq = from_rank * 8 + from_file;
    int to_sq = to_rank * 8 + to_file;

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
    int phase = board.game_phase;
    if (phase > 24) {phase = 24;}
    if (phase < 0) {phase = 0;}

    if (board.side_to_move == WHITE) {
        return (board.game_phase * board.mg_score + (24 - board.game_phase) * board.eg_score) / 24;
    } else {
        return -(board.game_phase * board.mg_score + (24 - board.game_phase) * board.eg_score) / 24;
    }
}


std::pair<int, Move> ChessEngine::minmax(const Board& board, int depth) {
    // Returns the evaluation of the outcome if both sides play perfectly for depth moves
    nodes_searched++;
    
    if (depth == 0) {
        return {evaluate(board), Move(0, 0)};
    }

    std::vector<Move> legal_moves;
    generate_legal_moves(board, legal_moves);

    if (legal_moves.empty()) {
        if (is_in_check(board, board.side_to_move)) { // Checkmate
            return {-29000 - depth, Move(0, 0)};
        }
        return {0, Move(0, 0)}; // Stalemate
    }

    int max_score = -999999;
    Move best_move;

    for (const Move& move : legal_moves) {
        Board simulated_board = board;
        simulated_board.make_move(move);

        int score = -minmax(simulated_board, depth - 1).first;
        if (score > max_score) {
            max_score = score;
            best_move = move;
        }
    }
    return {max_score, best_move};
}


Move ChessEngine::find_best_move(int max_depth, int max_time_ms) {
    nodes_searched = 0;
    start_time = std::chrono::high_resolution_clock::now();
    max_time = max_time_ms;

    Move best_move_overall;
    int best_score_overall = -INF;

    for (int current_depth = 1; current_depth <= max_depth; current_depth++) {
        std::pair<int, Move> res = search(board, 0, current_depth, -INF, INF);

        if (search_aborted) {
            break; 
        }

        best_move_overall = res.second;
        best_score_overall = res.first;

        // 5. Early Exit Heuristic: If we used up more than half our allowed time 
        // on this depth, we will almost certainly run out of time trying the next one.
        /*
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        if (elapsed > max_time_ms / 2) {
            break;
        }
        */
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end_time - start_time;
    double seconds = elapsed.count();
    
    std::cerr << "Positions searched: " << nodes_searched << "\n";
    std::cerr << "Time elapsed: " << seconds << "\n";
    std::cerr << "Nodes per second: " << (nodes_searched / seconds) << "\n";

    return best_move_overall;
}


Move ChessEngine::make_best_move(int max_depth, int max_time_ms) {
    Move best_move = find_best_move(max_depth, max_time_ms);
    board.make_move(best_move);
    return best_move;
}


std::pair<int, Move> ChessEngine::search(const Board& board, int ply, int depth, int alpha, int beta) {
    nodes_searched++;

    int alpha_orig = alpha;
    int remaining_depth = depth - ply;

    if (nodes_searched % 16384 == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        
        if (elapsed >= max_time) {
            search_aborted = true;
        }
    }

    if (search_aborted) {
        return {0, Move(0, 0)};
    }

    if (ply == depth) {
        return {evaluate(board), Move(0, 0)};
    }

    // Use transposition table
    Move hash_move = Move(0, 0);
    int tt_score = 0;

    // Check if we have seen this layout before
    if (tt.lookup(board.zobrist_hash, tt_score, hash_move, remaining_depth, alpha, beta)) {
        // Make mate depth based again
        if (tt_score > 28000)  tt_score -= ply;
        else if (tt_score < -28000) tt_score += ply;

        return {tt_score, hash_move}; 
    }


    std::vector<ScoredMove> ordered_moves;
    generate_ordered_moves(board, ordered_moves, hash_move);

    if (ordered_moves.empty()) {
        if (is_in_check(board, board.side_to_move)) { // Checkmate
            return {-29000 + ply, Move(0, 0)}; // Make further checkmates better
        }
        return {0, Move(0, 0)}; // Stalemate
    }
    // Draw by repetition or 50 move rule
    if (ply > 0) {
        if (board.halfmove_clock >= 100) {
        return {0, Move(0, 0)};
        }
        int same_pos_count = 0;
        for (int i = 0; i + 1 < board.reversible_history.size(); i++) {
            if (board.reversible_history[i] == board.zobrist_hash) {
                same_pos_count++;
            }
        }
        // This detects if the position has occurred before
        // Not technically draw, but computer shouldn't just shuffle pieces back and forth
        if (same_pos_count >= 1) {
            return {0, Move(0, 0)};
        }
    }
    
    Move best_move_at_this_node;
    int best_score = -INF;

    for (const ScoredMove& move : ordered_moves) {
        Board simulated_board = board;
        simulated_board.make_move(move.move);

        int score = -search(simulated_board, ply + 1, depth, -beta, -alpha).first;
    
        if (score > best_score) {
            best_score = score;
            best_move_at_this_node = move.move;
        }

        if (score >= beta) { // Opponent will never allow this
            break;
        }

        if (score > alpha) { // Remember what we have found (pruning occurs one recursion step further)
            alpha = score;
        }
    }

    uint8_t flag = TT_EXACT;
    if (best_score <= alpha_orig) flag = TT_ALPHA; // Failed low
    else if (best_score >= beta)  flag = TT_BETA;  // Failed high

    // Only save the move if we aren't returning a blank dummy move on a cutoff
    if (!search_aborted) {
        // Convert mate score to depth based
        int store_score = best_score;
        if (store_score > 28000)  store_score += ply;
        else if (store_score < -28000) store_score -= ply;

        tt.store(board.zobrist_hash, store_score, best_move_at_this_node, remaining_depth, flag);
    }

    return {best_score, best_move_at_this_node};
}


}