/**
 * @file string.h
 * @author LazyPaws
 * @brief Core definition of String in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/meow_object.h"

namespace meow::core::objects {
class ObjString : public meow::core::ObjBase<ObjectType::STRING> {
private:
    using storage_t = std::string;
    using visitor_t = meow::memory::GCVisitor;
    storage_t data_;
public:
    // --- Constructors & destructor ---
    ObjString() = default;
    explicit ObjString(const storage_t& data) : data_(data) {}
    explicit ObjString(storage_t&& data) noexcept : data_(std::move(data)) {}
    explicit ObjString(const char* data) : data_(data) {}

    // --- Rule of 5 ---
    ObjString(const ObjString&) = delete;
    ObjString(ObjString&&) = default;
    ObjString& operator=(const ObjString&) = delete;
    ObjString& operator=(ObjString&&) = delete;
    ~ObjString() override = default;

    // --- Assignments ---
    inline ObjString operator+(const ObjString& other) const noexcept {
        return ObjString(data_ + other.data_);
    }
    inline ObjString operator!() const noexcept {
        std::string temp(data_.size(), '\0');
        for (size_t i = 0; i < data_.size(); ++i) {
            temp[i] = 127 - data_[i];
        }
        return ObjString(temp);
    }

    // --- Iterator types ---
    using const_iterator = storage_t::const_iterator;
    using const_reverse_iterator = storage_t::const_reverse_iterator;

    // --- Character access ---

    /// @brief Unchecked character access. For performance-critical code
    [[nodiscard]] inline char get(size_t index) const noexcept {
        return data_[index];
    }
    /// @brief Checked character access. Throws if index is OOB
    [[nodiscard]] inline char at(size_t index) const {
        return data_.at(index);
    }

    // --- String access ---
    [[nodiscard]] inline const char* c_str() const noexcept {
        return data_.c_str();
    }

    // --- Capacity ---
    [[nodiscard]] inline size_t size() const noexcept {
        return data_.size();
    }
    [[nodiscard]] inline bool empty() const noexcept {
        return data_.empty();
    }

    // --- Iterators ---
    inline const_iterator begin() const noexcept {
        return data_.begin();
    }
    inline const_iterator end() const noexcept {
        return data_.end();
    }
    inline const_reverse_iterator rbegin() const noexcept {
        return data_.rbegin();
    }
    inline const_reverse_iterator rend() const noexcept {
        return data_.rend();
    }

    inline void trace(visitor_t&) const noexcept override {}
};
}  // namespace meow::core::objects