#include "search.h"

#include "thread.h"

namespace Juujfish {

#if 0

Value Search::Worker::iterative_deepening(Position &pos) {
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

#else

void Search::Worker::clear() {
  nodes = 0;

  killer.clear();
  history.clear();
  butterfly.clear();
}

Move Search::Worker::get_best_move() const {
  return pv[0];
}

void Search::Worker::start_searching() {
  if (!is_mainthread()) {
    iterative_deepening();
    return;
  }

  tt->new_search();
  thread_pool.start_searching();

  iterative_deepening();

  thread_pool.stop = true;
  thread_pool.wait_for_all_threads();

  std::sort(root_moves.begin(), root_moves.end());  // Potentially not necessary

  /*
      For now, take the strict 0-th thread's PV as the final result.
      In the future, we want to consider the deepest, minimum score when comparing moves in root_moves.
    */

  get_best_move();  // Temporary, eventually return this to GUI
}

void Search::Worker::iterative_deepening() {
  Value alpha, beta;
  Value score, prev_score;
  Value delta;

  Depth completed_depth = 0;

  Move prev_pv[MAX_MOVES];
  Move prev_move;

  while (++root_depth < MAX_PLY && !thread_pool.stop) {

    if (is_mainthread() && root_depth > 14) // Arbitrary depth limit for main thread
      thread_pool.stop = true;

    delta = 5 + root_moves[0].mean_score_squared / 10000;
    prev_score = root_moves[0].score;
    alpha = std::clamp(prev_score - delta, -VALUE_INFINITE, VALUE_INFINITE);
    beta = std::clamp(prev_score + delta, -VALUE_INFINITE, VALUE_INFINITE);

    prev_move = root_moves[0].move;
    copy_pv(prev_pv, pv);

    while (true) {
      score = Search::Worker::search<RootNode>(root_pos, alpha, beta,
                                               root_depth, false);
      if (thread_pool.stop)
        break;

      if (score <= alpha)
        alpha = std::max(alpha - delta, -VALUE_INFINITE);
      else if (score >= beta)
        beta = std::min(beta + delta, VALUE_INFINITE);
      else
        break;

      delta += delta / 3;
    }

    if (thread_pool.stop) {
      root_depth = completed_depth;
      copy_pv(pv, prev_pv);

      // Moves the previous best move to the front of the list
      RootMoves::iterator it = std::find_if(
          root_moves.begin(), root_moves.end(),
          [prev_move](const RootMove& rm) { return rm.move == prev_move; });

      if (it != root_moves.end())
        std::rotate(root_moves.begin(), it, it + 1);

      root_moves[0].score = prev_score;

    } else if (!thread_pool.stop) {
      completed_depth = root_depth;

      std::sort(root_moves.begin(), root_moves.end(), std::greater<RootMove>());

      assert(root_moves[0].move == pv[0]);

      root_moves[0].score = score;

      root_moves[0].mean_score_squared =
          (root_moves[0].mean_score_squared * (root_depth - 1) +
           score * score) /
          root_depth;
    }
  }
}

template <NodeType Nt>
Value Search::Worker::search(Position& pos, Value alpha, Value beta,
                             Depth depth, bool cut_node) {

  // STEP 1: Intial Declearations and Node setup
  constexpr bool pv_node = (Nt != NonPV);
  constexpr bool root_node = (Nt == RootNode);

  Value best_score = -VALUE_INFINITE;
  Value score = 0;

  Move best_move = Move::null_move();

  Color us = pos.get_side_to_move();

  int ply = root_depth - depth;

  // STEP 2: Check thread_pool.stop and for draw by repetition or 50-move rule
  if (thread_pool.stop.load(std::memory_order_relaxed) || pos.is_draw())
    return VALUE_DRAW;

  // STEP 3: Transposition Lookup
  auto [table_hit, table_data, table_writer] =
      tt->probe(pos.get_key(), pos.generate_secondary_key());

  if (!root_node && table_hit && table_data.depth >= depth && (table_data.score >= beta)) {

    if (pos.get_fifty_move_counter() < 90)
      return table_data.score;
  }

  // STEP 4: Evaluate if depth == 0
  if (depth == 0)
    return std::abs(evaluate(pos));

  // Start Moves Loop
  MoveOrderer mo(pos, table_hit ? table_data.move : Move::null_move(),
                 root_depth - depth, &killer, &history, &butterfly);

  Move curr_move;

  StateInfo new_st;
  memset(&new_st, 0, sizeof(new_st));

  int move_count = 0;
  while (!(curr_move = mo.next()).is_nullmove()) {

    if (!pos.legal(curr_move))
      continue;

    // STEP 5: Make Move and update move_count
    pos.make_move(curr_move, &new_st, pos.gives_check(curr_move));
    move_count++;

    // STEP 6: Null Window Search
    if (!root_node && (!pv_node || move_count > 1)) {
      score = -search<NonPV>(pos, -(alpha + 1), -alpha, depth - 1, !cut_node);
    }

    // STEP 7: Full Window Search if necessary
    if (pv_node && (score > alpha || move_count == 1)) {
      score = -search<PV>(pos, -beta, -alpha, depth - 1, false);
    }

    // STEP 8: Unmake Move and Update best_score, best_move, alpha, and heuristics
    pos.unmake_move();
    nodes++;

    // Exit loop if search is stopped
    if (thread_pool.stop.load(std::memory_order_relaxed))
      return VALUE_DRAW;

    if (root_node) {

      RootMove& rm =
          *std::find(root_moves.begin(), root_moves.end(), curr_move);

      rm.score = score;

      rm.mean_score_squared =
          (rm.mean_score_squared * (root_depth - 1) + score * score) /
          root_depth;
    }

    if (score > best_score) {
      best_score = score;
      best_move = curr_move;
    }

    butterfly.update(curr_move, us, depth);

    alpha = std::max(alpha, score);

    if (score >= beta) {
      killer.update(curr_move, ply);
      history.update(curr_move, us, type_of(pos.piece_at(curr_move.from_sq())),
                     depth);
      break;
    }
  }  // End Moves Loop

  // STEP 9: Handle No Moves Case (Checkmate or Stalemate)
  if (move_count == 0) {
    if (pos.is_in_check())
      best_score = -(VALUE_MATE - (root_depth - depth));
    else
      best_score = VALUE_DRAW;
  }

  // STEP 10: Store in Transposition Table
  if (!root_node) {
    table_writer.write(pos.get_key(), pos.generate_secondary_key(), depth,
                       best_score <= alpha
                           ? BOUND_UPPER
                           : (best_score >= beta ? BOUND_LOWER : BOUND_EXACT),
                       best_score, best_move);
  }

  // STEP 11: Update PV
  if (!best_move.is_nullmove() && pv_node && best_score >= alpha)  {
    pv[ply] = best_move;
  }

  return best_score;
}

void Search::Worker::copy_pv(Move* dest, const Move* src) {
  for (int i = 0; i < MAX_MOVES; i++)
    dest[i] = src[i];
}

#endif

}  // namespace Juujfish