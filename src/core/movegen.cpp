#include "movegen.h"
#include <iostream>

namespace Juujfish {

    template<GenType Gt, Direction D, bool Enemy>
    GradedMove* generate_promotions(GradedMove *move_list, Square to) {

        if constexpr (Gt == CAPTURES || Gt == EVASIONS || Gt == NON_EVASIONS) 
            *move_list++ = Move::make<PROMOTION>(to, to - D, QUEEN);

        if constexpr ((Gt == CAPTURES && Enemy) || 
                    (Gt == QUIETS && !Enemy) || 
                    (Gt == EVASIONS) || 
                    (Gt == NON_EVASIONS)) {
            
            
            *move_list++ = Move::make<PROMOTION>(to, to - D, ROOK);
            *move_list++ = Move::make<PROMOTION>(to, to - D, BISHOP);
            *move_list++ = Move::make<PROMOTION>(to, to - D, KNIGHT);
           
        }

        return move_list;
    }

    template<Color C, GenType Gt>
    GradedMove* generate_pawn_moves(const Position &pos, GradedMove *move_list, BitBoard target) {
        BitBoard pawns_bb = pos.pieces(C, PAWN);

        BitBoard pawns_on_7 = pawns_bb & (C == WHITE ? RANK_7_BB : RANK_2_BB);
        BitBoard pawns_not_on_7 = pawns_bb & ~(C == WHITE ? RANK_7_BB : RANK_2_BB);

        constexpr Direction UP = C == WHITE ? NORTH : SOUTH;
        constexpr Direction UP_RIGHT = C == WHITE ? NORTH_EAST : SOUTH_WEST;
        constexpr Direction UP_LEFT = C == WHITE ? NORTH_WEST : SOUTH_EAST;

        const BitBoard enemies = Gt == EVASIONS ? pos.get_checkers() : pos.pieces(~C);
        const BitBoard empty_squares = ~pos.pieces();


        if constexpr (Gt != CAPTURES) {
            BitBoard push_1 = shift<UP>(pawns_not_on_7) & empty_squares;
            BitBoard push_2 = shift<UP>(push_1 & (C == WHITE ? RANK_3_BB : RANK_6_BB)) & empty_squares;

            if constexpr (Gt == EVASIONS) {
                push_1 &= target;
                push_2 &= target;
            }

            while (push_1) {
                Square to = lsb(pop_lsb(push_1));
                Square from = to - UP;
                *move_list++ = Move::make<NORMAL>(to, from);
            }

            while (push_2) {
                Square to = lsb(pop_lsb(push_2));
                Square from = (to - UP) - UP;
                *move_list++ = Move::make<NORMAL>(to, from);
            }
        }

        if (Gt == CAPTURES || Gt == EVASIONS || Gt == NON_EVASIONS) {
            BitBoard right_attack = shift<UP_RIGHT>(pawns_not_on_7) & enemies;
            BitBoard left_attack = shift<UP_LEFT>(pawns_not_on_7) & enemies;

            while (right_attack) {
                Square to = lsb(pop_lsb(right_attack));
                *move_list++ = Move::make<NORMAL>(to, to - UP_RIGHT);
            }

            while (left_attack) {
                Square to = lsb(pop_lsb(left_attack));
                *move_list++ = Move::make<NORMAL>(to, to - UP_LEFT);
            }

            if (pos.can_en_passant()) {
                assert(rank_of(pos.get_ep_square()) == (C == WHITE ? RANK_6 : RANK_3));

                Square to = pos.get_ep_square();
                BitBoard ep_pawns = pawn_attacks_bb(~C, to) & pawns_not_on_7;

                while (ep_pawns) {
                    Square from = lsb(pop_lsb(ep_pawns));
                    *move_list++ = Move::make<ENPASSANT>(to, from);
                }                
            }
        }

        if (pawns_on_7 != 0) {
            BitBoard right_attack = shift<UP_RIGHT>(pawns_on_7) & enemies;
            BitBoard left_attack = shift<UP_LEFT>(pawns_on_7) & enemies;
            BitBoard up_push = shift<UP>(pawns_on_7) & empty_squares;

            if constexpr (Gt == EVASIONS)
                up_push &= target;
            
            while (right_attack)
                move_list = generate_promotions<Gt, UP_RIGHT, true>(move_list, lsb(pop_lsb(right_attack)));

            while (left_attack)
                move_list = generate_promotions<Gt, UP_LEFT, true>(move_list, lsb(pop_lsb(left_attack)));

            while (up_push)
                move_list = generate_promotions<Gt, UP, false>(move_list, lsb(pop_lsb(up_push)));

        }

        return move_list;
    }

    template<Color C, PieceType Pt>
    GradedMove* generate_piece_moves(const Position &pos, GradedMove *move_list, BitBoard target) {
        static_assert(Pt != PAWN && Pt != KING, "Error: Pawn and King moves are not supported by generate_moves.");

        BitBoard bb = pos.pieces(C, Pt);

        while (bb) {
            Square from = lsb(pop_lsb(bb));
            BitBoard moves_bb = attacks_bb(from, Pt, pos.pieces()) & target;

            while (moves_bb) {
                Square to = lsb(pop_lsb(moves_bb));
                *move_list++ = Move::make<NORMAL>(to, from);
            }
        }
        
        return move_list;        
    }


    template<Color Us, GenType Gt>
    GradedMove* generate_moves(const Position &pos, GradedMove *move_list) {
        Color them = ~Us;

        Square king_sq = lsb(pos.pieces(Us, KING));
        BitBoard target = 0;

        if (popcount(pos.get_checkers()) < 2) {
            // Set target
            if constexpr (Gt == EVASIONS) {
                Square checker_sq = lsb(pos.get_checkers());
                PieceType checker_pt = type_of(pos.piece_at(checker_sq));

                if (checker_pt == PAWN || checker_pt == KNIGHT)
                    target = square_to_bb(checker_sq);
                else if (checker_pt == BISHOP || checker_pt == ROOK || checker_pt == QUEEN)
                    target = attacks_bb(king_sq, QUEEN, square_to_bb(checker_sq)) & get_ray(king_sq, checker_sq);
                else {
                    std::cerr << "Error: Unknown checker piece type in evasion generation." << std::endl;
                    return nullptr;
                }

            } else if constexpr (Gt == CAPTURES) {
                target = pos.pieces(them);

            } else if constexpr (Gt == NON_EVASIONS) {
                target = ~pos.pieces(Us);

            } else if constexpr (Gt == QUIETS) {
                target = ~pos.pieces();

            } else {
                std::cerr << "Error: Unknown generation type." << std::endl;
                return nullptr;
            }

            move_list = generate_pawn_moves<Us, Gt>(pos, move_list, target);
            move_list = generate_piece_moves<Us, KNIGHT>(pos, move_list, target);
            move_list = generate_piece_moves<Us, BISHOP>(pos, move_list, target);
            move_list = generate_piece_moves<Us, ROOK>(pos, move_list, target);
            move_list = generate_piece_moves<Us, QUEEN>(pos, move_list, target);
        }

        BitBoard king_moves_bb = attacks_bb(king_sq, KING) & (Gt == EVASIONS ? ~pos.pieces(Us) : target);
        while (king_moves_bb)
            *move_list++ = Move::make<NORMAL>(lsb(pop_lsb(king_moves_bb)), king_sq);

        if ((Gt == QUIETS || Gt == NON_EVASIONS) && pos.can_castle(Us & ANY_CASTLING))
            for (CastlingRights cr : {Us & KING_SIDE, Us & QUEEN_SIDE})
                if (!pos.castling_blocked(cr) && pos.can_castle(cr))
                    *move_list++ = Move::make<CASTLING>(castling_king_to(cr), king_sq);
        
        return move_list;
    }

    template<GenType Gt>
    GradedMove* generate(const Position &pos, GradedMove *move_list) {
        static_assert(Gt != LEGAL, "Error: Legal moves generation is not supported right now.");
        assert((Gt == EVASIONS) == bool(pos.get_checkers()));

        Color us = pos.get_side_to_move();

        return us == WHITE ? generate_moves<WHITE, Gt>(pos, move_list) : generate_moves<BLACK, Gt>(pos, move_list);
    }

    template GradedMove* generate<CAPTURES>(const Position&, GradedMove*);
    template GradedMove* generate<QUIETS>(const Position&, GradedMove*);
    template GradedMove* generate<EVASIONS>(const Position&, GradedMove*);
    template GradedMove* generate<NON_EVASIONS>(const Position&, GradedMove*);

    template<>
    GradedMove* generate<LEGAL>(const Position &pos, GradedMove *move_list) {
        Color us = pos.get_side_to_move();

        Square king_sq = lsb(pos.pieces(us, KING));
        BitBoard pinned = pos.get_blockers(us) & pos.pieces(us);

        GradedMove *curr = move_list;

        move_list = pos.is_in_check() ? generate<EVASIONS>(pos, move_list) : generate<NON_EVASIONS>(pos, move_list);

        while (curr != move_list)
            if (((curr->from_sq() & pinned) || (curr->from_sq() == king_sq) || (curr->type_of() == ENPASSANT)) && !pos.legal(*curr))
                *curr = *(--move_list);
            else
                curr++;

        return move_list;
    }

} // namespace Juujfish