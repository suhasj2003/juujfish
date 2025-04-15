#ifndef TRANSPOSITION_H
    #define TRANSPOSITION_H

    #include <iostream>

    #include "types.h"
    #include "search.h"

    namespace Juujfish {

        struct TableEntry;
        struct TableData;
        struct TableWriter;

        constexpr size_t CACHE_LINE = 128;
        constexpr size_t BUCKET_SIZE = 8;
        constexpr size_t HASH_SIZE = 30;
        constexpr size_t TABLE_MEM_SIZE = 1 << HASH_SIZE; // 128 mb, must be a multiple of CACHE_LINE

        /*
            zobrist_key - 8 bytes
            second_key  - 2 bytes
            depth       - 1 byte
            bound       - 2 bits
            age         - 6 bits
            score       - 2 bytes
            move        - 2 bytes

            total       - 16 bytes
        */
        struct TableEntry {
            public:
                TableEntry(TableEntry *entry) {
                    zobrist_key = entry->zobrist_key;
                    second_key = entry->second_key;
                    depth = entry->depth;
                    bound_age = entry->bound_age;
                    score = entry -> score;
                    move = entry->move;
                }

                TableEntry() { memset(this, 0, sizeof(TableEntry)); }

                inline Key      get_zobrist() const { return zobrist_key; }
                inline int8_t   get_depth() const { return depth; }
                inline uint8_t  get_age() const { return bound_age & ((1 << 6) - 1); }
                inline Bound    get_bound() const { return Bound(bound_age >> 6); }
                inline int64_t   get_score() const { return score; }
                inline Move     get_move() const { return move; }

                inline void clear() { memset(this, 0, sizeof(TableEntry)); }
                inline bool is_occupied() const { return bool(depth); }
                

                void save(Key zobrist_key, uint16_t second_key, int8_t depth, Bound b, uint8_t age, int16_t score, Move m);

                friend struct TableBucket;
            private:
                Key      zobrist_key;
                uint16_t second_key;

                int8_t depth;

                uint8_t bound_age;
                int16_t score;

                Move move;
        };

        struct TableData {
            TableData(TableEntry *entry) 
                    : zobrist_key(entry->get_zobrist()), depth(entry->get_depth()), 
                        bound(entry->get_bound()), score(entry->get_score()),
                        move(entry->get_move()) {}

            Key zobrist_key;

            int8_t depth;

            Bound bound;
            int16_t score;           

            Move move;
        };

        struct TableWriter {
            public:
                TableWriter(TableEntry *entry, uint8_t age) {
                    this->entry = entry;
                    this->age = age;
                }

                inline void write(Key zobrist_key, uint16_t second_key, int8_t depth, Bound b, int16_t score, Move m) {
                    entry->save(zobrist_key, second_key, depth, b, age, score, m);
                }

            private:
                TableEntry *entry;
                uint8_t age; 
        };

        struct TableBucket {
            TableBucket() { memset(entries, 0, sizeof(TableEntry) * BUCKET_SIZE); }
            size_t find_index(Key zobrist_key, uint16_t second_key) const;

            TableEntry entries[BUCKET_SIZE];
        };

        class TranspositionTable {
            public:
                ~TranspositionTable() { delete[] table; }

                inline void init() {
                    bucket_count = TABLE_MEM_SIZE / (BUCKET_SIZE * sizeof(TableEntry));
                    table = new(std::align_val_t(CACHE_LINE)) TableBucket[bucket_count];

                    table_age = 0;
                }
                inline void clear() { memset(table, 0, TABLE_MEM_SIZE); }
                inline void new_search() { table_age++; }
                inline uint8_t get_age() const { return table_age; }

                inline uint8_t relative_age(uint8_t entry_age) const { return table_age - entry_age; }

                std::tuple<bool, TableData, TableWriter> probe(Key zobrist_key, uint16_t second_key) const;

            private:
                size_t bucket_count;
                TableBucket *table;

                uint8_t table_age;
        };

    } // namespace Juujfish

#endif // ifndef TRANSPOSITION_H