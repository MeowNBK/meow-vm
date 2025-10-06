/**
 * @file variant.h
 * @brief The one true Variant. Selects the best implementation at compile time.
 * @author LazyPaws & Mèo mất não (2025)
 *
 * Chooses between:
 * - variant_nanbox.h: Ultra-fast 8-byte NaN-boxing for 64-bit little-endian systems.
 * - variant_jumptable.h: Fast, portable fallback using O(1) jump tables.
 *
 * The rest of the codebase should ONLY include this file to use meow::utils::Variant.
 */

#pragma once

// --- Bước 1: Phát hiện nền tảng để chọn "chiến binh" ---
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#  define MEOW_BYTE_ORDER __BYTE_ORDER__
#  define MEOW_ORDER_LITTLE __ORDER_LITTLE_ENDIAN__
#endif

// Điều kiện để dùng NaN-Boxing: 64-bit VÀ little-endian
#if (defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)) && (defined(MEOW_BYTE_ORDER) && MEOW_BYTE_ORDER == MEOW_ORDER_LITTLE)
#  define MEOW_USE_NAN_BOXING 1
#else
#  define MEOW_USE_NAN_BOXING 0
#endif


// --- Bước 2: Gọi "chiến binh" tương ứng ra trận ---
#if MEOW_USE_NAN_BOXING
#  include "utils/types/variant_nanbox.h"
namespace meow::utils { template <typename... Ts> using Variant = NaNBoxedVariant<Ts...>; }
#else
#  include "utils/types/variant_jumptable.h"
namespace meow::utils { template <typename... Ts> using Variant = Variant<Ts...>; }
#endif
