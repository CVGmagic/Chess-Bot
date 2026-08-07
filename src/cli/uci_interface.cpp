#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include "../core/chess_engine.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
using namespace std;


ChessCore::ChessEngine engine;

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


int32_t fen_char_to_id(char c) {
    switch(c) {
        case 'P': return WHITE_PAWN;   case 'N': return WHITE_KNIGHT;
        case 'B': return WHITE_BISHOP; case 'R': return WHITE_ROOK;
        case 'Q': return WHITE_QUEEN;  case 'K': return WHITE_KING;
        case 'p': return BLACK_PAWN;   case 'n': return BLACK_KNIGHT;
        case 'b': return BLACK_BISHOP; case 'r': return BLACK_ROOK;
        case 'q': return BLACK_QUEEN;  case 'k': return BLACK_KING;
        default: return 0; // Empty square
    }
}


void parse_fen_to_engine(std::string fen, ChessCore::ChessEngine& engine) {
    std::cerr << "parsing fen " << fen << "\n";
    int32_t raw_squares[64] = {0};
    
    // Split FEN by spaces
    std::stringstream ss(fen);
    std::string piece_part, turn_part, castle_part;
    ss >> piece_part >> turn_part >> castle_part;

    // 1. Fill raw_squares
    int rank = 7; // Start at rank 8 (top of FEN)
    int file = 0;
    for (char c : piece_part) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += (c - '0');
        } else {
            // Map 2D (rank, file) to 1D array index
            int index = rank * 8 + file;
            raw_squares[index] = fen_char_to_id(c);
            file++;
        }
    }

    // 2. Parse turn
    int32_t side = -1;
    if (turn_part == "w") {
        side = 0;
    } else if (turn_part == "b") {
        side = 1;
    } else {
        std::cerr << "Unexpected turn part " << turn_part << " encountered\n";
    }

    // 3. Parse simple castling rights (example for KQkq)
    int32_t rights = 0;
    if (castle_part.find('K') != std::string::npos) rights |= 0b0001;
    if (castle_part.find('Q') != std::string::npos) rights |= 0b0010;
    if (castle_part.find('k') != std::string::npos) rights |= 0b0100;
    if (castle_part.find('q') != std::string::npos) rights |= 0b1000;

    // Finally, pass to your existing function
    engine.set_board_from_array(raw_squares, side, rights);
}


int uci_square_to_square(const string& square_string) {
    if (square_string.length() < 2) {
        return -1;
    }
    int file = square_string[0] - 'a';
    int rank = square_string[1] - '1';
    return rank * 8 + file;
}


string square_to_uci(int square) {
    string square_string;
    square_string += 'a' + square % 8;
    square_string += '1' + square / 8;
    return square_string;
}


string move_to_uci(ChessCore::Move& move) {
    if (move.is_none() || move.data == 0) {
        return "0000";
    }

    string uci_move = square_to_uci(move.get_from()) + square_to_uci(move.get_to());

    if (move.is_promotion()) {
        int promo_type = move.get_promo_piece_type();
        
        // Maps perfectly back to your custom promo flags:
        // Knight (1), Bishop (2), Rook (3), Queen (4)
        if (promo_type == 1)      uci_move += 'n';
        else if (promo_type == 2) uci_move += 'b';
        else if (promo_type == 3) uci_move += 'r';
        else if (promo_type == 4) uci_move += 'q';
    }

    return uci_move;
}


vector<string> split_string(const string& str) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}


void apply_uci_move(const string& move_string) {
    int from_sq = uci_square_to_square(move_string.substr(0, 2));
    int to_sq = uci_square_to_square(move_string.substr(2, 2));

    int promo_choice = 0;
    if (move_string.length() == 5) { // Promotion
        char promo_char = move_string[4];
        switch(promo_char) {
            case 'n' : promo_choice = 1; break;
            case 'b' : promo_choice = 2; break;
            case 'r' : promo_choice = 3; break;
            case 'q' : promo_choice = 4; break;
            default : cerr << "Unexpected promotion type\n";
        }

    }

    engine.make_opponent_move(from_sq, to_sq, promo_choice);
}


void parse_position_command(stringstream& ss) {
    std::string token;
    ss >> token; // Grab the next word after "position"

    if (token == "startpos") {
        // 1. Reset engine to standard layout
        engine.set_board_to_startpos(); 
        ss >> token; // Check if the next word is "moves"
    } 
    else if (token == "fen") {
        // 1. Reconstruct and parse the 6 FEN tokens
        std::string full_fen = "";

        // Lies ein Wort nach dem anderen aus dem stringstream
        while (ss >> token) {
            // Wenn das Wort "moves" kommt, gehört der Rest nicht mehr zur FEN
            if (token == "moves") {
                break; 
            }
            
            // Füge die Teile mit einem Leerzeichen dazwischen zusammen
            if (!full_fen.empty()) {
                full_fen += " ";
            }
            full_fen += token;
        }

        // Jetzt enthält 'full_fen' die komplette korrekte FEN!
        parse_fen_to_engine(full_fen, engine);
    }

    // 2. Play through historical moves sequentially if present
    if (token == "moves") {
        std::string move_str;
        while (ss >> move_str) {
            // Parse coordinates and apply it to the board memory
            apply_uci_move(move_str); 
        }
    }
}


void start_search(int depth, int max_time_ms) {
    ChessCore::Move best_move = engine.debug_search_with_tt(depth, max_time_ms);

    cout << "bestmove " << move_to_uci(best_move) << "\n" << flush;
}


void parse_go_command(stringstream& ss) {
    string token;

    int depth = -1;
    int movetime = -1;

    int wtime = 0, btime = 0, winc = 0, binc = 0;
    
    while (ss >> token) {
        if (token == "depth") {
            ss >> depth;
        }
        else if (token == "movetime") {
            ss >> movetime;
        }
        else if (token == "wtime") {
            ss >> wtime;
        }
        else if (token == "btime") {
            ss >> btime;
        }
        else if (token == "winc") {
            ss >> winc;
        }
        else if (token == "binc") {
            ss >> binc;
        }
    }

    if (movetime == -1 && depth == -1) { // Movetime and depth commands have precedence over time left
        int my_time = engine.get_side_to_move() == 0 ? (wtime) : (btime);

        if (my_time) {
            int my_inc = engine.get_side_to_move() == 0 ? (winc) : (binc);

            movetime = min(my_time / 25 + my_inc / 2, my_time);
            depth = 100; // Set depth to something unreachable
        }
        else {
            movetime = 100; // Set default to 100 ms
            depth = 100; // Set depth to something unreachable
        }
    }
    
    // If only either depth or movetime is set, set the other
    else if (movetime == -1) {
        movetime = 1'000'000;
    }
    else if (depth == -1) {
        depth = 100;
    }

    start_search(depth, movetime);
}


void process_command(stringstream& ss) {
    string cmd;
    ss >> cmd;

    if (cmd == "uci") {
        cout << "id name MyChessEngine v1.0\n";
        cout << "id author Caius Grobbel\n";
        cout << "uciok\n" << flush;
    }
    else if (cmd == "ucinewgame") {
        engine.reset_state();
    }
    else if (cmd == "isready") {
        cout << "readyok\n" << flush;
    }
    else if (cmd == "position") {
        parse_position_command(ss);
    }
    else if (cmd == "go") {
        parse_go_command(ss);
    }
    else if (cmd == "quit") {
        exit(0);
    }
    else {
        cerr << "Unknown command " << cmd << " encountered\n";
    }
}


extern "C" {
    #ifdef __EMSCRIPTEN__
    EMSCRIPTEN_KEEPALIVE
    #endif
    void send_uci_command(const char* command) {
        stringstream ss(command);
        process_command(ss);
    }
}

int main() {
    string line;

    while (getline(cin, line)) {
        // std::cerr << "[ENGINE INBOUND]: " << line << std::endl;

        if (line.empty()) {
            continue;
        }

        stringstream ss(line);

        process_command(ss);
    }
}
