#include "search.h"

namespace Juujfish {

    void Worker::init() {
        best_move = Move(0);
    }

    template<bool RootNode>
    Value Worker::search(Position &pos, int depth) {
        Value best_score = -VALUE_INFINITE;
        Value score = 0;
        bool moves_exist = false;
        
        if (depth == 0) 
            return pos.get_side_to_move() == WHITE ? evaluate(pos) : -evaluate(pos);
        
        StateInfo new_st;
        memset(&new_st, 0, sizeof(new_st));

        if (pos.is_in_check())
            for (auto m : MoveList<EVASIONS>(pos)) {
                if (pos.legal(m)) {
                    moves_exist = true;
                    pos.make_move(m, &new_st, pos.gives_check(m));
                    score = -search<false>(pos, depth-1);
                    pos.unmake_move();

                    if (score > best_score) {
                        best_score = score;
                        if (RootNode)
                            best_move = m;
                    }
                }
            }
        else
            for (auto m : MoveList<NON_EVASIONS>(pos)) {
                if (pos.legal(m)) {
                    moves_exist = true;
                    pos.make_move(m, &new_st, pos.gives_check(m));
                    score = -search<false>(pos, depth-1);
                    pos.unmake_move();

                    if (score > best_score) {
                        best_score = score;
                        if (RootNode)
                            best_move = m;
                    }
                }
            }

        if (!moves_exist) {
            if (pos.is_in_check())
                return -VALUE_MATE + depth;
            else 
                return VALUE_DRAW;
        }

        return best_score;
    }

    template Value Worker::search<true>(Position &pos, int depth);
    template Value Worker::search<false>(Position &pos, int depth);

} // namespace Juujfish