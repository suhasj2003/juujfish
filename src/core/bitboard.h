#ifndef BITBOARD_H
    #define BITBOARD_H

    #include <cassert>
    #include <cmath>
    #include <vector>
    #include <iostream>

    #include "types.h"
    

    namespace Juujfish {

        namespace BitBoards {
            void init();
        } // namespace Juujfish::BitBoards

        constexpr BitBoard FILE_A_BB = 0x0101010101010101ULL;
        constexpr BitBoard FILE_B_BB = 0x0202020202020202ULL;
        constexpr BitBoard FILE_C_BB = 0x0404040404040404ULL;
        constexpr BitBoard FILE_D_BB = 0x0808080808080808ULL;
        constexpr BitBoard FILE_E_BB = 0x1010101010101010ULL;
        constexpr BitBoard FILE_F_BB = 0x2020202020202020ULL;
        constexpr BitBoard FILE_G_BB = 0x4040404040404040ULL;
        constexpr BitBoard FILE_H_BB = 0x8080808080808080ULL;

        constexpr BitBoard RANK_1_BB = 0x00000000000000FFULL;
        constexpr BitBoard RANK_2_BB = 0x000000000000FF00ULL;
        constexpr BitBoard RANK_3_BB = 0x0000000000FF0000ULL;
        constexpr BitBoard RANK_4_BB = 0x00000000FF000000ULL;
        constexpr BitBoard RANK_5_BB = 0x000000FF00000000ULL;
        constexpr BitBoard RANK_6_BB = 0x0000FF0000000000ULL;
        constexpr BitBoard RANK_7_BB = 0x00FF000000000000ULL;
        constexpr BitBoard RANK_8_BB = 0xFF00000000000000ULL;

        extern BitBoard PawnPushes[COLOR_NB][SQUARE_NB];
        extern BitBoard PawnAttacks[COLOR_NB][SQUARE_NB];
        extern BitBoard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];

        extern BitBoard SlidingAttacks[0x1A480];

        extern BitBoard RaysBB[SQUARE_NB][DIRECTIONS_NB];

        constexpr BitBoard square_to_bb(Square s) {
            assert(is_square(s));
            return 1ULL << s;
        }

        constexpr BitBoard file_to_bb(File f) {
            return FILE_A_BB << f;
        }

        constexpr BitBoard rank_to_bb(Rank r) {
            return RANK_1_BB << (r * 8);
        }

        constexpr int direction_to_index(Direction d) {
            switch (d) {
                case NORTH:
                    return 0;
                case EAST:
                    return 1;
                case WEST:
                    return 2;
                case SOUTH:
                    return 3;
                case NORTH_EAST:
                    return 4;
                case NORTH_WEST:
                    return 5;
                case SOUTH_EAST:
                    return 6;
                case SOUTH_WEST:
                    return 7;
                default:
                    return -1;
            }
        }

        constexpr BitBoard get_ray(Square src, Square dest) {
            Direction d = get_direction(dest, src);
            int idx = direction_to_index(d);
            assert(idx >= 0 && idx <= 7);
            return RaysBB[src][idx];
        }

        constexpr BitBoard get_ray(Square src, Direction d) {
            int idx = direction_to_index(d);
            return RaysBB[src][idx];
        }

        inline BitBoard operator &(BitBoard b, Square s) { return BitBoard(b & square_to_bb(s)); }
        inline BitBoard operator |(BitBoard b, Square s) { return BitBoard(b | square_to_bb(s)); }
        inline BitBoard operator ^(BitBoard b, Square s) { return BitBoard(b ^ square_to_bb(s)); }
        inline BitBoard& operator &=(BitBoard& b, Square s) { return b &= square_to_bb(s); }
        inline BitBoard& operator |=(BitBoard& b, Square s) { return b |= square_to_bb(s); }
        inline BitBoard& operator ^=(BitBoard& b, Square s) { return b ^= square_to_bb(s); }

        inline BitBoard operator &(Square s, BitBoard b) { return BitBoard(square_to_bb(s) & b); }
        inline BitBoard operator |(Square s, BitBoard b) { return BitBoard(square_to_bb(s) | b); }
        inline BitBoard operator ^(Square s, BitBoard b) { return BitBoard(square_to_bb(s) ^ b); }

        constexpr BitBoard file_bb(File f) { return FILE_A_BB << f; }
        constexpr BitBoard file_bb(Square s) { return FILE_A_BB << file_of(s); }
        constexpr BitBoard rank_bb(Rank r) { return RANK_1_BB << (r * 8); }
        constexpr BitBoard rank_bb(Square s) { return RANK_1_BB << (rank_of(s) * 8); }

        inline int manhattan_distance(Square s1, Square s2) {return abs(file_of(s1) - file_of(s2)) + abs(rank_of(s1) - rank_of(s2));}

        constexpr BitBoard shift(BitBoard b, Direction d) {
            return d == NORTH         ? (b & ~RANK_8_BB) << 8
                 : d == SOUTH         ? (b & ~RANK_1_BB) >> 8
                 : d == NORTH + NORTH ? (b & ~(RANK_7_BB | RANK_8_BB)) << 16
                 : d == SOUTH + SOUTH ? (b & ~(RANK_1_BB | RANK_2_BB)) >> 16
                 : d == EAST          ? (b & ~FILE_H_BB) << 1
                 : d == WEST          ? (b & ~FILE_A_BB) >> 1
                 : d == NORTH_EAST    ? (b & ~(FILE_H_BB | RANK_8_BB)) << 9
                 : d == NORTH_WEST    ? (b & ~(FILE_A_BB | RANK_8_BB)) << 7
                 : d == SOUTH_EAST    ? (b & ~(FILE_H_BB | RANK_1_BB)) >> 7
                 : d == SOUTH_WEST    ? (b & ~(FILE_A_BB | RANK_1_BB)) >> 9
                                      : 0;
        }

        // Magic bitboards:
        struct Magic {
            BitBoard mask;
            BitBoard magic;
            int shift;
            BitBoard* attacks;

            unsigned index(BitBoard occupied) const {return ((occupied & mask) * magic) >> (64 - shift);}
            BitBoard attacks_bb(BitBoard occupied) {return attacks[index(occupied)];}
        };

        extern Magic Magics[SQUARE_NB][2];

        void init_magics();
        Magic find_magic(Square s, PieceType pt, int sliding_attacks_index);
        bool try_magic(Magic& magic, std::vector<BitBoard>& blocker_bitboards, std::vector<BitBoard>& attacks_vec);

        BitBoard generate_sliding_attacks(Square s, PieceType pt, BitBoard occupied);
        BitBoard generate_movement_mask(Square s, PieceType pt);
        std::vector<BitBoard> generate_blocker_bitboards(Square s, PieceType pt);

        inline BitBoard random_bb() {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
            return dist(gen);
        }

        inline BitBoard random_magic_candidate() {
            return random_bb() & random_bb() & random_bb();
        }

        inline int popcount(BitBoard b) {
            return __builtin_popcountll(b); //gcc
        }

        inline Square lsb(BitBoard b) {
            return Square(__builtin_ctzll(b)); //gcc
        }


        inline BitBoard pop_lsb(BitBoard& b) {
            BitBoard lsb = b & -b;
            b ^= lsb;
            return lsb;
        }

        // Pawn Moves lookup:
        inline BitBoard pawn_pushes_bb(BitBoard b, Color c) {
            BitBoard pushesbb = c == WHITE ? shift(b, NORTH) : shift(b, SOUTH);
            if (rank_of(lsb(b)) == (c == WHITE ? RANK_2 : RANK_7))
                pushesbb |= c == WHITE ? shift(shift(b, NORTH), NORTH) : shift(shift(b, SOUTH), SOUTH);
            return pushesbb;
        }

        inline BitBoard pawn_pushes_bb(Square s, Color c) {
            assert(is_square(s));
            return PawnPushes[c][s];
        }

        inline BitBoard pawn_attacks_bb(BitBoard b, Color c) {
            return c == WHITE ? shift(b, NORTH_EAST) | shift(b, NORTH_WEST)
                              : shift(b, SOUTH_EAST) | shift(b, SOUTH_WEST);
        }

        inline BitBoard pawn_attacks_bb(Square s, Color c) {
            assert(is_square(s));
            return PawnAttacks[c][s];
        }

        // Attacks BitBoards lookup:
        inline BitBoard attacks_bb(Square s, PieceType pt) {
            assert(is_square(s) && pt != PAWN);
            return PseudoAttacks[pt][s];
        }

        inline BitBoard attacks_bb(Square s, PieceType pt, BitBoard occupied) {
            assert(is_square(s) && pt != PAWN);
            switch (pt) {
                case BISHOP:
                    return Magics[s][pt - BISHOP].attacks_bb(occupied);
                case ROOK:
                    return Magics[s][pt - BISHOP].attacks_bb(occupied);
                case QUEEN:
                    return attacks_bb(s, BISHOP, occupied) | attacks_bb(s, ROOK, occupied);
                default:
                    return PseudoAttacks[pt][s];
            }
        }
    } // namespace Juujfish

#endif // #ifndef BITBOARD_H