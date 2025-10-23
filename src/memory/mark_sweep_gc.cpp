#include "memory/mark_sweep_gc.h"
#include "core/value.h"
#include "runtime/builtin_registry.h"
#include "runtime/execution_context.h"

namespace meow::memory {

MarkSweepGC::~MarkSweepGC() {
    std::cout << "[destroy] Đang xử lí các object khi hủy GC" << std::endl;
    for (auto const& [obj, data] : metadata_) {
        delete obj;
    }
}

void MarkSweepGC::register_object(const meow::core::MeowObject* object) {
    std::cout << "[register] Đang đăng kí object: " << object << std::endl;
    metadata_.emplace(object, GCMetadata{});
}

size_t MarkSweepGC::collect() noexcept {
    std::cout << "[collect] Đang collect các object" << std::endl;

    context_->trace(*this);
    builtins_->trace(*this);

    for (auto it = metadata_.begin(); it != metadata_.end();) {
        const meow::core::MeowObject* object = it->first;
        GCMetadata& data = it->second;

        if (data.is_marked_) {
            data.is_marked_ = false;
            ++it;
        } else {
            delete object;
            it = metadata_.erase(it);
        }
    }

    return metadata_.size();
}

void MarkSweepGC::visit_value(meow::core::param_t value) noexcept {
    if (value.is_object()) mark(value.as_object());
}

void MarkSweepGC::visit_object(const meow::core::MeowObject* object) noexcept { mark(object); }

void MarkSweepGC::mark(const meow::core::MeowObject* object) {
    if (object == nullptr) return;
    auto it = metadata_.find(object);
    if (it == metadata_.end() || it->second.is_marked_) return;
    it->second.is_marked_ = true;
    object->trace(*this);
}

}  // namespace meow::memory