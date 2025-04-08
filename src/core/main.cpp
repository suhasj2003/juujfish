#include <iostream>
#include <chrono>

#include "types.h"
#include "bitboard.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "evaluation.h"
#include "misc.h"

using namespace Juujfish;

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

void game() {

    auto states = new std::deque<StateInfo>(1);
    int fixed_depth = 8;

    Position p;
    p.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &states->back());
    // p.set("8/8/7k/8/8/5q2/7K/3r4 b - - 3 78", &states->back());

    bool mate_or_draw = false;
    Value score = VALUE_INFINITE;

    std::cout << pretty(p) << std::endl;

    srand(time(NULL));
    Color ENGINE = Color(rand() & 1);
    // Color ENGINE = WHITE;
    // Color ENGINE = BLACK;

    while (!mate_or_draw) {
        std::string input;

        if (p.get_side_to_move() == ENGINE) {
            Worker w;
            w.init();

            w.set_searched_depth(fixed_depth);


            auto start = std::chrono::high_resolution_clock::now();
            score = w.search<true>(p, -VALUE_INFINITE, VALUE_INFINITE, w.get_searched_depth());
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            Move m = Move(w.get_best_move().raw());

            if (score == -(VALUE_MATE) && m.is_nullmove()) {
                mate_or_draw = true;
                continue;
            }

            score = ENGINE == WHITE ? score : -score;

            states->emplace_back();

            p.make_move(m, &states->back(), p.gives_check(m));

            // std::cout << "Best move: " << moveToString(m) << " (score: " << score << ")" << std::endl;               

            std::cout << std::endl;
            std::cout << pretty(p) << std::endl;
            std::cout << "Best move: " << moveToString(m) << " (score: " << (double) score / 100 << ")" << std::endl;
            std::cout << "Nodes: " << w.get_nodes() << std::endl;
            std::cout << "Prunes: " << w.get_prunes() << std::endl;
            std::cout << "Search execution time: " << (double) duration.count() / 1000 << " seconds" << std::endl << std::endl;

            if (score == VALUE_DRAW || score == (VALUE_MATE - 1))
                mate_or_draw = true;

        } else {
            std::cout << "Enter move: " << std::endl;
            std::cin >> input;

            Move m = parseMove(p, input);

            if (m.is_nullmove()) {
                std::cout << "Move is not valid. Try again." << "\n" << std::endl;
                continue;
            } else if (!p.legal(m)) {
                std::cout << "Move is not legal. Try again." << std::endl;
                continue;
            }
        
            states->emplace_back();

            p.make_move(m, &states->back(), p.gives_check(m));

            std::cout << std::endl;
            std::cout << pretty(p) << std::endl;
        }
    }

    std::cout << std::endl;
    if (score == VALUE_DRAW) {
        std::cout << "DRAW" << std::endl;
    } else if (score == (VALUE_MATE - 1) || score == -(VALUE_MATE)) {
        std::cout << "CHECKMATE!!!" << std::endl;
    } else {
        std::cerr << "Error: Neither checkmate or draw." << std::endl;
    }

}

int main()
{
    BitBoards::init();
    Position::init();

   
    game();

    // Position p;

    // auto states = new std::vector<StateInfo>(1);

    // p.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &states->back());
    // std::cout << pretty(p) << std::endl;

    // while (true) {
    //     std::cout << "Enter depth: ";

    //     int depth;
    //     std::cin >> depth;
    //     std::cout << std::endl;

    //     auto start = std::chrono::high_resolution_clock::now();
    //     uint64_t total_positions = perft(p, depth, depth);
    //     auto end = std::chrono::high_resolution_clock::now();
    //     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    //     std::cout << std::endl << "Perft execution time: " << duration.count() << " milliseconds" << std::endl;
    //     std::cout << std::endl << "Number of positions at depth: " << depth << ": " << total_positions << std::endl;
    //     std::cout << std::endl << "MNPs: " << (total_positions / duration.count()) / 1000 << std::endl << std::endl;
    // }
    
    return 0;

}
