#ifndef SEARCH_H
#define SEARCH_H

#include "bitboard.h"
#include "evaluation.h"
#include "heuristic.h"
#include "movegen.h"
#include "moveorder.h"
#include "position.h"
#include "transposition.h"
#include "thread.h"
#include "types.h"

namespace Juujfish {

// Forward declarations
class TranspositionTable;
class ThreadPool;

enum NodeType : uint8_t { RootNode, PVNode, CutNode };

#if 0
namespace Search {
class Worker {
 public:
  Worker() = default;
  Worker(Worker& w) = delete;

  void init(TranspositionTable* tt, int write, int rewrite);
  void clear();
  void start_searching();
  Value iterative_deepening(Position& pos);

  template <bool RootNode>
  Value search(Position& pos, Value alpha, Value beta, uint8_t depth);

  inline Move get_best_move() { return best_move; }
  inline int get_transpositions() const { return transpositions; }
  inline int get_prunes() const { return prunes; }
  inline int get_table_prunes() const { return tt_prunes; }
  inline int get_nodes() const { return nodes; }

  inline int get_tt_wrote() const { return tt_wrote; }
  inline int get_tt_rewrote() const { return tt_rewrote; }

  inline uint8_t get_searched_depth() const { return searched_depth; }
  inline void set_searched_depth(uint8_t d) { searched_depth = d; }

 private:
  Position root_pos;

  uint8_t searched_depth;
  Move best_move;

  KillerHeuristic killer;
  HistoryHeuristic history;
  ButterflyHeuristic butterfly;

  TranspositionTable* tt = nullptr;

  // Diagnostics:
  int transpositions = 0;
  int prunes = 0;
  int tt_prunes = 0;
  int nodes = 0;
  int tt_wrote = 0;
  int tt_rewrote = 0;
};
}  // namespace Search
#else

struct RootMove {
  Move move;

  Value score;
  Depth searched_depth;
};

using RootMoves = std::vector<RootMove>;



namespace Search {

class Worker {
 public:
  Worker(ThreadPool& tp, size_t thread_id)
      : thread_pool(tp), _thread_id(thread_id) {}
  Worker(Worker& w) = delete;
  
  void clear();

  bool is_mainthread() const { return _thread_id == 0; }

  void start_searching();

 private:
  Value iterative_deepening();

  template <NodeType Nt>
  Value search(Position& pos, Value alpha, Value beta, uint8_t depth);

  template <NodeType Nt>
  Value qsearch(Position& pos, Value alpha, Value beta);

  Position	root_pos;
  StateInfo root_state;
  Depth		root_depth;
  RootMoves root_moves;

  // Pricipal variation
  Move pv[MAX_MOVES];

  // Transposition table
  TranspositionTable* tt;

  // Move ordering heuristics
  KillerHeuristic		killer;
  HistoryHeuristic		history;
  ButterflyHeuristic	butterfly;

  // Stats
  std::atomic<std::uint64_t> nodes;

  // Threads
  ThreadPool& thread_pool;
  size_t _thread_id;

  friend class Juujfish::ThreadPool;
};

}  // namespace Search

#endif
}  // namespace Juujfish

#endif  // SEARCH_H
