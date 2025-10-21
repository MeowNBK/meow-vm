#pragma once

#include <cstddef>

namespace meow::core {
struct MeowObject;
}

namespace meow::memory {
/**
 * @class GarbageCollector
 * @brief Dọn dẹp các object không còn được sử dụng, tránh memory leak
 */
class GarbageCollector {
   public:
    virtual ~GarbageCollector() = default;

    /**
     * @brief Đăng kí một object để GC quản lí
     * @param[in] object Object cần được GC quản li
     */
    virtual void register_object(const meow::core::MeowObject* object) = 0;

    /**
     * @brief Dọn dẹp các object không còn dược sử dụng
     */
    virtual size_t collect() noexcept = 0;
};
}  // namespace meow::memory