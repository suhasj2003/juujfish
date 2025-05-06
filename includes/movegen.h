#ifndef MOVEGEN_H
    #define MOVEGEN_H

    #include "types.h"
    #include "bitboard.h"
    #include "position.h"

    namespace Juujfish {

        const int MAX_MOVES = 256;
        
        enum GenType { CAPTURES, QUIETS, EVASIONS, NON_EVASIONS, LEGAL };

        struct GradedMove : public Move {
            int value;

            void operator=(Move m) {data = m.raw();}

            // To avoid any implicit conversions to float
            operator float() const = delete;
        };

        inline bool operator<(GradedMove m1, GradedMove m2) { return m1.value < m2.value; }
        inline bool operator>(GradedMove m1, GradedMove m2) { return m1.value > m2.value; }

        template<GenType Gt, Direction D, bool Enemy>
        GradedMove* generate_promotions(GradedMove *move_list, Square to);
        
        template<Color C, GenType Gt>
        GradedMove* generate_pawn_moves(const Position &pos, GradedMove *move_list, BitBoard target);

        template<Color C, PieceType Pt>
        GradedMove* generate_piece_moves(const Position &pos, GradedMove *move_list, BitBoard target);

        template<Color C, GenType Gt>
        GradedMove* generate_moves(const Position &pos, GradedMove *move_list);

        template<GenType Gt>
        GradedMove* generate(const Position &pos, GradedMove *move_list);
        

        template<GenType Gt>
        struct MoveList {
            public:
                explicit MoveList(const Position &pos) : last(generate<Gt>(pos, move_list)) {}

                const GradedMove *begin() { return move_list; }
                const GradedMove *end() { return last; }
                size_t      size() { return last - move_list; }
                bool        contains(Move m) { return std::find(begin(), end(), m) != end(); } 

            private:
                GradedMove move_list[MAX_MOVES], *last;
        };


    } // namespace Juujfish 

#endif // #ifndef MOVEGEN_H