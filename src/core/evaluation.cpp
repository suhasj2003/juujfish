#include <cmath>

#include "evaluation.h"

namespace Juujfish {

    Phase get_game_phase(Position &pos) {
    int major_minor_cnt = popcount(pos.pieces(KNIGHT) | pos.pieces(BISHOP) | pos.pieces(ROOK) | pos.pieces(QUEEN));

        if (major_minor_cnt <= 6) {
            return ENDGAME;
        } else if (major_minor_cnt <= 10) {
            return MIDDLEGAME;
        } else {
            for (Color c : {WHITE, BLACK}) {
                BitBoard backrank = c == WHITE ? RANK_1_BB : RANK_8_BB;
                backrank &= ~(pos.pieces(c, ROOK) | pos.pieces(c, KING));
                if (!(backrank)) return MIDDLEGAME;
            }
            return OPENING;
        }
    }


    Value evaluate(Position &pos) {
        Phase p = get_game_phase(pos);
        Value score = p == OPENING     ? (evaluate<WHITE, OPENING>(pos) - evaluate<BLACK, OPENING>(pos)) :
                      p == MIDDLEGAME  ? (evaluate<WHITE, MIDDLEGAME>(pos) - evaluate<BLACK, MIDDLEGAME>(pos)) :
                                         (evaluate<WHITE, ENDGAME>(pos) - evaluate<BLACK, ENDGAME>(pos));

        return score + score_tempo(pos);
    }

    template<Color C, Phase P>
    Value evaluate(Position &pos) {
        return ( score_material<C, P>(pos) + 
                score_king_safety<C, P>(pos) + 
                score_pawns<C, P>(pos) );
    }

    template<Color C, Phase P>
    Value score_material(Position &pos) {
        Value score = 0;
        double center_phase_multiplier = 0;
        double phase_multiplier = 0;

        BitBoard pawns = pos.pieces(C, PAWN);
        BitBoard knights = pos.pieces(C, KNIGHT);
        BitBoard bishops = pos.pieces(C, BISHOP);
        BitBoard rooks = pos.pieces(C, ROOK);
        BitBoard queens = pos.pieces(C, QUEEN);

        Square king_sq = lsb(pos.pieces(C, KING));

        BitBoard occ = pos.pieces();

        score += (int) (0.8 * PAWN_VALUE * popcount(pawns));
        score += (int) (0.6 * KNIGHT_VALUE * popcount(knights));
        score += (int) (0.6 * BISHOP_VALUE * popcount(bishops));
        score += (int) (0.6 * ROOK_VALUE * popcount(rooks));
        score += (int) (0.6 * QUEEN_VALUE * popcount(queens));
        score += KING_VALUE;

        // Pawns:
        center_phase_multiplier = (P == OPENING) ? 2 : ((P == MIDDLEGAME) ? 1 : 0);

        while (pawns) {
            Square pawn_sq = lsb(pop_lsb(pawns));
            BitBoard pawn_attack = pawn_attacks_bb(C, pawn_sq);

            score += (int) (0.1 * PAWN_VALUE * popcount(pawn_attack));

            score += center_phase_multiplier * OUTER_CENTER_BONUS * popcount(OUTER_CENTER & pawn_sq);
            score += center_phase_multiplier * INNER_CENTER_BONUS * popcount(INNER_CENTER & pawn_sq);
        }

        // Knights:
        center_phase_multiplier = (P == OPENING) ? 2 : ((P == MIDDLEGAME) ? 1 : 0);

        while (knights) {
            Square knight_sq = lsb(pop_lsb(knights));
            BitBoard knight_attack = attacks_bb(knight_sq, KNIGHT);

            score += (int) (0.07 * KNIGHT_VALUE * popcount(knight_attack));
            score += center_phase_multiplier * OUTER_CENTER_BONUS * popcount(OUTER_CENTER & knight_sq);
            score += center_phase_multiplier * INNER_CENTER_BONUS * popcount(INNER_CENTER & knight_attack);
        }


        // Bishops:
        center_phase_multiplier = (P == OPENING) ? 2 : ((P == MIDDLEGAME) ? 1 : 0);

        while (bishops) {
            Square bishop_sq = lsb(pop_lsb(bishops));
            BitBoard bishop_attack = attacks_bb(bishop_sq, BISHOP, occ);

            for (Direction d : {NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST}) {
                BitBoard bishop_attack_ray = bishop_attack & get_ray(bishop_sq, d);
                int attack_squares_nb = popcount(bishop_attack_ray);
                
                score += (int) (0.05 * BISHOP_VALUE * attack_squares_nb);
                score += (int) (0.01 * BISHOP_VALUE * std::max(0, attack_squares_nb - 3));
            }
            score += (OUTER_CENTER & bishop_sq) ? center_phase_multiplier * OUTER_CENTER_BONUS : 0;
        }

        // Rooks;
        while (rooks) {
            Square rook_sq = lsb(pop_lsb(rooks));
            BitBoard rook_attack = attacks_bb(rook_sq, ROOK);

            score += (int) 0.05 * ROOK_VALUE * popcount(rook_attack);
            score += ROOKS_CONNECTED_BONUS * popcount(rook_attack & rooks);
            score += pos.pieces(PAWN) & file_of(rook_sq) ? 0 : OPEN_FILE_BONUS; 
        }


        // Queens:
        phase_multiplier = (P == OPENING) ? 0.01 : ((P == MIDDLEGAME) ? 0.7 : 1.5);

        while (queens) {
            Square queen_sq = lsb(pop_lsb(queens));
            BitBoard queen_attack = attacks_bb(queen_sq, ROOK);

            score += (int) (phase_multiplier * 0.01 * QUEEN_VALUE * popcount(queen_attack));
        }


        // King:
        phase_multiplier = (P == OPENING) ? 0 : ((P == MIDDLEGAME) ? 0.5 : 3);

        score += phase_multiplier * popcount(attacks_bb(king_sq, KING));
        score += phase_multiplier * OUTER_CENTER_BONUS * popcount(OUTER_CENTER & king_sq);

        return score;
    }

    template<Color C, Phase P>
    Value score_king_safety(Position &pos) {
        Square king_sq = lsb(pos.pieces(C, KING));
        const BitBoard king_zone = attacks_bb(king_sq, KING);

        BitBoard pawn_shield_zone = king_zone & rank_bb(Rank(rank_of(king_sq) + (C == WHITE ? 1 : -1)));

        int phase_multiplier = (P == OPENING) ? 1 : ((P == MIDDLEGAME) ? 2 : 0);

        int pawn_shield = phase_multiplier * (-50 + 20 * popcount(pawn_shield_zone & pos.pieces(C, PAWN)));
        int open_file = (pos.pieces(PAWN) & file_bb(king_sq)) ? 0 : -OPEN_FILE_BONUS;
        int def_atk_squares = 3 * pos.count_attacks(C, king_zone) - pos.count_attacks(~C, king_zone);

        return pawn_shield + open_file + def_atk_squares;
    }

    template<Color C, Phase P>
    Value score_pawns(Position &pos) {
        BitBoard pawns = pos.pieces(C, PAWN);
        BitBoard opp_pawns = pos.pieces(~C, PAWN);
        BitBoard pawns_temp = pawns;
        Value score = 0;

        constexpr Direction UP_RIGHT = C == WHITE ? NORTH_EAST : SOUTH_WEST;
        constexpr Direction UP_LEFT = C == WHITE ? NORTH_WEST : SOUTH_EAST;

        BitBoard pawns_attack = shift<UP_RIGHT>(pawns) | shift<UP_LEFT>(pawns);

        score += 5 * popcount(pawns_attack & pawns);

        while (pawns_temp) {
            BitBoard pawn_bb = pop_lsb(pawns_temp);
            Square pawn_sq = lsb(pawn_bb);
            BitBoard nearby_files_bb = file_bb(pawn_sq);

            nearby_files_bb |= shift<EAST>(nearby_files_bb);
            nearby_files_bb |= shift<WEST>(nearby_files_bb);

            // Isolated pawn penalty
            score += (int) nearby_files_bb & ~pawn_bb ? 0 : -0.4 * PAWN_VALUE;

            // Isolate files to the squares above the pawn
            nearby_files_bb &= ~(rank_bb(pawn_sq) | (
                                    C == WHITE ? BitBoard(((1ULL << pawn_sq) - 1)) : ~BitBoard((1ULL << pawn_sq) - 1)
                                ));

            score += popcount(pawns & nearby_files_bb) + 1 > 
                     popcount(opp_pawns & nearby_files_bb) ?
                     PASS_PAWN_BONUS : 0;
        }

        return score;
    }
} // namespace Juujfish