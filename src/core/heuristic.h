#ifndef HEURISTIC_H
    #define HEURISTIC_H

    #include <algorithm>

    #include "types.h"
    #include "misc.h"

    namespace Juujfish {

        constexpr int KILLER_SCORE = 90000;
        constexpr int HISTORY_MAX = 8192;
        constexpr int BUTTERFLY_MAX = 1024;

        // Interface for different heuristics used in move ordering
        class Heuristic {
            public:
                virtual ~Heuristic() = default;

                virtual void init();
                virtual void clear();
        };

        class KillerHeuristic : public Heuristic {
            public:
                KillerHeuristic() {
                    init();
                }

                inline void init() override { killers.init(Move::null_move()); }
                inline void clear() override { killers.init(Move::null_move()); }

                inline int lookup(const Move &m, int ply) const;

                inline void update(const Move &m, int ply) { 
                    killers.set(killers.get(ply, 0), ply, 1); killers.set(m, ply, 0); 
                }

            private:
                NDArray<Move, MAX_PLY, 2> killers;
        };

        int KillerHeuristic::lookup(const Move &m, int ply) const {
            for (size_t i = 0; i < 2; ++i) {
                if (killers.get(ply, i) == m) {
                    return KILLER_SCORE - 10000 * i;
                }
            }
            return 0;
        }

        /* Both of these heuristics (History and Butterfly) will combine to give the Relative History Heuristic score. This
           should prefer moves that cause beta cutoffs but not be influenced by the frequency of those moves. */

        class HistoryHeuristic : public Heuristic {
            public:
                HistoryHeuristic() {
                    init();
                }

                inline void init() override { history.init(0); }
                inline void clear() override { history.init(0); }

                inline int lookup(const Move &m, Color c, PieceType pt) const;
                inline void update(const Move &m, Color c, PieceType pt, uint8_t depth);

            private:
                NDArray<int, COLOR_NB, PIECE_TYPE_NB, SQUARE_NB> history;
        };

        int HistoryHeuristic::lookup(const Move &m, Color c, PieceType pt) const {
            return history.get(c, pt, m.to_sq());
        }
        void HistoryHeuristic::update(const Move &m, Color c, PieceType pt, uint8_t depth) {
            int current_score = history.get(c, pt, m.to_sq());
            int clamped_score = std::clamp(depth * depth, 0, HISTORY_MAX);

            history.set(current_score + clamped_score - (current_score * clamped_score / HISTORY_MAX), c, pt, m.to_sq());
        }


        class ButterflyHeuristic : public Heuristic {
            public:
                ButterflyHeuristic() {
                    init();
                }

                inline void init() override { butterfly.init(1); } 
                inline void clear() override { butterfly.init(1); }

                inline int lookup(const Move &m, Color c) const;
                inline void update(const Move &m, Color c, uint8_t depth);

            private:
                NDArray<int, COLOR_NB, SQUARE_NB, SQUARE_NB> butterfly; 
        };

        int ButterflyHeuristic::lookup(const Move &m, Color c) const {
            return butterfly.get(c, m.from_sq(), m.to_sq());
        }

        void ButterflyHeuristic::update(const Move &m, Color c, uint8_t depth) {
            int current_score = butterfly.get(c, m.from_sq(), m.to_sq());
            int clamped_score = std::clamp(current_score + depth, 1, BUTTERFLY_MAX);

            butterfly.set(clamped_score, c, m.from_sq(), m.to_sq());
        }


    } // namespace Juujfish
#endif // ifndef HEURISTIC_H