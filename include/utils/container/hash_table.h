/**
 * @file hash_table.h
 * @author LazyPaws
 * @brief An util for HashTable in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "utils/hash.h"
#include "utils/list.h"
#include "utils/optional.h"
#include "utils/pair.h"
#include "utils/vector.h"

namespace meow::utils {
template <typename T, typename U>
class HashTable {
   private:
    using value_type = U;
    using key_t = T;
    using const_value_reference_t = const value_type&;
    using const_key_reference_t = const key_t&;
    using pair_type = Pair<key_t, value_type>;
    using table_t = utils::Vector<utils::List<utils::Pair<key_t, value_type>>>;

    table_t table_;

   public:
    HashTable(size_t new_size = 10, const_value_reference_t temp_value = value_type()) noexcept : table_(new_size) {
        table_.resize(new_size);
    }
    inline void insert(const_key_reference_t key, const_value_reference_t value) noexcept {
        size_t index = hash(key, table_.size());
        table_[index].push_front(pair_type(key, value));
    }
    [[nodiscard]] inline utils::Optional<const_value_reference_t> get(const_key_reference_t key) noexcept {
        size_t index = hash(key, table_.size());
        for (auto it = table_[index].begin(); it != table_[index].end(); it = it->next()) {
            if (it->data_.first_ == key) return it->data_.second_;
        }
        return temp_value_;
    }
};
}  // namespace meow::utils