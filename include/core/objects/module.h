/**
 * @file module.h
 * @author LazyPaws
 * @brief Core definition of Module in TrangMeo
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
class ObjModule : public meow::core::MeowObject {
   private:
    using string_t = meow::core::string_t;
    using proto_t = meow::core::proto_t;
    using module_map = std::unordered_map<string_t, value_t>;
    using visitor_t = meow::memory::GCVisitor;

    enum class State { EXECUTING, EXECUTED };

    module_map globals_;
    module_map exports_;
    string_t file_name_;
    string_t file_path_;
    proto_t main_proto_;

    State state;

   public:
    explicit ObjModule(string_t file_name, string_t file_path,
                       proto_t main_proto = nullptr) noexcept
        : file_name_(file_name), file_path_(file_path), main_proto_(main_proto) {}

    // --- Globals ---
    [[nodiscard]] inline meow::core::return_t get_global(string_t name) noexcept {
        return globals_[name];
    }
    inline void set_global(string_t name, meow::core::param_t value) noexcept {
        globals_[name] = value;
    }
    [[nodiscard]] inline bool has_global(string_t name) {
        return globals_.find(name) != globals_.end();
    }

    // --- Exports ---
    [[nodiscard]] inline meow::core::return_t get_export(string_t name) noexcept {
        return exports_[name];
    }
    inline void set_export(string_t name, meow::core::param_t value) noexcept {
        exports_[name] = value;
    }
    [[nodiscard]] inline bool has_export(string_t name) {
        return exports_.find(name) != exports_.end();
    }

    // --- File info ---
    inline string_t get_file_name() const noexcept { return file_name_; }
    inline string_t get_file_path() const noexcept { return file_path_; }

    // --- Main proto ---
    inline proto_t get_main_proto() const noexcept { return main_proto_; }
    inline void set_main_proto(proto_t proto) noexcept { main_proto_ = proto; }
    inline bool is_has_main() const noexcept { return main_proto_ != nullptr; }

    // --- Execution state ---
    inline void set_execution() noexcept { state = State::EXECUTING; }
    inline void set_executed() noexcept { state = State::EXECUTED; }
    [[nodiscard]] inline bool is_executing() const noexcept { return state == State::EXECUTING; }
    [[nodiscard]] inline bool is_executed() const noexcept { return state == State::EXECUTED; }

    void trace(visitor_t &visitor) const noexcept override;
};
}  // namespace meow::core::objects