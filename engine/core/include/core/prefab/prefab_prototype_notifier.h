#pragma once

#include "core/containers/unordered_map.h"
#include "core/containers/vector.h"

#include <mutex>

namespace kw {

class PrefabPrototype;
class PrefabPrototypeListener;

class PrefabPrototypeNotifier {
public:
    explicit PrefabPrototypeNotifier(MemoryResource& memory_resource);

    void subscribe(PrefabPrototype& prefab_prototype, PrefabPrototypeListener& prefab_prototype_listener);
    void unsubscribe(PrefabPrototype& prefab_prototype, PrefabPrototypeListener& prefab_prototype_listener);

    void notify(PrefabPrototype& prefab_prototype);

private:
    MemoryResource& m_memory_resource;
    UnorderedMap<PrefabPrototype*, Vector<PrefabPrototypeListener*>> m_listeners;
    std::recursive_mutex m_mutex;
};

} // namespace kw
