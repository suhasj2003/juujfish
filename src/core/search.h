#ifndef SEARCH_H
    #define SEARCH_H

    #include "types.h"
    #include "bitboard.h"
    #include "position.h"
    #include "movegen.h"
    #include "heuristic.h"
    #include "moveorder.h"
    #include "evaluation.h"

    namespace Juujfish {       
        class Worker {
            public:
                Worker() = default;
                Worker(Worker &w) = delete;
                
                void init(); 

                template<bool RootNode> 
                Value search(Position &pos, Value alpha, Value beta, int depth);
                // Value search(Position &pos, int depth);

                inline Move get_best_move() { return best_move; }
                inline int get_prunes() const { return prunes; }
                inline int get_nodes() const { return nodes; }

                inline int get_searched_depth() const { return searched_depth; }
                inline void set_searched_depth(int d) { searched_depth = d; }

            private:
                int searched_depth;
                int prunes = 0;
                int nodes = 0;
                Move best_move;

                KillerHeuristic killer;
                HistoryHeuristic history;
                ButterflyHeuristic butterfly;
        };
        
    } // namespace Juujfish

#endif // SEARCH_H