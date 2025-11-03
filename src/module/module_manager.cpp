#include "module/module_manager.h"

#include "common/pch.h"
#include "core/objects/module.h"
#include "core/objects/native.h"  // Cần thiết cho NativeFunction
#include "core/objects/string.h"
#include "module/loader/lexer.h"
#include "module/loader/parser.h"  // Cần TextParser để load file .meow
#include "memory/memory_manager.h"
#include "module/module_utils.h"  // Chứa các hàm helper quan trọng
#include "vm/meow_engine.h"       // Cần thiết cho hàm factory của native module

namespace meow::module {

using namespace meow::core;
using namespace meow::loader;
using namespace meow::memory;
using namespace meow::vm;

ModuleManager::ModuleManager(MemoryManager* heap, MeowEngine* engine) noexcept
    : heap_(heap), engine_(engine) {}

// --- Hàm load_module ---
module_t ModuleManager::load_module(string_t module_path_obj, string_t importer_path_obj) {
    if (!module_path_obj || !importer_path_obj) {
        // Nên throw exception hoặc trả về một giá trị lỗi rõ ràng
        throw std::runtime_error("ModuleManager::load_module: Invalid arguments (null pointers).");
    }

    std::string module_path = module_path_obj->c_str();
    std::string importer_path = importer_path_obj->c_str();

    // 1. Kiểm tra cache bằng đường dẫn yêu cầu ban đầu
    if (auto it = module_cache_.find(module_path_obj); it != module_cache_.end()) {
        return it->second;
    }

    // 2. Thiết lập các thông số cho việc tìm kiếm thư viện
    // Các đuôi file ngôn ngữ (không phải native)
    const std::vector<std::string> forbidden_extensions = {".meow", ".meowb"};
    // Đuôi file native theo platform
    const std::vector<std::string> candidate_extensions = {get_platform_library_extension()};
    // Các thư mục tìm kiếm cơ bản (ví dụ: gần executable, thư mục stdlib)
    std::filesystem::path root_dir =
        detect_root_cached("meow-root",  // Tên file config gốc (có thể thay đổi nếu bạn muốn)
                           "$ORIGIN",  // Token thay thế (có thể thay đổi)
                           true        // Coi thư mục 'bin' là con của thư mục gốc
        );
    std::vector<std::filesystem::path> search_roots = make_default_search_roots(root_dir);

    // 3. Cố gắng tìm và load như một thư viện native
    std::string resolved_native_path = resolve_library_path_generic(
        module_path, importer_path, entry_path_ ? entry_path_->c_str() : "", forbidden_extensions,
        candidate_extensions, search_roots, true);

    if (!resolved_native_path.empty()) {
        // Tìm thấy file có vẻ là native library!

        // Kiểm tra cache bằng đường dẫn tuyệt đối đã giải quyết
        string_t resolved_native_path_obj = heap_->new_string(resolved_native_path);
        if (auto it = module_cache_.find(resolved_native_path_obj); it != module_cache_.end()) {
            // Thêm vào cache với key là đường dẫn gốc luôn cho nhanh lần sau
            module_cache_[module_path_obj] = it->second;
            return it->second;
        }

        void* handle = open_native_library(resolved_native_path);
        if (!handle) {
            std::string err_detail = platform_last_error();
            throw std::runtime_error("Failed to load native library '" + resolved_native_path +
                                     "': " + err_detail);
        }

        // Tên hàm factory chuẩn để tạo module native
        const char* factory_symbol_name = "CreateMeowModule";
        void* symbol = get_native_symbol(handle, factory_symbol_name);

        if (!symbol) {
            std::string err_detail = platform_last_error();
            close_native_library(handle);  // Nhớ đóng handle nếu không tìm thấy symbol
            throw std::runtime_error("Failed to find symbol '" + std::string(factory_symbol_name) +
                                     "' in native library '" + resolved_native_path +
                                     "': " + err_detail);
        }

        // Định nghĩa kiểu con trỏ hàm factory
        using NativeModuleFactory = module_t (*)(MeowEngine*, MemoryManager*);
        NativeModuleFactory factory = reinterpret_cast<NativeModuleFactory>(symbol);

        module_t native_module = nullptr;
        try {
            // Gọi hàm factory để tạo module object
            native_module = factory(engine_, heap_);
            if (!native_module) {
                throw std::runtime_error("Native module factory for '" + resolved_native_path +
                                         "' returned null.");
            }
            // TODO: Có thể cần set thêm filename/filepath cho native module?
            // native_module->set_file_name(...);
            // native_module->set_file_path(...);
            native_module->set_executed();  // Native module coi như đã "chạy" xong khi load

        } catch (const std::exception& e) {
            close_native_library(handle);
            throw std::runtime_error("Exception thrown while calling native module factory for '" +
                                     resolved_native_path + "': " + e.what());
        } catch (...) {
            close_native_library(handle);
            throw std::runtime_error(
                "Unknown exception thrown while calling native module factory for '" +
                resolved_native_path + "'.");
        }

        // Cache lại module native với cả hai key (path gốc và path tuyệt đối)
        module_cache_[module_path_obj] = native_module;
        module_cache_[resolved_native_path_obj] = native_module;
        return native_module;
    }

    // 4. Không phải native library (hoặc không tìm thấy), xử lý như module MeowScript (.meow)
    std::filesystem::path base_dir;
    if (importer_path_obj == entry_path_) {  // Nếu import từ entry point
        base_dir = std::filesystem::path(entry_path_ ? entry_path_->c_str() : "").parent_path();
    } else {
        base_dir = std::filesystem::path(importer_path).parent_path();
    }

    // Giải quyết đường dẫn tuyệt đối cho file .meow
    std::filesystem::path meow_file_path_fs = normalize_path(base_dir / module_path);

    // Thử thêm đuôi .meow nếu chưa có
    if (!meow_file_path_fs.has_extension()) {
        meow_file_path_fs.replace_extension(".meow");
    } else {
        // Nếu có đuôi rồi nhưng không phải .meow, báo lỗi? (Tùy thiết kế)
        // if (meow_file_path_fs.extension() != ".meow") { ... }
    }

    std::string meow_file_path = meow_file_path_fs.string();
    string_t meow_file_path_obj = heap_->new_string(meow_file_path);

    // Kiểm tra cache bằng đường dẫn tuyệt đối của file .meow
    if (auto it = module_cache_.find(meow_file_path_obj); it != module_cache_.end()) {
        module_cache_[module_path_obj] = it->second;  // Cache thêm bằng path gốc
        return it->second;
    }

    // 5. Parse file .meow
    // Giả sử bạn có TextParser trong namespace meow::loader
    proto_t main_proto = nullptr;

    std::ifstream file(meow_file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Không thể mở tệp: " + meow_file_path);
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string buffer(size, '\0');

    file.seekg(0);
    file.read(&buffer[0], size);
    file.close();

    Lexer lexer(buffer);

    auto tokens = lexer.tokenize();

    TextParser parser(heap_, std::move(tokens), meow_file_path);

    try {
        main_proto = parser.parse_source();  // Parser sẽ throw nếu có lỗi
        if (!main_proto) {
            throw std::runtime_error("Parser returned null proto for '" + meow_file_path +
                                     "' without throwing.");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse MeowScript module '" + meow_file_path +
                                 "': " + e.what());
    }

    // 6. Tạo và cache module object cho file .meow
    // Lấy tên file từ đường dẫn đầy đủ
    string_t filename_obj = heap_->new_string(meow_file_path_fs.filename().string());
    module_t meow_module = heap_->new_module(filename_obj, meow_file_path_obj, main_proto);

    // Cache lại module .meow với cả hai key
    module_cache_[module_path_obj] = meow_module;
    module_cache_[meow_file_path_obj] = meow_module;

    // Quan trọng: Module .meow mới chỉ parse, chưa thực thi.
    // Việc thực thi (chạy @main) sẽ do VM xử lý khi gặp lệnh IMPORT_MODULE lần đầu.
    // Đánh dấu nó là chưa chạy.
    // meow_module->set_execution_state(State::NOT_EXECUTED); // Giả sử có hàm này

    return meow_module;
}

}  // namespace meow::module