#include "search.h"

namespace Juujfish {

    void Worker::init() {
        best_move = Move(0);

        killer.init();
        history.init();
        butterfly.init();
    }

    template<bool RootNode>
    Value Worker::search(Position &pos, Value alpha, Value beta, int depth) {
        Value best_score = -VALUE_INFINITE;
        Value score = 0;
        bool moves_exist = false;

        Color us = pos.get_side_to_move();
        int ply = searched_depth - depth;

        nodes++;

        if (pos.is_draw()) {
            return VALUE_DRAW;
        }
        
        if (depth == 0) 
            return pos.get_side_to_move() == WHITE ? evaluate(pos) : -evaluate(pos);
        
        StateInfo new_st;
        memset(&new_st, 0, sizeof(new_st));

        MoveOrderer mo(pos, ply, &killer, &history, &butterfly);
        Move m = mo.next();

        while (!m.is_nullmove()) {
            if (pos.legal(m)) {
                moves_exist = true;
                pos.make_move(m, &new_st, pos.gives_check(m));
                score = -search<false>(pos, -beta, -alpha, depth-1);
                pos.unmake_move();

                if (score > best_score) {
                    best_score = score;
                    if (RootNode)
                        best_move = m;
                }

                butterfly.update(m, us, depth);

                alpha = std::max(alpha, score);
                if (score > beta) {
                    killer.update(m, ply);
                    history.update(m, us, type_of(pos.piece_at(m.from_sq())), depth);

                    prunes++; 
                    return best_score;
                }
            }
            m = mo.next();
        }

        if (!moves_exist) {
            if (pos.is_in_check())
                return -(VALUE_MATE - (get_searched_depth() - depth));
            else 
                return VALUE_DRAW;
        }

        return best_score;
    }


    template Value Worker::search<true>(Position &pos, Value alpha, Value beta, int depth);
    template Value Worker::search<false>(Position &pos, Value alpha, Value beta, int depth);

} // namespace Juujfish