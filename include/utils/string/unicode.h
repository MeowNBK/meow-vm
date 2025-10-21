// #pragma once

// #include "common/pch.h"

// namespace meow::utils {
//     inline static std::u32string utf8_to_utf32(const std::string& s) noexcept
//     {
//         std::u32string out;
//         const unsigned char* bytes = reinterpret_cast<const unsigned
//         char*>(s.data()); size_t n = s.size(), i = 0; while (i < n) {
//             unsigned char b = bytes[i];
//             if (b <= 0x7F) {
//                 out.push_back(b); i += 1;
//             } else if ((b >> 5) == 0x6 && i+1 < n) {
//                 out.push_back(static_cast<char32_t>(((b & 0x1F) << 6) |
//                 (bytes[i+1] & 0x3F))); i += 2;
//             } else if ((b >> 4) == 0xE && i+2 < n) {
//                 out.push_back(static_cast<char32_t>(((b & 0x0F) << 12) |
//                 ((bytes[i+1] & 0x3F) << 6) | (bytes[i+2] & 0x3F))); i += 3;
//             } else if ((b >> 3) == 0x1E && i+3 < n) {
//                 out.push_back(static_cast<char32_t>(((b & 0x07) << 18) |
//                 ((bytes[i+1] & 0x3F) << 12) |
//                                 ((bytes[i+2] & 0x3F) << 6) | (bytes[i+3] &
//                                 0x3F)));
//                 i += 4;
//             } else {
//                 out.push_back(0xFFFD); i += 1;
//             }
//         }
//         return out;
//     }
// }