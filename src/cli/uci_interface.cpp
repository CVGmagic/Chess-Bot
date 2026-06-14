#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "../core/chess_engine.h"
using namespace std;


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


void apply_uci_move(const string& move_string, ChessCore::ChessEngine& engine) {
    int from_sq = uci_square_to_square(move_string.substr(0, 2));
    int to_sq = uci_square_to_square(move_string.substr(2, 2));

    int promo_choice = 0;
    if (move_string.length() == 5) { // Promotion
        char promo_char = move_string[4];
        switch(promo_char) {
            case 'n' : promo_choice = 1;
            case 'b' : promo_choice = 2;
            case 'r' : promo_choice = 3;
            case 'q' : promo_choice = 4;
            default : cerr << "Unexpected promotion type\n";
        }

    }

    engine.make_opponent_move(from_sq, to_sq, promo_choice);
}


void parse_position_command(stringstream& ss, ChessCore::ChessEngine& engine) {
    std::string token;
    ss >> token; // Grab the next word after "position"

    if (token == "startpos") {
        // 1. Reset engine to standard layout
        engine.set_board_to_startpos(); 
        ss >> token; // Check if the next word is "moves"
    } 
    else if (token == "fen") {
        // 1. Reconstruct and parse the 6 FEN tokens
        std::string fen_string = "";
        for (int i = 0; i < 6; ++i) {
            std::string part;
            ss >> part;
            fen_string += part + " ";
        }
        // engine.set_board_from_fen(fen_string);
        ss >> token; // Check if the next word is "moves"
    }

    // 2. Play through historical moves sequentially if present
    if (token == "moves") {
        std::string move_str;
        while (ss >> move_str) {
            // Parse "e2e4" coordinates and apply it to the board memory
            apply_uci_move(move_str, engine); 
        }
    }
}


void start_search(int depth, ChessCore::ChessEngine& engine) {
    ChessCore::Move best_move = engine.find_best_move(depth);

    cout << "bestmove " << move_to_uci(best_move) << "\n" << flush;
}


void parse_go_command(stringstream& ss, ChessCore::ChessEngine& engine) {
    string token;

    int depth = 7;
    
    while (ss >> token) {
        if (token == "depth") {
            ss >> depth;
            break;
        }
    }

    start_search(depth, engine);
}


int main() {
    ChessCore::ChessEngine engine;
    string line;

    while (getline(cin, line)) {
        // std::cerr << "[ENGINE INBOUND]: " << line << std::endl;

        if (line.empty()) {
            continue;
        }

        stringstream ss(line);

        string cmd;
        ss >> cmd;

        if (cmd == "uci") {
            cout << "id name MyChessEngine v1.0\n";
            cout << "id author Caius Grobbel\n";
            cout << "uciok\n" << flush;
        }
        else if (cmd == "isready") {
            cout << "readyok\n" << flush;
        }
        else if (cmd == "position") {
            parse_position_command(ss, engine);
        }
        else if (cmd == "go") {
            parse_go_command(ss, engine);
        }
        else if (cmd == "quit") {
            break; 
        }
        else {
            cerr << "Unknown command encountered\n";
        }
    }
}