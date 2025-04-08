#ifndef POSITION_H
    #define POSITION_H

    #include "types.h"
    #include "bitboard.h"

    #include <iostream>

    namespace Juujfish {

        struct StateInfo {
            Key zobrist_key;

            Key pawn_key;
            Key minor_piece_key;
            Key major_piece_key;

            int fifty_move_counter;
            int plies_from_start;
            int repetition;
            
            CastlingRights castling_rights;
            bool can_ep;
            Square ep_square;

            bool in_check;
            BitBoard check_squares[PIECE_TYPE_NB];
            BitBoard checkersBB;

            BitBoard blockers[COLOR_NB]; // Pieces blocking sliding attacks to the king
            BitBoard pinners[COLOR_NB]; // Pinners to the king

            Color side_to_move;
            Move previous_move;
            Piece captured_piece;

            struct StateInfo *prev;
            struct StateInfo *next;
        };

        extern BitBoard CastlingPaths[4];
        extern BitBoard CastlingRookSquares[4];

        inline void copy_state(StateInfo *dest, StateInfo *src) {
            std::memcpy(dest, src, sizeof(struct StateInfo));   
        }

        class Position {
            public:
                static void init();

                Position() = default;
                Position(const Position&) = delete;
                Position& operator=(const Position&) = delete;

                void set(const std::string& fen, StateInfo *new_st);
                // void set(const Position& pos);

                constexpr StateInfo*        get_state() const {return st;}
                constexpr Key               get_key() const {return st->zobrist_key;}
                constexpr Key               get_pawn_key() const {return st->pawn_key;}
                constexpr Key               get_minor_piece_key() const {return st->minor_piece_key;}
                constexpr Key               get_major_piece_key() const {return st->major_piece_key;}
                constexpr int               get_fifty_move_counter() const {return st->fifty_move_counter;}
                constexpr int               get_plies_from_start() const {return st->plies_from_start;}
                constexpr Color             get_side_to_move() const {return st->side_to_move;}
                constexpr CastlingRights    get_castling_rights() const {return st->castling_rights;}
                constexpr bool              is_in_check() const {return st->in_check;}
                constexpr BitBoard          get_checkers() const {return st->checkersBB;}
                constexpr BitBoard          get_blockers(Color c) const {return st->blockers[c];}
                constexpr BitBoard          get_pinners(Color c) const {return st->pinners[c];}
                constexpr bool              can_en_passant() const {return st->can_ep;}
                constexpr Square            get_ep_square() const {return st->ep_square;}
                constexpr BitBoard          get_check_squares(PieceType pt) const {return st->check_squares[pt];}

                void copy(const Position& pos);

                inline bool can_castle(CastlingRights cr) const { return (st->castling_rights & cr) != 0; }

                bool castling_blocked(CastlingRights cr) const;
                bool castling_attacked(CastlingRights cr) const;

                bool is_occupied(Square s) const;
                bool is_occupied(Square s, Color c) const;

                BitBoard pieces(PieceType pt = PIECE_TYPE_NB) const;
                BitBoard pieces(Color c, PieceType pt = PIECE_TYPE_NB) const;

                Piece piece_at(Square s) const;

                bool set_piece(Color c, PieceType pt, Square s);
                bool remove_piece(Square s);

                bool legal(Move m) const;
                bool gives_check(Move m) const;

                bool is_capture(Move m) const;

                void make_move(Move m, StateInfo *new_st, bool gives_check);
                void do_castling(Color c, Square to, Square from, Square rto, Square rfrom);
                void do_en_passant(Color c, Square to, Square from, Square capture_sq);
                void do_promotion(Color c, Square to, Square from, PieceType promotion_type);

                void unmake_move();
                void undo_castling(Color c, Square to, Square from, Square rto, Square rfrom);
                void undo_en_passant(Color c, Square to, Square from, Square capture_sq);
                void undo_promotion(Color c, Square to, Square from);

                BitBoard all_attacks(Color c) const;
                bool sq_is_attacked(Color c, Square s, BitBoard occupied) const;
                BitBoard attacked_by(Color c, Square s) const;
                
                template<PieceType Pt>
                BitBoard attacks_by(Color c) const;

                void set_check_squares();

                void update_checkers();
                void update_pinners_blockers();

                bool is_draw() const;
                bool is_repetition() const;
                bool update_repetition();

            private:
                BitBoard pieceBB[COLOR_NB][PIECE_TYPE_NB];
                BitBoard colorBB[COLOR_NB];
                BitBoard boardBB;
                StateInfo* st;
        };

        inline Piece char_to_piece(char c) {
            switch (c) {
                case 'P': return W_PAWN;
                case 'N': return W_KNIGHT;
                case 'B': return W_BISHOP;
                case 'R': return W_ROOK;
                case 'Q': return W_QUEEN;
                case 'K': return W_KING;
                case 'p': return B_PAWN;
                case 'n': return B_KNIGHT;
                case 'b': return B_BISHOP;
                case 'r': return B_ROOK;
                case 'q': return B_QUEEN;
                case 'k': return B_KING;
                default:  return NO_PIECE;
            }
        }

        inline bool Position::is_occupied(Square s) const {
            return boardBB & square_to_bb(s);
        }

        inline bool Position::is_occupied(Square s, Color c) const {
            return colorBB[c] & square_to_bb(s);
        }

        inline BitBoard Position::pieces(PieceType pt) const {
           if (pt == PIECE_TYPE_NB) {
                return boardBB;
            }
            return pieceBB[WHITE][pt] | pieceBB[BLACK][pt];
        }

        inline BitBoard Position::pieces(Color c, PieceType pt) const {
            if (pt == PIECE_TYPE_NB) {
                return colorBB[c];
            }
            return pieceBB[c][pt];
        }

        inline Square castling_king_to(CastlingRights cr) {
            switch (cr) {
                case WHITE_OO:  return SQ_G1;
                case WHITE_OOO: return SQ_C1;
                case BLACK_OO:  return SQ_G8;
                case BLACK_OOO: return SQ_C8;
                default:        return SQ_NONE;
            }
        }

        inline Piece Position::piece_at(Square s) const {
            assert(is_square(s));
            for (int c = WHITE; c < COLOR_NB; ++c) {
                for (int pt = PAWN; pt < PIECE_TYPE_NB; ++pt) {
                    if (pieceBB[c][pt] & square_to_bb(s)) {
                        return piece_of(Color(c), PieceType(pt));
                    }
                }
            }
            return NO_PIECE;
        }

        inline bool Position::set_piece(Color c, PieceType pt, Square s) {
            assert(is_square(s));
            if (!is_occupied(s)) {
                BitBoard bb = square_to_bb(s);
                pieceBB[c][pt] |= bb;
                colorBB[c] |= bb;
                boardBB |= bb;
                return true;
            } else return false;
        }

        inline bool Position::remove_piece(Square s) {
            assert(is_square(s));
            if (is_occupied(s)) {
                BitBoard bb = square_to_bb(s);
                Piece p = piece_at(s);
                Color c = color_of(p);
                PieceType pt = type_of(p);
                pieceBB[c][pt] ^= bb;
                colorBB[c] ^= bb;
                boardBB ^= bb;
                return true;
            } else return false;
        }
    } // namespace Juujfish

#endif // #ifndef POSITION_H