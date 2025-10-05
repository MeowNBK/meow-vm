#pragma once

// --- Bước 1: Phát hiện Kiến trúc Little-Endian 64-bit ---
// Đây là điều kiện tiên quyết để kích hoạt NaN-Boxing
// Logic này thường được định nghĩa trong các file Nan-Box, nhưng ta cần định nghĩa lại ở đây
// để việc chọn file include là độc lập.

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#  define MEOW_BYTE_ORDER __BYTE_ORDER__
#  define MEOW_ORDER_LITTLE __ORDER_LITTLE_ENDIAN__
#endif

// Kiểm tra 64-bit và Little-Endian (thường là x86_64 hoặc aarch64)
#if (defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) ) \
    && (defined(MEOW_BYTE_ORDER) && MEOW_BYTE_ORDER == MEOW_ORDER_LITTLE) \
    // && (sizeof(void*) == 8)
#  define MEOW_CAN_DO_NAN_BOXING 1
#else
#  define MEOW_CAN_DO_NAN_BOXING 0
#endif


// --- Bước 2: Chọn Chiến lược Tối ưu ---

#if MEOW_CAN_DO_NAN_BOXING
    // Ưu tiên 1: NaN-Boxing (Cực nhanh và nhỏ, 8 byte)
    // Chọn phiên bản "anything_goes" vì nó đã bao gồm logic NaN-Box với 3 bit tag 
    // và là triển khai mạnh mẽ nhất trong các file NaN-Box của bạn.
    #include "utils/types/variant_anything_goes.h"

#elif __cplusplus >= 202300L
    // Dự phòng 1: Sử dụng std::variant nếu C++23+ (tối ưu hóa compiler)
    #include <variant>
    // Bạn cần alias meow::utils::Variant = std::variant ở đây

#elif defined(__OPTIMIZE__)
    // Ưu tiên 2: Variant Optimized với Jump Tables hoặc Packed64
    // Chọn Packed64 nếu tất cả các kiểu đều nhỏ (để giữ 9 byte), hoặc Fast_Opt nếu cần tính tổng quát.
    // Dùng variant_fast.h (hoặc fast_opt) vì nó dùng Ops Tables, vẫn rất nhanh và portable.
    #include "utils/types/variant_fast.h"

#else
    // Dự phòng cuối: Sử dụng triển khai an toàn nhưng có thể chậm hơn
    // Hoặc sử dụng std::variant cho tính tương thích nếu không có file tùy chỉnh
    #include "utils/types/variant_legacy.h"
    // Nếu không muốn dùng legacy, bạn có thể buộc dùng variant_fast:
    // #include "utils/types/variant_fast.h"

#endif

// Trong trường hợp sử dụng std::variant làm fallback
#if !defined(MEOW_CAN_DO_NAN_BOXING) && (__cplusplus >= 202300L)
namespace meow::utils {
    template <typename... Args>
    using Variant = std::variant<Args...>;
}
#endif