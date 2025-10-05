# Phiên bản CMake tối thiểu để dùng C++ Modules
cmake_minimum_required(VERSION 3.28)

# Định nghĩa project
project(meow LANGUAGES CXX)

# Cấu hình cho chuẩn C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tìm các file nguồn (.cpp)
file(GLOB_RECURSE MEOW_IMPLEMENT_SOURCES
    "src/*.cpp"
)

# Tạo file Executable chính
add_executable(meow)

# Thêm tất cả các file implementation vào target
target_sources(meow PRIVATE
    ${MEOW_IMPLEMENT_SOURCES}
)

# Thiết lập đường dẫn Include
target_include_directories(meow PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Biên dịch precompiled headers
target_precompile_headers(meow PRIVATE "include/common/pch.h")

# Đặt vị trí output cho file Executable
set_target_properties(meow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)