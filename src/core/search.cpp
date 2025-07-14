#include "search.h"

namespace Juujfish {

void Search::Worker::init(TranspositionTable* tt, int write, int rewrite) {
  best_move = Move(0);

  killer.init();
  history.init();
  butterfly.init();

  this->tt = tt;
  this->tt->new_search();

  tt_wrote = write;
  tt_rewrote = rewrite;
}

Value Search::Worker::iterative_deepening() {
  Value score = 0;
  Depth d;
  for (d = 1; d <= searched_depth; d++) {
    score = search<true>(pos, -VALUE_INFINITE, VALUE_INFINITE, d);
  }
  return score;
}

template <bool RootNode>
Value Search::Worker::search(Position& pos, Value alpha, Value beta,
                             uint8_t depth) {
  Value best_score = -VALUE_INFINITE;
  Value score = 0;

  Move best_move = Move::null_move();

  bool moves_exist = false;

  Color us = pos.get_side_to_move();
  int ply = searched_depth - depth;

  nodes++;

  if (pos.is_draw()) {
    return VALUE_DRAW;
  }

  if (depth == 0)
    return pos.get_side_to_move() == WHITE ? evaluate(pos) : -evaluate(pos);

  auto [table_hit, table_data, table_writer] =
      tt->probe(pos.get_key(), pos.generate_secondary_key());

  bool rewrote = !table_hit && table_data.depth;

  if (table_hit)
    transpositions++;

  if (table_hit && table_data.depth >= depth &&
      (table_data.score >= beta || table_data.score <= alpha)) {
    prunes++;
    tt_prunes++;
    return table_data.score;
  }

  StateInfo new_st;
  memset(&new_st, 0, sizeof(new_st));

  MoveOrderer mo(pos, table_hit ? table_data.move : Move::null_move(), ply,
                 &killer, &history, &butterfly);
  Move m = mo.next();

  while (!m.is_nullmove()) {
    if (pos.legal(m)) {
      moves_exist = true;

      pos.make_move(m, &new_st, pos.gives_check(m));
      score = -search<false>(pos, -beta, -alpha, depth - 1);
      pos.unmake_move();

      if (score > best_score) {
        best_score = score;
        best_move = m;
        if (RootNode)
          this->best_move = m;
      }

      butterfly.update(m, us, depth);

      alpha = std::max(alpha, score);

      if (score >= beta) {
        table_writer.write(pos.get_key(), pos.generate_secondary_key(), depth,
                           BOUND_LOWER, score, m);

        killer.update(m, ply);
        history.update(m, us, type_of(pos.piece_at(m.from_sq())), depth);

        prunes++;

        if (rewrote)
          tt_rewrote += 1;
        else
          tt_wrote += table_hit ? 0 : 1;
        return best_score;
      }
    }
    m = mo.next();
  }

  if (!moves_exist) {
    if (pos.is_in_check())
      return -(VALUE_MATE - (get_searched_depth() - depth));
    else
      return VALUE_DRAW;
  } else {
    if (best_score <= alpha)
      table_writer.write(pos.get_key(), pos.generate_secondary_key(), depth,
                         BOUND_UPPER, best_score, best_move);
    else
      table_writer.write(pos.get_key(), pos.generate_secondary_key(), depth,
                         BOUND_EXACT, best_score, best_move);
  }

  if (rewrote)
    tt_rewrote += 1;
  else
    tt_wrote += table_hit ? 0 : 1;

  return best_score;
}

template Value Search::Worker::search<true>(Position& pos, Value alpha,
                                            Value beta, uint8_t depth);
template Value Search::Worker::search<false>(Position& pos, Value alpha,
                                             Value beta, uint8_t depth);

}  // namespace Juujfish