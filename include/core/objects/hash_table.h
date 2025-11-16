/**
 * @file hash_table.h
 * @author LazyPaws
 * @brief Core definition of Hash Table in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "core/meow_object.h"
#include "core/type.h"
#include "core/value.h"
#include "memory/gc_visitor.h"

namespace meow::core::objects {
class ObjHashTable : public meow::core::ObjBase<ObjectType::HASH_TABLE> {
   private:
    using key_t = meow::core::string_t;
    using map_t = std::unordered_map<key_t, value_t>;
    using visitor_t = meow::memory::GCVisitor;

    map_t fields_;

   public:
    // --- Constructors & destructor---
    ObjHashTable() = default;
    explicit ObjHashTable(const map_t& fields) : fields_(fields) {
    }
    explicit ObjHashTable(map_t&& fields) noexcept : fields_(std::move(fields)) {
    }

    // --- Rule of 5 ---
    ObjHashTable(const ObjHashTable&) = delete;
    ObjHashTable(ObjHashTable&&) = delete;
    ObjHashTable& operator=(const ObjHashTable&) = delete;
    ObjHashTable& operator=(ObjHashTable&&) = delete;
    ~ObjHashTable() override = default;

    // --- Iterator types ---
    using iterator = map_t::iterator;
    using const_iterator = map_t::const_iterator;

    // --- Lookup ---

    // Unchecked lookup. For performance-critical code
    [[nodiscard]] inline meow::core::return_t get(key_t key) noexcept {
        return fields_[key];
    }
    // Unchecked lookup/update. For performance-critical code
    template <typename T>
    inline void set(key_t key, T&& value) noexcept {
        fields_[key] = std::forward<T>(value);
    }
    // Checked lookup. Throws if key is not found
    [[nodiscard]] inline meow::core::return_t at(key_t key) const {
        return fields_.at(key);
    }
    [[nodiscard]] inline bool has(key_t key) const {
        return fields_.find(key) != fields_.end();
    }

    // --- Capacity ---
    [[nodiscard]] inline size_t size() const noexcept {
        return fields_.size();
    }
    [[nodiscard]] inline bool empty() const noexcept {
        return fields_.empty();
    }

    // --- Iterators ---
    inline iterator begin() noexcept {
        return fields_.begin();
    }
    inline iterator end() noexcept {
        return fields_.end();
    }
    inline const_iterator begin() const noexcept {
        return fields_.begin();
    }
    inline const_iterator end() const noexcept {
        return fields_.end();
    }

    void trace(visitor_t& visitor) const noexcept override;
};
}  // namespace meow::core::objects