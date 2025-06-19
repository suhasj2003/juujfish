#ifndef SEARCH_H
    #define SEARCH_H

    #include "types.h"
    #include "bitboard.h"
    #include "position.h"
    #include "movegen.h"
    #include "heuristic.h"
    #include "moveorder.h"
    #include "transposition.h"
    #include "evaluation.h"

    namespace Juujfish {

        class TranspositionTable;

        enum NodeType : uint8_t { RootNode, PVNode, CutNode };

        namespace Search {
            class Worker {
                 public:
                      Worker() = default;
                      Worker(Worker& w) = delete;

                      void init(TranspositionTable* tt, int write, int rewrite);
		      void clear(); 
		      void start_searching();
                      Value iterative_deepening(Position& pos);

                      template <bool RootNode>
                      Value search(Position& pos, Value alpha, Value beta, uint8_t depth);

                      inline Move get_best_move() { return best_move; }
                      inline int get_transpositions() const { return transpositions; }
                      inline int get_prunes() const { return prunes; }
                      inline int get_table_prunes() const { return tt_prunes; }
                      inline int get_nodes() const { return nodes; }

                      inline int get_tt_wrote() const { return tt_wrote; }
                      inline int get_tt_rewrote() const { return tt_rewrote; }

                      inline uint8_t get_searched_depth() const { return searched_depth; }
                      inline void set_searched_depth(uint8_t d) { searched_depth = d; }

                 private:
		      Position root_pos;

                      uint8_t searched_depth;
                      Move best_move;

                      KillerHeuristic killer;
                      HistoryHeuristic history;
                      ButterflyHeuristic butterfly;

                      TranspositionTable* tt = nullptr;

                      // Diagnostics:
                      int transpositions = 0;
                      int prunes = 0;
                      int tt_prunes = 0;
                      int nodes = 0;
                      int tt_wrote = 0;
                      int tt_rewrote = 0;
            };
        }
    } // namespace Juujfish

#endif // SEARCH_H
