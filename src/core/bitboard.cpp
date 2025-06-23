#include <iostream>
#include <vector>

#include "bitboard.h"

namespace Juujfish {

BitBoard PawnPushes[COLOR_NB][SQUARE_NB];
BitBoard PawnAttacks[COLOR_NB][SQUARE_NB];
BitBoard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];

BitBoard SlidingAttacks[0x1A480];

BitBoard RaysBB[SQUARE_NB][DIRECTIONS_NB];

alignas(64) Magic Magics[SQUARE_NB][2];

void BitBoards::init() {

  init_magics();

  for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1) {
    PawnPushes[WHITE][s1] = pawn_pushes_bb(WHITE, square_to_bb(s1));
    PawnPushes[BLACK][s1] = pawn_pushes_bb(BLACK, square_to_bb(s1));

    PawnAttacks[WHITE][s1] = pawn_attacks_bb(WHITE, square_to_bb(s1));
    PawnAttacks[BLACK][s1] = pawn_attacks_bb(BLACK, square_to_bb(s1));

    for (int step : {-9, -8, -7, -1, 1, 7, 8, 9}) {
      if (is_square(Square(s1 + step)) &&
          manhattan_distance(s1, Square(s1 + step)) <= 2)
        PseudoAttacks[KING][s1] |= square_to_bb(Square(s1 + step));
    }

    for (int step : {-17, -15, -10, -6, 6, 10, 15, 17}) {
      if (is_square(Square(s1 + step)) &&
          manhattan_distance(s1, Square(s1 + step)) <= 3)
        PseudoAttacks[KNIGHT][s1] |= square_to_bb(Square(s1 + step));
    }

    PseudoAttacks[QUEEN][s1] = PseudoAttacks[BISHOP][s1] =
        attacks_bb(s1, BISHOP, 0);
    PseudoAttacks[QUEEN][s1] |= PseudoAttacks[ROOK][s1] =
        attacks_bb(s1, ROOK, 0);

    for (Direction d : {NORTH, EAST, WEST, SOUTH, NORTH_EAST, NORTH_WEST,
                        SOUTH_EAST, SOUTH_WEST}) {
      BitBoard ray = square_to_bb(s1);
      Square to = s1;
      while (is_square(to + d) && manhattan_distance(to, to + d) <= 2) {
        ray |= (to + d);
        to += d;
      }

      RaysBB[s1][direction_to_index(d)] = ray;
    }
  }
}

void init_magics() {
  int sliding_attacks_index = 0;
  for (Square s = SQ_A1; s <= SQ_H8; ++s) {
    struct Magic m = find_magic(s, BISHOP, sliding_attacks_index);
    Magics[s][BISHOP - BISHOP] = m;
    sliding_attacks_index += (1 << m.shift);
  }
  for (Square s = SQ_A1; s <= SQ_H8; ++s) {
    struct Magic m = find_magic(s, ROOK, sliding_attacks_index);
    Magics[s][ROOK - BISHOP] = m;
    sliding_attacks_index += (1 << m.shift);
  }
}

Magic find_magic(Square s, PieceType pt, int sliding_attacks_index) {

  std::vector<BitBoard> blocker_bitboards = generate_blocker_bitboards(s, pt);
  std::vector<BitBoard> attacks_vec;

  BitBoard mask = generate_movement_mask(s, pt);

  for (BitBoard bb : blocker_bitboards) {
    attacks_vec.push_back(generate_sliding_attacks(s, pt, bb));
  }

  int shift = popcount(mask);

  struct Magic magic;
  magic.mask = mask;
  magic.shift = shift;
  magic.attacks = SlidingAttacks + sliding_attacks_index;

  while (true) {
    magic.magic = random_magic_candidate();
    if (try_magic(magic, blocker_bitboards, attacks_vec))
      break;
  }
  return magic;
}

bool try_magic(Magic& magic, std::vector<BitBoard>& blocker_bitboards,
               std::vector<BitBoard>& attacks_vec) {
  std::fill(magic.attacks, magic.attacks + (1 << magic.shift), 0);
  for (size_t idx = 0; idx < blocker_bitboards.size(); ++idx) {
    BitBoard bb = blocker_bitboards[idx];
    BitBoard attack = attacks_vec[idx];
    unsigned index = magic.index(bb);
    if (magic.attacks[index] != 0 && magic.attacks[index] != attack) {
      return false;
    }
    magic.attacks[index] = attack;
  }
  return true;
}

BitBoard generate_sliding_attacks(Square s, PieceType pt, BitBoard occupied) {
  assert(is_square(s));
  BitBoard mask = 0;

  Direction RookDirections[4] = {NORTH, EAST, SOUTH, WEST};
  Direction BishopDirections[4] = {NORTH_EAST, NORTH_WEST, SOUTH_EAST,
                                   SOUTH_WEST};

  for (Direction d : (pt == BISHOP ? BishopDirections : RookDirections)) {
    Square to = s;
    while (is_square(to + d) && manhattan_distance(to, to + d) <= 2) {
      mask |= (to + d);
      if (occupied & (to + d))
        break;
      to += d;
    }
  }
  return mask;
}

BitBoard generate_movement_mask(Square s, PieceType pt) {
  BitBoard movement_mask = generate_sliding_attacks(s, pt, 0);
  if (pt == BISHOP) {
    movement_mask &= ~((FILE_A_BB | FILE_H_BB) | (RANK_1_BB | RANK_8_BB));
  } else if (pt == ROOK) {
    File f = file_of(s);
    Rank r = rank_of(s);
    movement_mask &= ~(1ULL << f) & ~(1ULL << (56 + f)) & ~(1ULL << 8 * r) &
                     ~(1ULL << (7 + 8 * r));
  }
  return movement_mask;
}

std::vector<BitBoard> generate_blocker_bitboards(Square s, PieceType pt) {
  BitBoard movement_mask = generate_movement_mask(s, pt);

  std::vector<uint64_t> blocker_bitboards;

  std::vector<int> bit_positions;
  for (int i = 0; i < 64; i++) {
    if (movement_mask & (1ULL << i)) {
      bit_positions.push_back(i);
    }
  }

  int num_bits = bit_positions.size();
  int num_combinations = 1 << num_bits;

  for (int i = 0; i < num_combinations; i++) {
    uint64_t blocker = 0;
    for (int j = 0; j < num_bits; j++) {
      if (i & (1 << j)) {
        blocker |= (1ULL << bit_positions[j]);
      }
    }
    blocker_bitboards.push_back(blocker);
  }

  return blocker_bitboards;
}
}  // namespace Juujfish