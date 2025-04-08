#ifndef MOVEORDER_H
    #define MOVEORDER_H

    #include <iostream>

    #include "types.h"
    #include "bitboard.h"
    #include "position.h"
    #include "movegen.h"
    #include "heuristic.h"
    #include "misc.h"

    namespace Juujfish {

        enum Stage {
            CAPTURES_GEN,
            CAPTURE,
            QUIETS_GEN,
            QUIET,
            BAD_CAPTURE,
            BAD_QUIET,

            // EVASIONS:
            EVASIONS_GEN,
            EVASION
        };

        inline Stage& operator++(Stage& stage) {
            stage = static_cast<Stage>((static_cast<int>(stage) + 1));
            return stage;
        }

        class MoveOrderer {
            public:
                MoveOrderer(const Position &pos,
                            int ply,
                            const KillerHeuristic *kh, 
                            const HistoryHeuristic *hh, 
                            const ButterflyHeuristic *bh) :
                    pos(pos), ply(ply), killers(kh), history(hh), butterfly(bh) {
                    stage = pos.is_in_check() ? EVASIONS_GEN : CAPTURES_GEN;
                }
                Move next();
                void skip_quiets_moves() { skip_quiets = true; } 
            private:
                template<typename Pred>
                Move select(Pred filter);

                template<GenType Gt>
                void score();

                GradedMove *begin() { return curr; }
                GradedMove *end() { return end_moves; }

                GradedMove moves[MAX_MOVES];
                GradedMove *curr, *end_moves, *end_bad_captures, *begin_bad_quiets, *end_bad_quiets;

                Stage stage;

                const Position &pos;

                // Depth       depth;
                int         ply;

                const KillerHeuristic     *killers;
                const HistoryHeuristic    *history;
                const ButterflyHeuristic  *butterfly;

                bool skip_quiets = false;
        };
        

    } // namespace Juujfish

#endif // ifndef MOVEORDER_H