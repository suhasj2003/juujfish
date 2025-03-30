#include <iostream>
#include <chrono>

#include "types.h"
#include "bitboard.h"
#include "position.h"
#include "movegen.h"

using namespace Juujfish;

std::string pretty(BitBoard b) {

    std::string s = "+---+---+---+---+---+---+---+---+\n";

    for (int r = RANK_8; r >= RANK_1; --r)
    {
        for (int f = FILE_A; f <= FILE_H; ++f)
            s += b & Square((r << 3) + f) ? "| X " : "|   ";

        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
    }
    s += "  a   b   c   d   e   f   g   h\n";

    return s;
}

std::string pretty(Position &p) {

    std::string s = "+---+---+---+---+---+---+---+---+\n";

    for (int r = RANK_8; r >= RANK_1; --r)
    {
        for (int f = FILE_A; f <= FILE_H; ++f) {
            Piece piece = p.piece_at(Square((r << 3) + f));
            if (piece != NO_PIECE) {
                s += "| " + std::string(1, piece_to_char(piece)) + " ";
            } else {
                s += "|   ";
            }
        }  
        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
    }
    s += "  a   b   c   d   e   f   g   h\n";

    return s;
}

int squareIndex(const std::string& pos) {
    return (pos[1] - '1') * 8 + (pos[0] - 'a');
}

// Converts move string to a Move object
Move parseMove(const std::string& moveStr) {
    if (moveStr.length() < 4 || moveStr.length() > 5) {
        throw std::invalid_argument("Invalid move string length");
    }

    int from = squareIndex(moveStr.substr(0, 2));
    int to = squareIndex(moveStr.substr(2, 2));
    MoveType moveType = NORMAL;
    PieceType promotionPiece = KNIGHT;

    if (moveStr.length() == 5) {
        switch (moveStr[4]) {
            case 'n': promotionPiece = KNIGHT; break;
            case 'b': promotionPiece = BISHOP; break;
            case 'r': promotionPiece = ROOK; break;
            case 'q': promotionPiece = QUEEN; break;
            default: throw std::invalid_argument("Invalid promotion piece");
        }
        return Move::make<PROMOTION>(Square(to), Square(from), promotionPiece);
    }

    // Special case detection (castling, en passant) would need board state
    if ((from == 4 && to == 6) || (from == 60 && to == 62)) {
        Move::make<CASTLING>(Square(to), Square(from));
    } else if ((from == 4 && to == 2) || (from == 60 && to == 58)) {
        Move::make<CASTLING>(Square(to), Square(from));
    }

    return Move::make<NORMAL>(Square(to), Square(from));
}

std::string moveToString(Move m) {
    std::string moveStr;

    Square from = m.from_sq();
    Square to = m.to_sq();

    moveStr += char('a' + file_of(from));
    moveStr += char('1' + rank_of(from));
    moveStr += char('a' + file_of(to));
    moveStr += char('1' + rank_of(to));

    if (m.type_of() == PROMOTION) {
        switch (m.promotion_type()) {
            case KNIGHT: moveStr += 'n'; break;
            case BISHOP: moveStr += 'b'; break;
            case ROOK: moveStr += 'r'; break;
            case QUEEN: moveStr += 'q'; break;
            default: break;
        }
    } else if (m.type_of() == CASTLING) {
        if (from == Square(4) && to == Square(6)) {
            return "e1g1"; // White kingside castling
        } else if (from == Square(4) && to == Square(2)) {
            return "e1c1"; // White queenside castling
        } else if (from == Square(60) && to == Square(62)) {
            return "e8g8"; // Black kingside castling
        } else if (from == Square(60) && to == Square(58)) {
            return "e8c8"; // Black queenside castling
        }
    }

    return moveStr;
}

uint64_t perft(Position &pos, int depth, int d) {
    if (depth == 0) {
        return 1;
    }
    uint64_t num_positions = 0;
    StateInfo st;


    if (pos.is_in_check()) {
        for (auto m : MoveList<EVASIONS>(pos)) {
            if (pos.legal(m)) {
                if (depth == 1) {
                    if (depth == d) {
                        std::cout << moveToString(m) << ": " << 1 << std::endl;
                    }
                    num_positions++;
                } else {
                    pos.make_move(m, &st, pos.gives_check(m));
                    int temp = perft(pos, depth-1, d);
                    if (depth == d) {
                        std::cout << moveToString(m) << ": " << temp << std::endl;
                    }
                    num_positions += temp;
                    pos.unmake_move();
                }
            }
        }
    } else {
        for (auto m : MoveList<NON_EVASIONS>(pos)) {
            if (pos.legal(m)) {
                if (depth == 1) {
                    if (depth == d) {
                        std::cout << moveToString(m) << ": " << 1 << std::endl;
                    }
                    
                    num_positions++;
                } else {
                    pos.make_move(m, &st, pos.gives_check(m));
                    int temp = perft(pos, depth-1, d);
                    if (depth == d) {
                        std::cout << moveToString(m) << ": " << temp << std::endl;
                    }
                    num_positions += temp;
                    pos.unmake_move();
                }
            } 
        }
    }

    return num_positions;
}


int main(int argc, char const *argv[])
{
    BitBoards::init();
    Position::init();

    auto states = new std::deque<StateInfo>(1);


    Position p;
    p.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &states->back());

    
    // std::cout << pretty(p) << std::endl;
    // std::cout << pretty(p.get_blockers(WHITE)) << std::endl;
    // std::cout << pretty(p.get_pinners(BLACK)) << std::endl;



    // p.set("r3k1rQ/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N4p/PPPBBPPP/R3K2R b KQq - 0 1", &states->back());


    // std::cout << pretty(p) << std::endl;
    // std::cout << "MoveList size: " << MoveList<NON_EVASIONS>(p).size() << std::endl;
    // for (auto m : MoveList<NON_EVASIONS>(p))
    //     std::cout << moveToString(m) << std::endl;
        
    // std::cout << std::endl << std::endl;
    // for (auto m : MoveList<NON_EVASIONS>(p))
    //     if (!p.legal(m))
    //         std::cout << moveToString(m) << std::endl;

    // std::string fen;
    // std::cout << "Enter fen: ";
    // std::cin >> fen;


    while (true) {
        std::cout << "Enter depth: ";

        int depth;
        std::cin >> depth;

        auto start = std::chrono::high_resolution_clock::now();
        uint64_t total_positions = perft(p, depth, depth);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
        std::cout << "Perft execution time: " << duration.count() << " milliseconds" << std::endl;
        std::cout << std::endl << "Number of positions at depth: " << depth << ": " << total_positions << std::endl << std::endl;
    }

    // int depth = 7;

    // auto start = std::chrono::high_resolution_clock::now();
    // uint64_t total_positions = perft(p, depth);
    // auto end = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // std::cout << "Perft execution time: " << duration.count() << " milliseconds" << std::endl;
    // std::cout << std::endl << "Number of positions at depth: " << depth << ": " << total_positions << std::endl << std::endl;
    
    return 0;

}
