#include <tuple>

#include "transposition.h"

namespace Juujfish {

inline Key zobrist_to_index(Key zobrist_key) {
  uint64_t mask = ((1ULL << HASH_SIZE) - 1) << 16;
  return ((zobrist_key & mask) >> 16) / (BUCKET_SIZE * sizeof(TableEntry));
}

void TableEntry::save(Key zobrist_key, uint16_t second_key, int8_t depth,
                      Bound b, uint8_t age, int16_t score, Move m) {
  this->zobrist_key = zobrist_key;
  this->second_key = second_key;
  this->depth = depth;

  bound_age = b << 6 + age;

  this->score = score;
  move = m;
}

size_t TableBucket::find_index(Key zobrist_key, uint16_t second_key) const {
  size_t idx = 0;
  while (idx < BUCKET_SIZE && !(entries[idx].zobrist_key == zobrist_key &&
                                entries[idx].second_key == second_key)) {
    ++idx;
  }
  return idx;
}

std::tuple<bool, TableData, TableWriter> TranspositionTable::probe(
    Key zobrist_key, uint16_t second_key) const {
  TableBucket* bucket;
  size_t entry_index;

  size_t bucket_index = zobrist_to_index(zobrist_key);

  assert(bucket_index < bucket_count &&
         "Error: Bucket index is not less than Bucket count");

  bucket = &table[bucket_index];
  entry_index = bucket->find_index(zobrist_key, second_key);

  TableEntry* entry = &bucket->entries[entry_index];

  if (entry_index < BUCKET_SIZE && entry->is_occupied()) {
    return std::make_tuple(true, TableData(entry),
                           TableWriter(entry, this->table_age));
  } else if ((entry_index = bucket->find_index(0, 0)) < BUCKET_SIZE) {
    entry = &bucket->entries[entry_index];
    return std::make_tuple(false, TableData(entry),
                           TableWriter(entry, this->table_age));
  } else {

    // Replacement:
    entry = &bucket->entries[0];
    TableEntry* replacement_entry = &bucket->entries[0];

    int replacement_score = entry->get_depth() - relative_age(entry->get_age());

    for (size_t i = 1; i < BUCKET_SIZE; ++i) {
      entry = &bucket->entries[i];
      if (entry->is_occupied() &&
          replacement_score >
              (entry->get_depth() - relative_age(entry->get_age()))) {

        replacement_score =
            (entry->get_depth() - relative_age(entry->get_age()));
        replacement_entry = entry;
      }
    }

    return std::make_tuple(false, TableData(replacement_entry),
                           TableWriter(replacement_entry, this->table_age));
  }
}

}  // namespace Juujfish