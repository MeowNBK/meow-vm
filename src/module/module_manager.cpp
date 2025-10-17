// --- Old codes ---

// #pragma once

// #include "common/pch.h"
// #include "core/definitions.h"
// #include "common/expected.h"
// #include "diagnostics/diagnostic.h"
// #include "loader/bytecode_parser.h"

// class MeowEngine;
// class MemoryManager;

// class ModuleManager {
// public:
//     Expected<Module, Diagnostic> loadModule(const std::string& modulePath, const std::string& importerPath, bool isBinary);

//     inline std::unordered_map<std::string, Module>& getModuleCache() noexcept {
//         return moduleCache;
//     }

//     inline const std::unordered_map<std::string, Module>& getModuleCache() const noexcept {
//         return moduleCache;
//     }

//     inline void resetModuleCache() noexcept {
//         moduleCache.clear();
//     }

//     inline void addCache(const std::string& name, const Module& mod) {
//         moduleCache[name] = mod;
//     }

//     inline void setEntryPath(const std::string& entry) noexcept {
//         entryPath = entry;
//     }

//     inline void setEngine(MeowEngine* eng) noexcept {
//         engine = eng;
//     }

//     inline void setMemoryManager(MemoryManager* manager) noexcept {
//         memoryManager = manager;
//     }
// private:
//     std::unordered_map<std::string, Module> moduleCache;
//     BytecodeParser textParser;
//     std::string entryPath;

//     MeowEngine* engine;
//     MemoryManager* memoryManager;
// };


// #include "module/module_manager.h"
// #include "memory/memory_manager.h"
// #include "vm/meow_engine.h"

// #if defined(_WIN32)
// #include <windows.h>
// #else
// #include <dlfcn.h>
// #endif

// #if !defined(_WIN32)
// #include <unistd.h>
// #include <limits.h>
// #endif
// #if defined(__APPLE__)
// #include <mach-o/dyld.h>
// #endif

// // --- helper: lấy thư mục chứa executable (cross-platform) ---
// static std::filesystem::path getExecutableDir() {
// #if defined(_WIN32)
//     char buf[MAX_PATH];
//     DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
//     if (len == 0) throw std::runtime_error("GetModuleFileNameA failed");
//     return std::filesystem::path(std::string(buf, static_cast<size_t>(len))).parent_path();
// #elif defined(__linux__)
//     char buf[PATH_MAX];
//     ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
//     if (len == -1) throw std::runtime_error("readlink(/proc/self/exe) failed");
//     buf[len] = '\0';
//     return std::filesystem::path(std::string(buf)).parent_path();
// #elif defined(__APPLE__)
//     uint32_t size = 0;
//     _NSGetExecutablePath(nullptr, &size); // will set size
//     std::vector<char> buf(size);
//     if (_NSGetExecutablePath(buf.data(), &size) != 0) {
//         throw std::runtime_error("_NSGetExecutablePath failed");
//     }
//     std::filesystem::path p(buf.data());
//     return std::filesystem::absolute(p).parent_path();
// #else
//     return std::filesystem::current_path();
// #endif
// }

// // --- helper: expand token $ORIGIN in a path string to exeDir ---
// static std::string expandOriginToken(const std::string& raw, const std::filesystem::path& exeDir) {
//     std::string out;
//     const std::string token = "$ORIGIN";
//     size_t pos = 0;
//     while (true) {
//         size_t p = raw.find(token, pos);
//         if (p == std::string::npos) {
//             out.append(raw.substr(pos));
//             break;
//         }
//         out.append(raw.substr(pos, p - pos));
//         out.append(exeDir.string());
//         pos = p + token.size();
//     }
//     return out;
// }

// // --- detect stdlib root: đọc file meow-root cạnh binary (1 lần, cached) ---
// static std::filesystem::path detectStdlibRoot_cached() {
//     static std::optional<std::filesystem::path> cached;
//     if (cached.has_value()) return *cached;

//     std::filesystem::path result;
//     try {
//         std::filesystem::path exeDir = getExecutableDir(); // thư mục chứa binary
//         std::filesystem::path configFile = exeDir / "meow-root";

//         // 1) nếu có file meow-root, đọc và expand $ORIGIN
//         if (std::filesystem::exists(configFile)) {
//             std::ifstream in(configFile);
//             if (in) {
//                 std::string line;
//                 std::getline(in, line);
//                 // trim (simple)
//                 while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
//                 size_t i = 0;
//                 while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
//                 if (i > 0) line = line.substr(i);

//                 if (!line.empty()) {
//                     std::string expanded = expandOriginToken(line, exeDir);
//                     result = std::filesystem::absolute(std::filesystem::path(expanded));
//                     cached = result;
//                     return result;
//                 }
//             }
//         }

//         // 2) fallback nhanh: nếu exeDir là "bin", dùng parent; ngược lại dùng exeDir
//         if (exeDir.filename() == "bin") {
//             result = exeDir.parent_path();
//         } else {
//             result = exeDir;
//         }

//         // 3) nếu không tìm thấy stdlib trực tiếp ở root, có thể thử root/lib
//         cached = std::filesystem::absolute(result);
//         return *cached;
//     } catch (...) {
//         // fallback to current path nếu có gì sai
//         cached = std::filesystem::current_path();
//         return *cached;
//     }
// }


// static std::string platformLastError() {
// #if defined(_WIN32)
//     DWORD err = GetLastError();
//     if (!err) return "";
//     LPSTR msgBuf = nullptr;
//     FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//                    nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//                    (LPSTR)&msgBuf, 0, nullptr);
//     std::string msg = msgBuf ? msgBuf : "";
//     if (msgBuf) LocalFree(msgBuf);
//     return msg;
// #else
//     const char* e = dlerror();
//     return e ? std::string(e) : std::string();
// #endif
// }

// // Trả về Expected<Module, Diagnostic> thay vì ném VMError
// Expected<Module, Diagnostic> ModuleManager::loadModule(const std::string& modulePath, const std::string& importerPath, Bool isBinary) {
//     // check cache by requested modulePath first
//     if (auto it = moduleCache.find(modulePath); it != moduleCache.end()) {
//         return it->second;
//     }

// #if defined(_WIN32)
//     std::string libExtension = ".dll";
// #elif defined(__APPLE__)
//     std::string libExtension = ".dylib";
// #else
//     std::string libExtension = ".so";
// #endif

//     auto resolveLibraryPath = [&](const std::string& modPath, const std::string& importer) -> std::string {
//         try {
//             std::filesystem::path candidate(modPath);
//             std::string ext = candidate.extension().string();

//             // if explicit meow/text/binary extension provided, don't try to treat as native library here
//             if (!ext.empty()) {
//                 std::string extLower = ext;
//                 for (char &c : extLower) c = (char)std::tolower((unsigned char)c);
//                 if (extLower == ".meow" || extLower == ".meowb") {
//                     return "";
//                 }
//             }

//             if (candidate.extension().empty()) {
//                 candidate.replace_extension(libExtension);
//             }

//             if (candidate.is_absolute() && std::filesystem::exists(candidate)) {
//                 return std::filesystem::absolute(candidate).lexically_normal().string();
//             }

//             std::filesystem::path stdlibRoot = detectStdlibRoot_cached();
//             std::filesystem::path stdlibPath = stdlibRoot / candidate;
//             if (std::filesystem::exists(stdlibPath)) {
//                 return std::filesystem::absolute(stdlibPath).lexically_normal().string();
//             }

//             stdlibPath = stdlibRoot / "lib" / candidate;
//             if (std::filesystem::exists(stdlibPath)) {
//                 return std::filesystem::absolute(stdlibPath).lexically_normal().string();
//             }

//             stdlibPath = stdlibRoot / "stdlib" / candidate;
//             if (std::filesystem::exists(stdlibPath)) {
//                 return std::filesystem::absolute(stdlibPath).lexically_normal().string();
//             }

//             stdlibPath = stdlibRoot / "bin" / "stdlib" / candidate;
//             if (std::filesystem::exists(stdlibPath)) {
//                 return std::filesystem::absolute(stdlibPath).lexically_normal().string();
//             }

//             stdlibPath = stdlibRoot / "bin" / candidate;
//             if (std::filesystem::exists(stdlibPath)) {
//                 return std::filesystem::absolute(stdlibPath).lexically_normal().string();
//             }

//             // try parent/bin/stdlib as extra attempt
//             stdlibPath = std::filesystem::absolute(stdlibRoot / ".." / "bin" / "stdlib" / candidate);
//             if (std::filesystem::exists(stdlibPath)) {
//                 return std::filesystem::absolute(stdlibPath).lexically_normal().string();
//             }

//             // try relative to importer
//             std::filesystem::path baseDir = (importer == entryPath)
//                 ? std::filesystem::path(entryPath)
//                 : std::filesystem::path(importer).parent_path();

//             std::filesystem::path relativePath = std::filesystem::absolute(baseDir / candidate);
//             if (std::filesystem::exists(relativePath)) {
//                 return relativePath.lexically_normal().string();
//             }

//             return "";
//         } catch (...) {
//             return "";
//         }
//     };

//     // Try to resolve as native library first
//     std::string libPath = resolveLibraryPath(modulePath, importerPath);

//     if (!libPath.empty()) {
//         void* handle = nullptr;
// #if defined(_WIN32)
//         handle = (void*)LoadLibraryA(libPath.c_str());
// #else
//         dlerror();
//         handle = dlopen(libPath.c_str(), RTLD_LAZY);
// #endif
//         if (!handle) {
//             std::string detail = platformLastError();
//             Diagnostic d(RuntimeError::NativeLibraryLoadFailed, libPath, std::vector<std::string>{ libPath, detail });
//             return Expected<Module, Diagnostic>(d);
//         }

//         using NativeFunction = Module(*)(MeowEngine*);
//         NativeFunction factory = nullptr;

// #if defined(_WIN32)
//         FARPROC procAddress = GetProcAddress((HMODULE)handle, "CreateMeowModule");
//         if (procAddress == nullptr) {
//             std::string detail = platformLastError();
//             Diagnostic d(RuntimeError::NativeLibrarySymbolNotFound, libPath, std::vector<std::string>{ "CreateMeowModule", libPath, detail });
//             return Expected<Module, Diagnostic>(d);
//         }
//         factory = reinterpret_cast<NativeFunction>(procAddress);
// #else
//         dlerror();
//         void* sym = dlsym(handle, "CreateMeowModule");
//         const char* derr = dlerror();
//         if (sym == nullptr || derr != nullptr) {
//             std::string detail = platformLastError();
//             Diagnostic d(RuntimeError::NativeLibrarySymbolNotFound, libPath, std::vector<std::string>{ "CreateMeowModule", libPath, detail });
//             return Expected<Module, Diagnostic>(d);
//         }
//         factory = reinterpret_cast<NativeFunction>(sym);
// #endif

//         // call factory to create Module
//         Module nativeModule = nullptr;
//         try {
//             nativeModule = factory(engine);
//         } catch (...) {
//             // If factory throws, convert to Diagnostic
//             Diagnostic d(RuntimeError::NativeLibraryLoadFailed, libPath, std::vector<std::string>{ libPath, "factory-threw" });
//             return Expected<Module, Diagnostic>(d);
//         }

//         // cache both the requested modulePath and physical libPath
//         moduleCache[modulePath] = nativeModule;
//         moduleCache[libPath] = nativeModule;

//         return Expected<Module, Diagnostic>(nativeModule);
//     }

//     // Not native library (or not found as native). Resolve relative/absolute path for meow module
//     std::filesystem::path baseDir = (importerPath == entryPath)
//         ? std::filesystem::path(entryPath)
//         : std::filesystem::path(importerPath).parent_path();
//     std::filesystem::path resolvedPath = std::filesystem::absolute(baseDir / modulePath);
//     std::string absolutePath = resolvedPath.lexically_normal().string();

//     if (auto it = moduleCache.find(absolutePath); it != moduleCache.end()) {
//         return it->second;
//     }

//     // parse module (binary/text). Parsers now return Expected<Bool, Diagnostic>
//     std::unordered_map<std::string, Proto> protosLocal;
//     if (isBinary) {
//         // auto r = binaryParser.parseFile(absolutePath, *memoryManager);
//         // if (!r) {
//         //     // pass through the Diagnostic from parser but mark error kind as ModuleLoadFailed
//         //     Diagnostic diag = r.error();
//         //     // wrap/augment diag if desired; here we create a ModuleLoadFailed to be consistent
//         //     Diagnostic out(RuntimeError::ModuleLoadFailed, absolutePath, std::vector<std::string>{ absolutePath });
//         //     // Prefer the parser's detailed diag args if present
//         //     if (!diag.args.empty()) out.args = diag.args;
//         //     return Expected<Module, Diagnostic>(out);
//         // }
//         // protosLocal = binaryParser.protos;
//     } else {
//         auto r = textParser.parseFile(absolutePath, *memoryManager);
//         if (!r) {
//             Diagnostic diag = r.error();
//             Diagnostic out(RuntimeError::ModuleLoadFailed, absolutePath, std::vector<std::string>{ absolutePath });
//             if (!diag.args.empty()) out.args = diag.args;
//             return Expected<Module, Diagnostic>(out);
//         }
//         protosLocal = textParser.protos;
//     }

//     const std::string mainName = "@main";
//     auto pit = protosLocal.find(mainName);
//     if (pit == protosLocal.end()) {
//         Diagnostic d(RuntimeError::MissingMainFunction, absolutePath, std::vector<std::string>{ modulePath, mainName });
//         return Expected<Module, Diagnostic>(d);
//     }

//     // create module object
//     Module newModule = nullptr;
//     try {
//         newModule = memoryManager->newObject<ObjModule>(modulePath, absolutePath, isBinary);
//     } catch (...) {
//         // Diagnostic d(RuntimeError::ModuleLoadFailed, absolutePath, std::vector<std::string>{ "alloc-failed" });
//         // return Expected<Module, Diagnostic>(d);
//     }

//     // newModule->mainProto = pit->second;
//     newModule->setMain(pit->second);
//     // newModule->hasMain = true;

//     // import native "native" module globals if present
//     if (newModule->getName() != "native") {
//         auto itNative = moduleCache.find("native");
//         if (itNative != moduleCache.end()) {
//             for (const auto& [name, func] : itNative->second->globals) {
//                 // newModule->globals[name] = func;
//                 newModule->setGlobal(name, func);
//             }
//         }
//     }

//     // cache and return
//     moduleCache[absolutePath] = newModule;
//     return Expected<Module, Diagnostic>(newModule);
// }
