#include <iostream>

#include "moveorder.h"

namespace Juujfish {

void partial_insertion_sort(GradedMove* begin, GradedMove* end, int threshold) {
  for (GradedMove *sorted_idx = begin, *i = begin + 1; i < end; ++i) {
    if (i->value > threshold) {
      GradedMove temp = *i, *j;
      *i = *++sorted_idx;
      for (j = sorted_idx; j != begin && *(j - 1) < temp; --j)
        *j = *(j - 1);
      *j = temp;
    }
  }
}

inline int mvv_lva(const PieceType victim, const PieceType attacker) {
  assert(victim >= PAWN && attacker >= PAWN);
  assert(victim <= KING && attacker <= KING);
  return PieceValue[victim] - PieceValue[attacker];
}

template <typename Pred>
Move MoveOrderer::select(Pred filter) {
  for (; curr != end_moves; ++curr)
    if (filter())
      return *curr++;
  return Move::null_move();
}

template <GenType Gt>
void MoveOrderer::score() {
  static_assert(Gt == CAPTURES || Gt == QUIETS || Gt == EVASIONS,
                "Error: Invalid GenType in score().");

  Color us = pos.get_side_to_move();

  BitBoard threatened_by_pawn = pos.attacks_by<PAWN>(~us);
  BitBoard threatened_by_minor = pos.attacks_by<BISHOP>(~us) |
                                 pos.attacks_by<KNIGHT>(~us) |
                                 threatened_by_pawn;
  BitBoard threatened_by_rook = pos.attacks_by<ROOK>(~us) | threatened_by_minor;

  BitBoard threatened_pieces =
      ((pos.pieces(us, QUEEN) & threatened_by_rook) |
       (pos.pieces(us, ROOK) & threatened_by_minor) |
       ((pos.pieces(us, BISHOP) | pos.pieces(us, KNIGHT)) &
        threatened_by_pawn));

  for (auto& m : *this) {

    Square to = m.to_sq(), from = m.from_sq();
    PieceType promo_type = m.promotion_type();
    MoveType mt = m.type_of();

    Piece p = pos.piece_at(from);
    PieceType pt = type_of(p);

    if constexpr (Gt == CAPTURES) {
      Piece capture_piece = pos.piece_at(to);
      PieceType capture_piece_type = type_of(capture_piece);

      if (mt == ENPASSANT) {
        m.value = mvv_lva(PAWN, PAWN);
      } else if (mt == PROMOTION) {
        assert(capture_piece == NO_PIECE || (capture_piece_type >= PAWN && capture_piece_type <= KING));
        m.value = mvv_lva(promo_type, PAWN) + (capture_piece != NO_PIECE
                      ? PieceValue[capture_piece_type]
                      : 0);
      } else if (mt == CASTLING) {
        std::cerr << "Error: Castling move in capture list." << std::endl;
        m.value = 0;
      } else {
        m.value = mvv_lva(capture_piece_type, pt);
      }

    } else if constexpr (Gt == QUIETS) {
      int value = killers->lookup(m, ply) +
                  (history->lookup(m, us, pt) / butterfly->lookup(m, us));

      value += pos.get_check_squares(pt) & to ? 10 : 0;

      value -= ((threatened_by_rook & to) && (pt == QUEEN))   ? 20
               : ((threatened_by_minor & to) && (pt == ROOK)) ? 10
               : ((threatened_by_pawn & to) && (pt == BISHOP || pt == KNIGHT))
                   ? 5
                   : 0;

      value += !(threatened_pieces & from)                   ? 0
               : (pt == QUEEN && !(threatened_by_rook & to)) ? 30
               : (pt == ROOK && !(threatened_by_minor & to)) ? 15
               : ((pt == BISHOP || pt == KNIGHT) && !(threatened_by_rook & to))
                   ? 7
                   : 0;

      m.value = value;

    } else if constexpr (Gt == EVASIONS) {
      if (pos.is_capture(m)) {
        if (type_of(pos.piece_at(to)) > KING)
          std::cout << "Error: Invalid piece on square. " << type_of(pos.piece_at(to)) << std::endl;
        assert(type_of(pos.piece_at(to)) >= PAWN);
        assert(type_of(pos.piece_at(to)) <= KING);
      }
        
      if (pos.is_capture(m))
        m.value = PieceValue[type_of(pos.piece_at(to))] + (1 << 20);
      else
        m.value = killers->lookup(m, ply) +
                  (history->lookup(m, us, pt) / butterfly->lookup(m, us));
    }
  }
}

Move MoveOrderer::next() {

  auto bad_quiets_threshhold = 0;

top:
  switch (stage) {
    case GENERAL_TT:
    case EVASION_TT:
      ++stage;
      if (!table_move.is_nullmove())
        return table_move;
      else
        goto top;

    case CAPTURES_GEN:
      curr = end_bad_captures = moves;
      end_moves = generate<CAPTURES>(pos, curr);

      score<CAPTURES>();
      partial_insertion_sort(curr, end_moves, std::numeric_limits<int>::min());

      ++stage;
      [[fallthrough]];

    case CAPTURE:
      if (select([&]() {
            return curr->value >= 0 ? true
                                    : (*end_bad_captures++ = *curr, false);
          }))
        return *(curr - 1);

      ++stage;
      [[fallthrough]];

    case QUIETS_GEN:
      if (!skip_quiets) {
        curr = end_bad_captures;
        end_moves = begin_bad_quiets = end_bad_quiets =
            generate<QUIETS>(pos, curr);

        score<QUIETS>();
        partial_insertion_sort(curr, end_moves, bad_quiets_threshhold);
      }

      ++stage;
      [[fallthrough]];

    case QUIET:
      // Starts with killers and then goes to the rest of them.
      if (!skip_quiets && select([]() { return true; })) {
        if ((curr - 1)->value > bad_quiets_threshhold)
          return *(curr - 1);

        begin_bad_quiets = curr - 1;
      }

      // Prepare pointers for bad captures
      curr = moves;
      end_moves = end_bad_captures;

      ++stage;
      [[fallthrough]];

    case BAD_CAPTURE:
      if (select([]() { return true; })) {
        return *(curr - 1);
      }

      // Prepare pointers for bad quiets
      curr = begin_bad_quiets;
      end_moves = end_bad_quiets;

      ++stage;
      [[fallthrough]];

    case BAD_QUIET:
      if (!skip_quiets)
        return select([]() { return true; });

      return Move::null_move();

    // EVASIONS:
    case EVASION_GEN:
      curr = moves;
      end_moves = generate<EVASIONS>(pos, curr);

      score<EVASIONS>();
      partial_insertion_sort(curr, end_moves, std::numeric_limits<int>::min());

      ++stage;
      [[fallthrough]];

    case EVASION:
      return select([]() { return true; });
  }

  std::cerr << "Error: Got to end of MoveOrderer." << std::endl;
  return Move::null_move();
}

}  // namespace Juujfish