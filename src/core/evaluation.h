#ifndef EVALUATION_H
    #define EVALUATION_H

    #include "types.h"
    #include "bitboard.h"
    #include "position.h"
    #include "movegen.h"

    namespace Juujfish {

        enum Phase {
            OPENING,
            MIDDLEGAME,
            ENDGAME
        };

        constexpr BitBoard INNER_CENTER = 103481868288ULL;
        constexpr BitBoard OUTER_CENTER = 66229406269440ULL;

        constexpr Value ACTIVITY_WEIGHT = 2;
        constexpr Value KING_SAFETY_WEIGHT = 5;
        constexpr Value INNER_CENTER_BONUS = 10;
        constexpr Value OUTER_CENTER_BONUS = 5;
        constexpr Value ROOKS_CONNECTED_BONUS = 20;
        constexpr Value OPEN_FILE_BONUS = 20;
        constexpr Value PASS_PAWN_BONUS = 30;

        Phase get_game_phase(Position &pos);  
        
        template<Color C, Phase P>
        Value score_material(Position &pos);

        template <Color C, Phase P, PieceType Pt>
        Value score_material(Position &pos);

        template<Color C, Phase P>
        Value score_king_safety(Position &pos);

        template<Color C, Phase P>
        Value score_pawns(Position &pos);

        inline Value score_tempo(Position &pos) { return MoveList<LEGAL>(pos).size(); }

        template<Color C, Phase P>
        Value evaluate(Position &pos);

        Value evaluate(Position &pos);
            

    } // namespace Juujfish

#endif // ifndef EVALUATION_H