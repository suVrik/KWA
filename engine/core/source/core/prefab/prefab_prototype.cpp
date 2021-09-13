#include "core/prefab/prefab_prototype.h"
#include "core/debug/assert.h"
#include "core/prefab/prefab_prototype_listener.h"
#include "core/prefab/prefab_prototype_notifier.h"
#include "core/scene/primitive.h"

namespace kw {

PrefabPrototype::PrefabPrototype(PrefabPrototypeNotifier& prefab_prototype_notifier, MemoryResource& persistent_memory_resource)
    : m_prefab_prototype_notifier(prefab_prototype_notifier)
    , m_primitives(persistent_memory_resource)
{
}

PrefabPrototype::PrefabPrototype(PrefabPrototypeNotifier& prefab_prototype_notifier, Vector<UniquePtr<Primitive>>&& primitives)
    : m_prefab_prototype_notifier(prefab_prototype_notifier)
    , m_primitives(std::move(primitives))
{
}

PrefabPrototype::PrefabPrototype(PrefabPrototype&& other) = default;
PrefabPrototype::~PrefabPrototype() = default;

PrefabPrototype& PrefabPrototype::operator=(PrefabPrototype&& other) {
    KW_ASSERT(&m_prefab_prototype_notifier == &other.m_prefab_prototype_notifier);
    m_primitives = std::move(other.m_primitives);
    return *this;
}

void PrefabPrototype::subscribe(PrefabPrototypeListener& prefab_prototype_listener) {
    if (is_loaded()) {
        prefab_prototype_listener.prefab_prototype_loaded();
    } else {
        m_prefab_prototype_notifier.subscribe(*this, prefab_prototype_listener);
    }
}

void PrefabPrototype::unsubscribe(PrefabPrototypeListener& prefab_prototype_listener) {
    if (!is_loaded()) {
        m_prefab_prototype_notifier.unsubscribe(*this, prefab_prototype_listener);
    }
}

const Vector<UniquePtr<Primitive>>& PrefabPrototype::get_primitives() const {
    return m_primitives;
}

bool PrefabPrototype::is_loaded() const {
    return !m_primitives.empty();
}

} // namespace kw
