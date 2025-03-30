#ifndef TYPES_H
    #define TYPES_H

    #include <random>
    #include <cstdint>

    namespace Juujfish {

        using BitBoard = std::uint64_t;
        using Value = int;

        enum Square : int {
            SQ_NONE = -1,
            SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
            SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
            SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
            SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
            SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
            SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
            SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
            SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
            SQUARE_NB
        };

        constexpr bool is_square(Square s) { return s >= SQ_A1 && s < SQUARE_NB; }

        enum File : int {
            FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
            FILE_NB
        };

        enum Rank : int {
            RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
            RANK_NB
        };

        constexpr File file_of(Square s) {return File(int(s) & 7);}
        constexpr Rank rank_of(Square s) {return Rank(int(s) >> 3);}
        constexpr Square square_of(File f, Rank r) {return Square((r << 3) + f);}

        enum Direction : int {
            NO_DIRECTION = 0,

            NORTH   = 8,
            EAST    = 1,
            SOUTH   = -8,
            WEST    = -1,

            NORTH_EAST   = NORTH + EAST,
            NORTH_WEST   = NORTH + WEST,
            SOUTH_EAST   = SOUTH + EAST,
            SOUTH_WEST   = SOUTH + WEST,

            DIRECTIONS_NB = 8
        };

        constexpr Direction get_direction(Square dest, Square src) {
            int file_diff = file_of(dest) - file_of(src);
            int rank_diff = rank_of(dest) - rank_of(src);

            if (file_diff == 0) return (rank_diff > 0) ? NORTH : SOUTH;
            if (rank_diff == 0) return (file_diff > 0) ? EAST : WEST;
            if (file_diff == rank_diff) return (file_diff > 0) ? NORTH_EAST : SOUTH_WEST;
            if (file_diff == -rank_diff) return (file_diff > 0) ? SOUTH_EAST : NORTH_WEST;

            return NO_DIRECTION;
        }

        constexpr Direction operator+(Direction d1, Direction d2) { return Direction(int(d1) + int(d2)); }
        constexpr Direction operator-(Direction d1, Direction d2) { return Direction(int(d1) - int(d2)); }

        constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
        constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
        inline Square& operator+=(Square& s, Direction d) { return s = s + d; }
        inline Square& operator-=(Square& s, Direction d) { return s = s - d; }
        inline Square& operator++(Square& s) { return s = Square(int(s) + 1); }
        inline Square& operator--(Square& s) { return s = Square(int(s) - 1); }


        enum Color {
            WHITE,
            BLACK,
            COLOR_NB
        };

        enum CastlingRights {
            NO_CASTLING,
            WHITE_OO    = 1,
            WHITE_OOO   = WHITE_OO << 1,
            BLACK_OO    = WHITE_OO << 2,
            BLACK_OOO   = WHITE_OO << 3,

            KING_SIDE       = WHITE_OO | BLACK_OO,
            QUEEN_SIDE      = WHITE_OOO | BLACK_OOO,
            WHITE_CASTLING  = WHITE_OO | WHITE_OOO,
            BLACK_CASTLING  = BLACK_OO | BLACK_OOO,
            ANY_CASTLING    = WHITE_CASTLING | BLACK_CASTLING,
        };

        constexpr CastlingRights operator|(CastlingRights cr1, CastlingRights cr2) {
            return CastlingRights(int(cr1) | int(cr2));
        }
        constexpr CastlingRights operator&(CastlingRights cr1, CastlingRights cr2) {
            return CastlingRights(int(cr1) & int(cr2));
        }
        constexpr CastlingRights operator^(CastlingRights cr1, CastlingRights cr2) {
            return CastlingRights(int(cr1) ^ int(cr2));
        }
        constexpr CastlingRights operator~(CastlingRights cr) {
            return CastlingRights(~int(cr));
        }
        inline CastlingRights& operator|=(CastlingRights& cr1, CastlingRights cr2) {
            return cr1 = cr1 | cr2;
        }
        inline CastlingRights& operator&=(CastlingRights& cr1, CastlingRights cr2) {
            return cr1 = cr1 & cr2;
        }
        inline CastlingRights& operator^=(CastlingRights& cr1, CastlingRights cr2) {
            return cr1 = cr1 ^ cr2;
        }
        inline CastlingRights operator&(Color c, CastlingRights cr) {
            return CastlingRights((c == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr);
        }

        constexpr Value PAWN_VALUE      = 100;
        constexpr Value KNIGHT_VALUE    = 310;
        constexpr Value BISHOP_VALUE    = 320;
        constexpr Value ROOK_VALUE      = 500;
        constexpr Value QUEEN_VALUE     = 900;
        constexpr Value KING_VALUE      = 20000;

        constexpr Value VALUE_ZERO = 0;
        constexpr Value VALUE_DRAW = 0;
        constexpr Value VALUE_MATE = 32000;
        constexpr Value VALUE_INFINITE = 32001;

        enum PieceType {
            NO_PIECE_TYPE = -1, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, 
            PIECE_TYPE_NB
        };

        enum Piece {
            NO_PIECE = -1,
            W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
            B_PAWN = 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
            PIECE_NB
        };

        constexpr Piece piece_of(Color c, PieceType pt) {return Piece(int(c) << 3 | int(pt));}
        constexpr Color color_of(Piece p) { return Color(int(p) >> 3); }
        constexpr PieceType type_of(Piece p) { return PieceType(int(p) & 7); }

        using Depth = int;

        enum MoveType {
            NORMAL,
            ENPASSANT   = 1 << 14,
            PROMOTION   = 2 << 14,
            CASTLING    = 3 << 14,
        };
        
        class Move {
            public:
                Move() = default;
                constexpr explicit Move(std::uint16_t d) : data(d) {}

                template<MoveType Mt>
                static constexpr Move make(Square to, Square from, PieceType pt = KNIGHT) {
                        return Move(Mt + ((pt - KNIGHT) << 12) + (from << 6) + to);
                }

                constexpr Square from_sq() const { return Square((data >> 6) & 0x3F); }
                constexpr Square to_sq() const { return Square(data & 0x3F); }
                constexpr PieceType promotion_type() const { return PieceType((data >> 12 & 0x3) + KNIGHT); }
                constexpr MoveType type_of() const { return MoveType(data & 0xC000); }

                constexpr bool operator==(const Move& m) const { return data == m.data; }
                constexpr bool operator!=(const Move& m) const { return data != m.data; }

                constexpr explicit operator bool() const { return data != 0; }
                
                constexpr std::uint16_t raw() const { return data; }

            protected:
                std::uint16_t data;

        }; // class Move

        using Key = std::uint64_t;

        inline Key random_key() {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
            return dist(gen);
        }

    } // namespace Juujfish

#endif  // #ifndef TYPES_H