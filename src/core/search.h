#ifndef SEARCH_H
    #define SEARCH_H

    #include "types.h"
    #include "bitboard.h"
    #include "position.h"
    #include "movegen.h"
    #include "evaluation.h"

    namespace Juujfish {       
        class Worker {
            public:
                Worker() = default;
                Worker(Worker &w) = delete;
                
                void init(); 

                template<bool RootNode> 
                Value search(Position &pos, int depth);

                inline Move get_best_move() { return best_move; }

            private:
                Move best_move;
        };
        
    } // namespace Juujfish

#endif // SEARCH_H