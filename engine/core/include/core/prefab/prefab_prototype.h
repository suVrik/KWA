#pragma once

#include "core/containers/unique_ptr.h"
#include "core/containers/vector.h"

namespace kw {

class PrefabPrototypeListener;
class PrefabPrototypeNotifier;
class Primitive;

class PrefabPrototype {
public:
    PrefabPrototype(PrefabPrototypeNotifier& prefab_prototype_notifier,
                    MemoryResource& persistent_memory_resource);
    PrefabPrototype(PrefabPrototypeNotifier& prefab_prototype_notifier,
                    Vector<UniquePtr<Primitive>>&& primitives);
    PrefabPrototype(PrefabPrototype&& other);
    ~PrefabPrototype();
    PrefabPrototype& operator=(PrefabPrototype&& other);

    // This prefab prototype listener will be notified when this prefab prototype is loaded.
    // If this prefab prototype is already loaded, the listener will be notified immediately.
    void subscribe(PrefabPrototypeListener& prefab_prototype_listener);
    void unsubscribe(PrefabPrototypeListener& prefab_prototype_listener);

    const Vector<UniquePtr<Primitive>>& get_primitives() const;

    bool is_loaded() const;

protected:
    PrefabPrototypeNotifier& m_prefab_prototype_notifier;

    Vector<UniquePtr<Primitive>> m_primitives;
};

} // namespace kw
