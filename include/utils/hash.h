#pragma once

#include "common/pch.h"

[[nodiscard]] inline size_t hash(size_t number, size_t bucket_count = 10) noexcept {
    return number % bucket_count;
}