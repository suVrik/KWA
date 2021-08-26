#pragma once

#include <core/containers/unique_ptr.h>
#include <core/containers/vector.h>

namespace kw {

class ContainerPrototypeListener;
class ContainerPrototypeNotifier;
class Primitive;

class ContainerPrototype {
public:
    ContainerPrototype(ContainerPrototypeNotifier& container_prototype_notifier,
                       MemoryResource& persistent_memory_resource);
    ContainerPrototype(ContainerPrototypeNotifier& container_prototype_notifier,
                       Vector<UniquePtr<Primitive>>&& primitives);
    ContainerPrototype(ContainerPrototype&& other);
    ~ContainerPrototype();
    ContainerPrototype& operator=(ContainerPrototype&& other);

    // This container prototype listener will be notified when this container prototype is loaded.
    // If this container prototype is already loaded, the listener will be notified immediately.
    void subscribe(ContainerPrototypeListener& container_prototype_listener);
    void unsubscribe(ContainerPrototypeListener& container_prototype_listener);

    const Vector<UniquePtr<Primitive>>& get_primitives() const;

    bool is_loaded() const;

protected:
    ContainerPrototypeNotifier& m_container_prototype_notifier;

    Vector<UniquePtr<Primitive>> m_primitives;
};

} // namespace kw
