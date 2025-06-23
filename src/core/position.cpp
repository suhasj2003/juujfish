#include "position.h"
#include "misc.h"

#include <iostream>

namespace Juujfish {
namespace Zobrist {
Key psq[PIECE_NB][SQUARE_NB];
Key black_to_move;
Key castling_rights[4];
Key ep_file[FILE_NB];
}  // namespace Zobrist

BitBoard CastlingPaths[4];

void Position::init() {
  for (int i = 0; i < PIECE_NB; ++i) {
    for (int j = 0; j < SQUARE_NB; ++j) {
      Zobrist::psq[i][j] = random_key();
    }
  }
  Zobrist::black_to_move = random_key();
  for (int i = 0; i < 4; ++i) {
    Zobrist::castling_rights[i] = random_key();
  }
  for (int i = 0; i < FILE_NB; ++i) {
    Zobrist::ep_file[i] = random_key();
  }
  CastlingPaths[0] = square_to_bb(SQ_F1) | square_to_bb(SQ_G1);
  CastlingPaths[1] =
      square_to_bb(SQ_B1) | square_to_bb(SQ_C1) | square_to_bb(SQ_D1);
  CastlingPaths[2] = square_to_bb(SQ_F8) | square_to_bb(SQ_G8);
  ;
  CastlingPaths[3] =
      square_to_bb(SQ_B8) | square_to_bb(SQ_C8) | square_to_bb(SQ_D8);
}

std::string Position::fen() const {
  std::string fen_str;

  // 1. Pieces
  for (int r = RANK_8; r >= RANK_1; --r) {
    int empty_count = 0;
    for (int f = FILE_A; f <= FILE_H; ++f) {
      Square sq = square_of(File(f), Rank(r));
      Piece p = piece_at(sq);
      if (p == NO_PIECE) {
        ++empty_count;
      } else {
        if (empty_count > 0) {
          fen_str += std::to_string(empty_count);
          empty_count = 0;
        }
        fen_str += piece_to_char(p);
      }
    }
    if (empty_count > 0) {
      fen_str += std::to_string(empty_count);
      empty_count = 0;
    }
    if (r > RANK_1)
      fen_str += '/';
  }

  // 2. Side to move
  fen_str += ' ';
  fen_str += (st->side_to_move == WHITE ? "w" : "b");

  // 3. Castling rights
  fen_str += ' ';
  if (st->castling_rights == NO_CASTLING) {
    fen_str += '-';
  } else {
    if (st->castling_rights & WHITE_OO) fen_str += 'K';
    if (st->castling_rights & WHITE_OOO) fen_str += 'Q';
    if (st->castling_rights & BLACK_OO) fen_str += 'k';
    if (st->castling_rights & BLACK_OOO) fen_str += 'q';
  }

  // 4. En passant square
  fen_str += ' ';
  if (st->can_ep) {
    fen_str += file_of(st->ep_square) + 'a';
    fen_str += rank_of(st->ep_square) + '1';
  } else {
    fen_str += '-';
  }

  // 5. Halfmove clock
  fen_str += ' ';
  fen_str += std::to_string(st->fifty_move_counter);

  // 6. Fullmove number
  fen_str += ' ';
  fen_str += std::to_string((st->plies_from_start + 2) / 2);

  return fen_str;
}

void Position::set(const std::string& fen, StateInfo* new_st) {
  // 0. Reset position and read FEN
  std::memset(this, 0, sizeof(Position));
  std::memset(new_st, 0, sizeof(StateInfo));
  st = new_st;

  // 0.5 Read fen string
  std::string s = fen;
  std::string seq[6];
  std::string delimiter = " ";
  size_t pos = 0;

  if (!s.empty() && s[s.length() - 1] == '\n') {
    s.erase(s.length() - 1);
  }

  int i = 0;
  while ((pos = s.find(delimiter)) != std::string::npos && i < 6) {
    seq[i++] = s.substr(0, pos);
    s = s.substr(pos + 1, s.length());
  }
  seq[i] = s;

  // 1. Set pieces
  i = 56;
  for (char c : seq[0]) {
    if (c >= '1' && c <= '8') {
      i += c - '0';
    } else if (c == '/') {
      i -= 16;
      continue;
    } else {
      Piece p = char_to_piece(c);
      PieceType pt = type_of(p);
      bool was_piece_set = set_piece(color_of(p), pt, Square(i));
      if (was_piece_set) {
        st->zobrist_key ^= Zobrist::psq[p][i];

        switch (pt) {
          case PAWN:
            st->pawn_key ^= Zobrist::psq[p][i];
            break;
          case KNIGHT:
          case BISHOP:
            st->minor_key ^= Zobrist::psq[p][i];
            break;
          case ROOK:
          case QUEEN:
            st->major_key ^= Zobrist::psq[p][i];
          default:
            break;
        }
      }
      ++i;
    }
  }

  // 2. Set side to move
  st->side_to_move = (seq[1] == "w") ? WHITE : BLACK;
  st->zobrist_key ^= st->side_to_move == WHITE ? 0 : Zobrist::black_to_move;

  // 3. Set castling rights
  st->castling_rights = NO_CASTLING;
  if (seq[2].find('K') != std::string::npos) {
    st->castling_rights |= WHITE_OO;
    st->zobrist_key ^= Zobrist::castling_rights[0];
  }
  if (seq[2].find('Q') != std::string::npos) {
    st->castling_rights |= WHITE_OOO;
    st->zobrist_key ^= Zobrist::castling_rights[1];
  }
  if (seq[2].find('k') != std::string::npos) {
    st->castling_rights |= BLACK_OO;
    st->zobrist_key ^= Zobrist::castling_rights[2];
  }
  if (seq[2].find('q') != std::string::npos) {
    st->castling_rights |= BLACK_OOO;
    st->zobrist_key ^= Zobrist::castling_rights[3];
  }

  // 4. Set en passant square
  if (seq[3] != "-") {
    int file = seq[3][0] - 'a';
    int rank = seq[3][1] - '1';
    st->ep_square = square_of(File(file), Rank(rank));
    st->can_ep = true;
    st->zobrist_key ^= Zobrist::ep_file[file];
  } else {
    st->can_ep = false;
    st->ep_square = SQ_NONE;
  }

  // 5. Set halfmove clock and fullmove number
  st->fifty_move_counter = std::stoi(seq[4]);
  st->plies_from_start =
      (2 * std::stoi(seq[5]) - 1) + st->side_to_move == WHITE ? 0 : 1;

  // 6. Set checkers and pinned/blocker pieces
  set_check_squares();
  update_checkers();
  st->in_check = st->checkersBB != 0;
  update_pinners_blockers();
}

void Position::copy(const Position& pos) {
  boardBB = pos.boardBB;
  for (Color c : {WHITE, BLACK}) {
    colorBB[c] = pos.colorBB[c];
    for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
      pieceBB[c][pt] = pos.pieceBB[c][pt];
    }
  }
  StateInfo new_st;
  copy_state(&new_st, pos.st);

  st = &new_st;
}

bool Position::legal(Move m) const {
  Color us = get_side_to_move();
  Color them = ~us;

  Square from = m.from_sq();
  Square to = m.to_sq();
  MoveType mt = m.type_of();

  Piece p = piece_at(from);
  PieceType pt = type_of(p);

  Square king_sq = lsb(pieces(us, KING));

  assert(p != NO_PIECE);

  if (mt == CASTLING) {
    CastlingRights cr = to > from ? us & KING_SIDE : us & QUEEN_SIDE;
    if (castling_attacked(cr) || castling_blocked(cr))
      return false;
    else
      return true;

  } else if (mt == ENPASSANT) {
    Square capture_sq = Square(to + (us == WHITE ? -8 : 8));
    BitBoard occupied = (pieces() ^ from ^ capture_sq) | to;

    return (((attacks_bb(king_sq, ROOK, occupied) &
              (pieces(them, ROOK) | pieces(them, QUEEN))) == 0) &&
            ((attacks_bb(king_sq, BISHOP, occupied) &
              (pieces(them, BISHOP) | pieces(them, QUEEN))) == 0));

  } else if (pt == KING)
    return !sq_is_attacked(us, to, pieces() ^ from);

  return !(get_blockers(us) & from) || ((get_ray(king_sq, from) & to) != 0);
}

bool Position::gives_check(Move m) const {
  Square from = m.from_sq();
  Square to = m.to_sq();
  PieceType promotion_type = m.promotion_type();
  MoveType mt = m.type_of();

  Piece p = piece_at(from);
  PieceType pt = type_of(p);

  Color us = get_side_to_move();
  Color them = ~us;

  Square king_sq = lsb(pieces(them, KING));

  assert(color_of(p) == us);

  if (get_check_squares(pt) & to)
    return true;

  if (get_blockers(them) & from) {
    return !(get_ray(king_sq, from) & to) || mt == CASTLING;
  }

  BitBoard occ = 0;
  Square rto;

  Square capture_sq;

  switch (mt) {
    case NORMAL:
      return false;
    case ENPASSANT:
      capture_sq = square_of(file_of(to), rank_of(from));
      occ = ((pieces() ^ from) ^ capture_sq) | to;
      return ((attacks_bb(king_sq, ROOK, occ) &
               (pieces(us, ROOK) | pieces(us, QUEEN))) != 0) ||
             ((attacks_bb(king_sq, BISHOP, occ) &
               (pieces(us, BISHOP) | pieces(us, QUEEN))) != 0);
    case PROMOTION:
      return ((attacks_bb(to, promotion_type, pieces() ^ from ^ to)) &
              pieces(them, KING)) != 0;
    case CASTLING:
      rto = us == WHITE ? (to > from ? SQ_F1 : SQ_D1)
                        : (to > from ? SQ_F8 : SQ_D8);
      return (get_check_squares(ROOK) & rto) != 0;
    default:
      std::cerr << "Error: Unknown movetype in gives_check." << std::endl;
      return false;
  }
}

/* This is for testing if Transposition Table move is valid or if it is a collision with another
       position. */
// bool Position::pseudo_legal(Move m) const {}

bool Position::is_capture(Move m) const {
  Square to = m.to_sq();
  PieceType promo_type = m.promotion_type();
  MoveType mt = m.type_of();

  Color cpc = color_of(piece_at(to));

  if (mt == ENPASSANT) {
    return true;
  } else if (mt == CASTLING) {
    return false;
  } else if (mt == PROMOTION) {
    return (promo_type == QUEEN);
  } else {
    return piece_at(to) != NO_PIECE && cpc != get_side_to_move();
  }
}

bool Position::castling_blocked(CastlingRights cr) const {
  if (cr == NO_CASTLING)
    return false;
  BitBoard path;

  switch (cr) {
    case WHITE_OO:
      path = CastlingPaths[0];
      break;
    case WHITE_OOO:
      path = CastlingPaths[1];
      break;
    case BLACK_OO:
      path = CastlingPaths[2];
      break;
    case BLACK_OOO:
      path = CastlingPaths[3];
      break;
    default:
      std::cerr << "Error: Invalid castling rights" << std::endl;
      return false;
  }
  return (path & boardBB) != 0;
}

bool Position::castling_attacked(CastlingRights cr) const {
  if (cr == NO_CASTLING)
    return false;
  BitBoard occ = pieces();

  Color us = st->side_to_move;

  switch (cr) {
    case WHITE_OO:
      return (sq_is_attacked(us, SQ_E1, occ) ||
              sq_is_attacked(us, SQ_F1, occ) || sq_is_attacked(us, SQ_G1, occ));
    case WHITE_OOO:
      return (sq_is_attacked(us, SQ_C1, occ) ||
              sq_is_attacked(us, SQ_D1, occ) || sq_is_attacked(us, SQ_E1, occ));
    case BLACK_OO:
      return (sq_is_attacked(us, SQ_E8, occ) ||
              sq_is_attacked(us, SQ_F8, occ) || sq_is_attacked(us, SQ_G8, occ));
    case BLACK_OOO:
      return (sq_is_attacked(us, SQ_C8, occ) ||
              sq_is_attacked(us, SQ_D8, occ) || sq_is_attacked(us, SQ_E8, occ));
    default:
      std::cerr << "Error: Invalid castling rights" << std::endl;
      return false;
  }
}

int Position::count_attacks(Color c, const BitBoard zone) const {
  int attacks = popcount(pawn_attacks_bb(c, pieces(c, PAWN)) & zone);
  BitBoard occ = pieces();

  for (PieceType pt : {QUEEN, ROOK, BISHOP, KNIGHT, KING}) {
    BitBoard bb = pieces(c, pt);

    while (bb) {
      Square s = Square(lsb(pop_lsb(bb)));
      attacks += popcount(attacks_bb(s, pt, occ) & zone);
    }
  }
  return attacks;
}

bool Position::sq_is_attacked(Color c, Square s, BitBoard occ) const {
  Color them = ~c;

  for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
    BitBoard them_piece_bb = pieces(them, pt);

    BitBoard attack_mask =
        pt == PAWN ? pawn_attacks_bb(c, s) : attacks_bb(s, pt, occ);
    if (them_piece_bb & attack_mask)
      return true;
  }
  return false;
}

BitBoard Position::attacked_by(Color c, Square s) const {
  BitBoard attack_pieces = 0ULL;
  BitBoard occ = pieces();
  for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
    BitBoard them_piece_bb = pieces(c, pt);

    BitBoard attack_mask =
        pt == PAWN ? pawn_attacks_bb(~c, s) : attacks_bb(s, pt, occ);
    attack_pieces |= them_piece_bb & attack_mask;
  }
  return attack_pieces;
}

template <PieceType Pt>
BitBoard Position::attacks_by(Color c) const {
  BitBoard piece_bb = pieces(c, Pt);
  BitBoard attacks = 0ULL;
  BitBoard occ = pieces();

  while (piece_bb) {
    Square piece_sq = lsb(pop_lsb(piece_bb));
    attacks |= Pt == PAWN ? pawn_attacks_bb(c, piece_sq)
                          : attacks_bb(piece_sq, Pt, occ);
  }
  return attacks;
}

template BitBoard Position::attacks_by<PAWN>(Color c) const;
template BitBoard Position::attacks_by<KNIGHT>(Color c) const;
template BitBoard Position::attacks_by<BISHOP>(Color c) const;
template BitBoard Position::attacks_by<ROOK>(Color c) const;
template BitBoard Position::attacks_by<QUEEN>(Color c) const;
template BitBoard Position::attacks_by<KING>(Color c) const;

void Position::update_checkers() {
  Color us = st->side_to_move;
  Color them = ~us;

  Square king_sq = lsb(pieces(us, KING));

  BitBoard checkers = attacked_by(them, king_sq);

  assert(popcount(checkers) <= 2);

  st->checkersBB = checkers;
}

void Position::set_check_squares() {
  Color them = ~st->side_to_move;

  Square king_sq = lsb(pieces(them, KING));
  BitBoard occ = pieces();

  for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
    if (pt == PAWN)
      st->check_squares[PAWN] = pawn_attacks_bb(them, king_sq);
    else if (pt == KING)
      st->check_squares[KING] = 0;
    else
      st->check_squares[pt] = attacks_bb(king_sq, pt, occ);
  }
}

void Position::update_pinners_blockers() {
  for (Color c : {WHITE, BLACK}) {
    Color us = c;
    Color them = ~us;

    BitBoard blockers_bb = 0;
    BitBoard pinners_bb = 0;

    BitBoard occ = pieces();
    BitBoard st_sliding_occ = pieces(them, ROOK) | pieces(them, QUEEN);
    BitBoard dia_sliding_occ = pieces(them, BISHOP) | pieces(them, QUEEN);

    BitBoard king_bb = pieces(us, KING);
    Square king_sq = lsb(king_bb);

    BitBoard king_rook_atk = attacks_bb(king_sq, ROOK, st_sliding_occ);
    BitBoard king_bishop_atk = attacks_bb(king_sq, BISHOP, dia_sliding_occ);

    for (Direction d : {NORTH, EAST, WEST, SOUTH}) {
      BitBoard king_ray_atk = king_rook_atk & get_ray(king_sq, d);

      BitBoard pinner_candidate_bb = king_ray_atk & st_sliding_occ;
      BitBoard blocker_candidate_bb = king_ray_atk & occ & ~pinner_candidate_bb;

      assert(popcount(pinner_candidate_bb) <= 1);
      if ((popcount(pinner_candidate_bb) == 1) &&
          (popcount(blocker_candidate_bb) == 1)) {
        pinners_bb |= pinner_candidate_bb;
        blockers_bb |= blocker_candidate_bb;
      }
    }

    for (Direction d : {NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST}) {
      BitBoard king_ray_atk = king_bishop_atk & get_ray(king_sq, d);

      BitBoard pinner_candidate_bb = king_ray_atk & dia_sliding_occ;
      BitBoard blocker_candidate_bb = king_ray_atk & occ & ~pinner_candidate_bb;

      assert(popcount(pinner_candidate_bb) <= 1);
      if ((popcount(pinner_candidate_bb) == 1) &&
          (popcount(blocker_candidate_bb) == 1)) {
        pinners_bb |= pinner_candidate_bb;
        blockers_bb |= blocker_candidate_bb;
      }
    }

    st->blockers[us] = blockers_bb;
    st->pinners[them] = pinners_bb;
  }
}

void Position::make_move(Move m, StateInfo* new_st, bool gives_check) {
  Square from = m.from_sq();
  Square to = m.to_sq();
  PieceType promotion_type = m.promotion_type();
  MoveType move_type = m.type_of();

  CastlingRights cr = NO_CASTLING;

  Piece p = piece_at(from);
  PieceType pt = type_of(p);
  Color us = st->side_to_move;
  Color them = ~us;

  Piece capture_piece = NO_PIECE;

  assert(is_square(from) && is_square(to));
  assert(p != NO_PIECE);
  assert(color_of(p) == us);

  // 1. Make move
  if (move_type == CASTLING) {
    cr = file_of(to) - file_of(from) > 0
             ? (us == WHITE ? WHITE_OO : BLACK_OO)
             : (us == WHITE ? WHITE_OOO : BLACK_OOO);

    switch (cr) {
      case WHITE_OO:
        do_castling(us, to, from, SQ_F1, SQ_H1);
        break;
      case WHITE_OOO:
        do_castling(us, to, from, SQ_D1, SQ_A1);
        break;
      case BLACK_OO:
        do_castling(us, to, from, SQ_F8, SQ_H8);
        break;
      case BLACK_OOO:
        do_castling(us, to, from, SQ_D8, SQ_A8);
        break;
      default:
        std::cerr << "Error: Castling Error." << std::endl;
        return;
    }

  } else if (move_type == ENPASSANT) {
    Square capture_sq = Square(to + (us == WHITE ? -8 : 8));
    do_en_passant(us, to, from, capture_sq);
    capture_piece = us == WHITE ? B_PAWN : W_PAWN;

  } else if (move_type == PROMOTION) {
    if (file_of(from) != file_of(to)) {
      Square capture_sq = to;

      capture_piece = piece_at(capture_sq);
      remove_piece(to);
    }
    do_promotion(us, to, from, promotion_type);

  } else if (move_type == NORMAL) {
    if (is_occupied(to, them)) {
      capture_piece = piece_at(to);
      remove_piece(to);
    }
    remove_piece(from);
    set_piece(us, pt, to);

  } else {
    std::cerr << "Error: Unknown MoveType." << std::endl;
    return;
  }

  // 2. Update State
  std::memset(new_st, 0, sizeof(StateInfo));

  StateInfo* old_st = get_state();

  copy_state(new_st, old_st);

  old_st->next = new_st;
  new_st->prev = old_st;
  new_st->next = nullptr;

  new_st->zobrist_key = old_st->zobrist_key;
  new_st->pawn_key = old_st->pawn_key;
  new_st->minor_key = old_st->minor_key;
  new_st->major_key = old_st->major_key;

  new_st->previous_move = m;
  new_st->fifty_move_counter += 1;
  new_st->plies_from_start += 1;
  new_st->captured_piece = capture_piece;
  new_st->repetition = 0;
  new_st->side_to_move = them;
  new_st->castling_rights = old_st->castling_rights;

  new_st->can_ep = false;
  new_st->ep_square = SQ_NONE;

  if (old_st->can_ep)
    new_st->zobrist_key ^= Zobrist::ep_file[file_of(old_st->ep_square)];

  st = new_st;

  set_check_squares();

  st->checkersBB = 0;
  if ((st->in_check = gives_check)) {
    update_checkers();
    assert(popcount(st->checkersBB) > 0);
  }

  update_pinners_blockers();

  if (move_type == CASTLING) {
    Square rto, rfrom;

    switch (cr) {
      case WHITE_OO:
        rto = SQ_F1;
        rfrom = SQ_H1;
        break;
      case WHITE_OOO:
        rto = SQ_D1;
        rfrom = SQ_A1;
        break;
      case BLACK_OO:
        rto = SQ_F8;
        rfrom = SQ_H8;
        break;
      case BLACK_OOO:
        rto = SQ_D8;
        rfrom = SQ_A8;
        break;
      default:
        std::cerr << "Error: Invalid Castling Rights." << std::endl;
        return;
    }

    st->zobrist_key ^= Zobrist::psq[piece_of(us, KING)][from];
    st->zobrist_key ^= Zobrist::psq[piece_of(us, KING)][to];
    st->zobrist_key ^= Zobrist::psq[piece_of(us, ROOK)][rfrom];
    st->zobrist_key ^= Zobrist::psq[piece_of(us, ROOK)][rto];

    st->major_key ^= Zobrist::psq[piece_of(us, ROOK)][rfrom];
    st->major_key ^= Zobrist::psq[piece_of(us, ROOK)][rto];

    if (st->castling_rights & (us & KING_SIDE))
      st->zobrist_key ^= (us == WHITE ? Zobrist::castling_rights[0]
                                      : Zobrist::castling_rights[2]);
    if (st->castling_rights & (us & QUEEN_SIDE))
      st->zobrist_key ^= (us == WHITE ? Zobrist::castling_rights[1]
                                      : Zobrist::castling_rights[3]);

    st->castling_rights &= ~(us & ANY_CASTLING);

  } else if (move_type == ENPASSANT) {
    Square capture_sq = Square(to + ((us == WHITE) ? -8 : 8));

    assert(type_of(p) == PAWN);
    assert(type_of(capture_piece) == PAWN);

    st->fifty_move_counter = 0;

    st->zobrist_key ^= Zobrist::psq[capture_piece][capture_sq];
    st->zobrist_key ^= Zobrist::psq[p][from];
    st->zobrist_key ^= Zobrist::psq[p][to];

    st->pawn_key ^= Zobrist::psq[capture_piece][capture_sq];
    st->pawn_key ^= Zobrist::psq[p][from];
    st->pawn_key ^= Zobrist::psq[p][to];

  } else if (move_type == PROMOTION) {
    if (capture_piece != NO_PIECE) {
      st->zobrist_key ^= Zobrist::psq[capture_piece][to];

      switch (type_of(capture_piece)) {
        case KNIGHT:
        case BISHOP:
          st->minor_key ^= Zobrist::psq[capture_piece][to];
          break;
        case ROOK:
        case QUEEN:
          st->major_key ^= Zobrist::psq[capture_piece][to];
          break;
        default:
          std::cerr << "Error: Promotion capture piece type is not defined."
                    << std::endl;
          return;
      }

      if (type_of(capture_piece) == ROOK) {
        File capture_file = file_of(to);
        Rank capture_rank = rank_of(to);

        if (capture_file == FILE_H &&
            capture_rank == (them == WHITE ? RANK_1 : RANK_8) &&
            st->castling_rights & (them & KING_SIDE)) {

          st->castling_rights &= ~(them & KING_SIDE);
          st->zobrist_key ^= Zobrist::castling_rights[them == WHITE ? 0 : 2];
        }

        if (capture_file == FILE_A &&
            capture_rank == (them == WHITE ? RANK_1 : RANK_8) &&
            st->castling_rights & (them & QUEEN_SIDE)) {

          st->castling_rights &= ~(them & QUEEN_SIDE);
          st->zobrist_key ^= Zobrist::castling_rights[them == WHITE ? 1 : 3];
        }
      }
    }

    Piece promotion_piece = piece_of(us, promotion_type);

    st->zobrist_key ^= Zobrist::psq[p][from];
    st->zobrist_key ^= Zobrist::psq[promotion_piece][to];

    switch (type_of(promotion_piece)) {
      case KNIGHT:
      case BISHOP:
        st->minor_key ^= Zobrist::psq[promotion_piece][to];
        break;
      case ROOK:
      case QUEEN:
        st->major_key ^= Zobrist::psq[promotion_piece][to];
        break;
      default:
        std::cerr << "Error: Promotion piece type is incorrect." << std::endl;
        return;
    }

  } else if (move_type == NORMAL) {

    if (type_of(p) == ROOK) {
      File rook_file = file_of(from);
      Rank rook_rank = rank_of(from);
      if (rook_rank == (us == WHITE ? RANK_1 : RANK_8)) {
        if ((rook_file == FILE_H) && (st->castling_rights & (us & KING_SIDE))) {
          st->castling_rights &= ~(us & KING_SIDE);
          st->zobrist_key ^= Zobrist::castling_rights[(us == WHITE ? 0 : 2)];

        } else if ((rook_file == FILE_A) &&
                   (st->castling_rights & (us & QUEEN_SIDE))) {
          st->castling_rights &= ~(us & QUEEN_SIDE);
          st->zobrist_key ^= Zobrist::castling_rights[(us == WHITE ? 1 : 3)];
        }
      }
    } else if (type_of(p) == KING) {
      if (st->castling_rights & (us & KING_SIDE)) {
        st->zobrist_key ^= Zobrist::castling_rights[us == WHITE ? 0 : 2];
      }

      if (st->castling_rights & (us & QUEEN_SIDE)) {
        st->zobrist_key ^= Zobrist::castling_rights[us == WHITE ? 1 : 3];
      }

      st->castling_rights &= ~(us & ANY_CASTLING);

    } else if (type_of(p) == PAWN) {
      int to_rank = rank_of(to), from_rank = rank_of(from);

      if ((to_rank - from_rank == 2 || to_rank - from_rank == -2) &&
          (from_rank == RANK_2 || from_rank == RANK_7) &&
          (pawn_attacks_bb(us,
                           square_to_bb(Square(to - (us == WHITE ? 8 : -8)))) &
           pieces(them, PAWN))) {

        File ep_file = file_of(to);
        Square ep_square = square_of(ep_file, us == WHITE ? RANK_3 : RANK_6);

        st->can_ep = true;
        st->ep_square = ep_square;

        st->zobrist_key ^= Zobrist::ep_file[ep_file];
      }

      st->fifty_move_counter = 0;
    }

    // Deals with normal captures
    if (capture_piece != NO_PIECE) {
      st->zobrist_key ^= Zobrist::psq[capture_piece][to];

      switch (type_of(capture_piece)) {
        case PAWN:
          st->pawn_key ^= Zobrist::psq[capture_piece][to];
          break;
        case KNIGHT:
        case BISHOP:
          st->minor_key ^= Zobrist::psq[capture_piece][to];
          break;
        case ROOK:
        case QUEEN:
          st->major_key ^= Zobrist::psq[capture_piece][to];
          break;
        case KING:
          std::cerr << "Error: King cannot be captured" << std::endl;
          return;
        default:
          std::cerr << "Error: Normal capture piece type is not defined."
                    << std::endl;
          return;
      }

      if (type_of(capture_piece) == ROOK) {
        File capture_file = file_of(to);
        Rank capture_rank = rank_of(to);

        if (capture_file == FILE_H &&
            capture_rank == (them == WHITE ? RANK_1 : RANK_8) &&
            st->castling_rights & (them & KING_SIDE)) {

          st->castling_rights &= ~(them & KING_SIDE);
          st->zobrist_key ^= Zobrist::castling_rights[them == WHITE ? 0 : 2];
        }

        if (capture_file == FILE_A &&
            capture_rank == (them == WHITE ? RANK_1 : RANK_8) &&
            st->castling_rights & (them & QUEEN_SIDE)) {

          st->castling_rights &= ~(them & QUEEN_SIDE);
          st->zobrist_key ^= Zobrist::castling_rights[them == WHITE ? 1 : 3];
        }
      }
    }

    st->zobrist_key ^= Zobrist::psq[p][from];
    st->zobrist_key ^= Zobrist::psq[p][to];

    switch (type_of(p)) {
      case PAWN:
        st->pawn_key ^= Zobrist::psq[p][from];
        st->pawn_key ^= Zobrist::psq[p][to];
        break;
      case KNIGHT:
      case BISHOP:
        st->minor_key ^= Zobrist::psq[p][from];
        st->minor_key ^= Zobrist::psq[p][to];
        break;
      case ROOK:
      case QUEEN:
        st->major_key ^= Zobrist::psq[p][from];
        st->major_key ^= Zobrist::psq[p][to];
        break;
      default:
        break;
    }
  }

  st->zobrist_key ^= Zobrist::black_to_move;

  if (capture_piece != NO_PIECE)
    st->fifty_move_counter = 0;

  update_repetition();
  return;
}

void Position::do_castling(Color c, Square to, Square from, Square rto,
                           Square rfrom) {
  assert(c == WHITE || c == BLACK);
  assert(is_square(to) && is_square(from) && is_square(rto) &&
         is_square(rfrom));

  assert(type_of(piece_at(from)) == KING && color_of(piece_at(from)) == c);

  assert(type_of(piece_at(rfrom)) == ROOK && color_of(piece_at(rfrom)) == c);

  remove_piece(from);
  set_piece(c, KING, to);

  remove_piece(rfrom);
  set_piece(c, ROOK, rto);
}

void Position::do_en_passant(Color c, Square to, Square from,
                             Square capture_sq) {
  assert(c == WHITE || c == BLACK);
  assert(is_square(to) && is_square(from) && is_square(capture_sq));

  assert(type_of(piece_at(from)) == PAWN && color_of(piece_at(from)) == c);
  assert(type_of(piece_at(capture_sq)) == PAWN &&
         color_of(piece_at(capture_sq)) == (~c));

  assert(capture_sq == Square(to + (c == WHITE ? -8 : 8)));

  remove_piece(from);
  set_piece(c, PAWN, to);

  remove_piece(capture_sq);
}

void Position::do_promotion(Color c, Square to, Square from,
                            PieceType promotion_type) {
  assert(c == WHITE || c == BLACK);
  assert(is_square(to) && is_square(from));
  assert(promotion_type == KNIGHT || promotion_type == BISHOP ||
         promotion_type == ROOK || promotion_type == QUEEN);

  assert(type_of(piece_at(from)) == PAWN && color_of(piece_at(from)) == c);

  remove_piece(from);
  set_piece(c, promotion_type, to);
}

void Position::unmake_move() {
  Color us = st->side_to_move;
  Color them = ~us;

  Move prev_m = st->previous_move;
  Square to = prev_m.to_sq();
  Square from = prev_m.from_sq();
  MoveType mt = prev_m.type_of();

  CastlingRights cr;
  PieceType pt = type_of(piece_at(to));
  Piece captured_piece = st->captured_piece;

  // 1. Revert state:
  if (st->prev == nullptr) {
    std::cerr << "Error: No previous states." << std::endl;
    return;
  }

  StateInfo* prev = st->prev;
  st = prev;
  st->next = nullptr;

  // 2. Reverse previous move:
  if (mt == CASTLING) {
    cr = file_of(to) - file_of(from) > 0
             ? (them == WHITE ? WHITE_OO : BLACK_OO)
             : (them == WHITE ? WHITE_OOO : BLACK_OOO);
    switch (cr) {
      case WHITE_OO:
        undo_castling(them, to, from, SQ_F1, SQ_H1);
        break;
      case WHITE_OOO:
        undo_castling(them, to, from, SQ_D1, SQ_A1);
        break;
      case BLACK_OO:
        undo_castling(them, to, from, SQ_F8, SQ_H8);
        break;
      case BLACK_OOO:
        undo_castling(them, to, from, SQ_D8, SQ_A8);
        break;
      default:
        std::cerr << "Error: Castling Error." << std::endl;
        return;
    }

  } else if (mt == ENPASSANT) {
    Square capture_sq = Square(to + ((them == WHITE) ? -8 : 8));
    undo_en_passant(them, to, from, capture_sq);

  } else if (mt == PROMOTION) {
    undo_promotion(them, to, from);

    if (captured_piece != NO_PIECE) {
      Square capture_sq = to;
      set_piece(us, type_of(captured_piece), capture_sq);
    }

  } else if (mt == NORMAL) {
    remove_piece(to);
    set_piece(them, pt, from);

    if (captured_piece != NO_PIECE)
      set_piece(us, type_of(captured_piece), to);

  } else {
    std::cerr << "Error: Unknown MoveType." << std::endl;
    return;
  }
}

void Position::undo_castling(Color c, Square to, Square from, Square rto,
                             Square rfrom) {
  assert(c == WHITE || c == BLACK);

  assert(is_square(to) && is_square(from) && is_square(rto) &&
         is_square(rfrom));
  assert(type_of(piece_at(to)) == KING && type_of(piece_at(rto)) == ROOK);

  remove_piece(to);
  set_piece(c, KING, from);

  remove_piece(rto);
  set_piece(c, ROOK, rfrom);
}

void Position::undo_en_passant(Color c, Square to, Square from,
                               Square capture_sq) {
  assert(c == WHITE || c == BLACK);
  assert(is_square(to) && is_square(from) && is_square(capture_sq));

  assert(capture_sq == Square(to + ((c == WHITE) ? -8 : 8)));

  set_piece(c, PAWN, from);
  remove_piece(to);

  set_piece(~c, PAWN, capture_sq);
}

void Position::undo_promotion(Color c, Square to, Square from) {
  assert(c == WHITE || c == BLACK);
  assert(is_square(to) && is_square(from));

  remove_piece(to);
  set_piece(c, PAWN, from);
}

bool Position::update_repetition() {
  Key new_key = st->zobrist_key;
  StateInfo* curr = st;
  int new_repetition = 0;

  int end = st->fifty_move_counter;

  if (curr && curr->prev && (curr = curr->prev->prev)) {
    for (int i = 2; i < end; i += 2) {
      if (new_key == curr->zobrist_key) {
        new_repetition = curr->repetition + 1;
        break;
      }
      if (!(curr->prev && (curr = curr->prev->prev))) {
        break;
      }
    }
  }
  st->repetition = new_repetition;
  return new_repetition != 0;
}

bool Position::is_repetition() const {
  return st->repetition != 0;
}

bool Position::is_draw() const {
  return st->fifty_move_counter >= 100 || st->repetition >= 2;
}

Key Position::recompute_zobrist() const {
  Key key = 0ULL;
  key ^= get_side_to_move() == WHITE ? 0 : Zobrist::black_to_move;
  for (Color c : {WHITE, BLACK}) {
    for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
      BitBoard bb = pieces(c, pt);
      while (bb) {
        Square sq = Square(lsb(pop_lsb(bb)));
        key ^= Zobrist::psq[piece_of(c, pt)][sq];
      }
    }
  }

  for (Color c : {WHITE, BLACK}) {
    if (st->castling_rights & (c & KING_SIDE)) {
      key ^= c == WHITE ? Zobrist::castling_rights[0]
                        : Zobrist::castling_rights[2];
    }
    if (st->castling_rights & (c & QUEEN_SIDE)) {
      key ^= c == WHITE ? Zobrist::castling_rights[1]
                        : Zobrist::castling_rights[3];
    }
  }

  if (st->can_ep) {
    key ^= Zobrist::ep_file[file_of(st->ep_square)];
  }

  return key;
}

Key Position::recompute_secondary() const {
  Key key = 0ULL;

  for (Color c : {WHITE, BLACK}) {
    for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN}) {
      BitBoard bb = pieces(c, pt);
      while (bb) {
        Square sq = Square(lsb(pop_lsb(bb)));
        if (pt == PAWN) {
          key ^= Zobrist::psq[piece_of(c, pt)][sq];
        } else if (pt == KNIGHT || pt == BISHOP) {
          key ^= Zobrist::psq[piece_of(c, pt)][sq];
        } else {
          key ^= Zobrist::psq[piece_of(c, pt)][sq];
        }
      }
    }
  }

  return key >> 48;
}

}  // namespace Juujfish