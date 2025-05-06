#ifndef MISC_H
    #define MISC_H

    #include <array>
    #include <iostream>

    #include "types.h"
    #include "bitboard.h"
    #include "position.h"
    #include "movegen.h"

    namespace Juujfish {

        template<typename T, size_t... Dims>
        class NDArray {
            public:
                static_assert(sizeof...(Dims) > 0, "Error: MultiArray must have at least one dimension");
                static_assert(((Dims > 0) && ...), "Error: All dimensions must be greater than zero");

                NDArray(const NDArray&) = delete; 
                NDArray& operator=(const NDArray&) = delete; 

                NDArray() {
                   init();
                }

                NDArray(T init_value) {
                    init(init_value);
                }

                ~NDArray() = default;

                inline void init() {
                    for (size_t i = 0; i < total_size; ++i) {
                        data[i] = T();
                    }
                }

                inline void init(T init_value) {
                    for (size_t i = 0; i < total_size; ++i) {
                        data[i] = init_value;
                    }
                }

                template<typename... Args>
                inline T get(Args... indices) const {
                    size_t index = flatten_index(indices...);
                    assert(index < total_size);

                    return data[index];
                }

                template<typename... Args>
                inline void set(const T& value, Args... indices) {
                    size_t index = flatten_index(indices...);
                    assert(index < total_size);

                    data[index] = value;
                }

            private:
                static constexpr size_t get_total_size_helper() {
                    size_t size = 1;
                    ((size *= Dims), ...);
                    return size;
                }

                const size_t total_size = get_total_size_helper();
                T data[get_total_size_helper()];

                const std::array<size_t, sizeof...(Dims)> dimensions = {Dims...};
                const std::array<size_t, sizeof...(Dims)> strides = [this] {
                    std::array<size_t, sizeof...(Dims)> temp{};
                    size_t stride = 1;
                    for (size_t i = sizeof...(Dims); i-- > 0;) {
                        temp[i] = stride;
                        stride *= dimensions[i];
                    }
                    return temp;
                }();

                template<typename... Args>
                inline size_t flatten_index(Args... indices) const {
                    static_assert(sizeof...(Args) == sizeof...(Dims), "Error: Number of indices must match the number of dimensions");
                    size_t index = 0;
                    size_t i = 0;
                    ((index += indices * strides[i++]), ...);
                    return index;
                }
        };


        // Stuff below will either be deleted, reimplemented, or moved to a more appropriate location in the codebase.

        inline char piece_to_char(Piece p) {
            switch (p) {
                case W_PAWN:    return 'P';
                case W_KNIGHT:  return 'N';
                case W_BISHOP:  return 'B';
                case W_ROOK:    return 'R';
                case W_QUEEN:   return 'Q';
                case W_KING:    return 'K';
                case B_PAWN:    return 'p';
                case B_KNIGHT:  return 'n';
                case B_BISHOP:  return 'b';
                case B_ROOK:    return 'r';
                case B_QUEEN:   return 'q';
                case B_KING:    return 'k';
                default:        return '.';
            }
        }

        inline std::string pretty(BitBoard b) {

            std::string s = "+---+---+---+---+---+---+---+---+\n";
        
            for (int r = RANK_8; r >= RANK_1; --r)
            {
                for (int f = FILE_A; f <= FILE_H; ++f)
                    s += b & Square((r << 3) + f) ? "| X " : "|   ";
        
                s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
            }
            s += "  a   b   c   d   e   f   g   h\n";
        
            return s;
        }
        
        inline std::string pretty(Position &p) {
        
            std::string s = "+---+---+---+---+---+---+---+---+\n";
        
            for (int r = RANK_8; r >= RANK_1; --r)
            {
                for (int f = FILE_A; f <= FILE_H; ++f) {
                    Piece piece = p.piece_at(Square((r << 3) + f));
                    if (piece != NO_PIECE) {
                        s += "| " + std::string(1, piece_to_char(piece)) + " ";
                    } else {
                        s += "|   ";
                    }
                }  
                s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
            }
            s += "  a   b   c   d   e   f   g   h\n";
        
            return s;
        }
        
        inline int squareIndex(const std::string& pos) {
            return (pos[1] - '1') * 8 + (pos[0] - 'a');
        }

        inline std::string moveToString(Move m) {
            std::string moveStr;
        
            Square from = m.from_sq();
            Square to = m.to_sq();
        
            moveStr += char('a' + file_of(from));
            moveStr += char('1' + rank_of(from));
            moveStr += char('a' + file_of(to));
            moveStr += char('1' + rank_of(to));
        
            if (m.type_of() == PROMOTION) {
                switch (m.promotion_type()) {
                    case KNIGHT: moveStr += 'n'; break;
                    case BISHOP: moveStr += 'b'; break;
                    case ROOK: moveStr += 'r'; break;
                    case QUEEN: moveStr += 'q'; break;
                    default: break;
                }
            } else if (m.type_of() == CASTLING) {
                if (from == Square(4) && to == Square(6)) {
                    return "e1g1"; // White kingside castling
                } else if (from == Square(4) && to == Square(2)) {
                    return "e1c1"; // White queenside castling
                } else if (from == Square(60) && to == Square(62)) {
                    return "e8g8"; // Black kingside castling
                } else if (from == Square(60) && to == Square(58)) {
                    return "e8c8"; // Black queenside castling
                }
            }
        
            return moveStr;
        }

        inline Move parseMove(Position &pos, const std::string &moveStr) {
            if (moveStr.length() < 2) {
                return Move::null_move();
            }
        
            MoveList<LEGAL> legalMoves(pos);
        
            for (auto m : legalMoves) {
                std::string legalMoveStr = moveToString(m);
        
                if (legalMoveStr == moveStr) {
                    return m; 
                }
            }
            return Move::null_move();
        }

    } // namespace Juujfish

#endif // ifndef MISC_H