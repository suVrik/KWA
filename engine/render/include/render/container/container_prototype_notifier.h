#pragma once

#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <mutex>

namespace kw {

class ContainerPrototype;
class ContainerPrototypeListener;

class ContainerPrototypeNotifier {
public:
    explicit ContainerPrototypeNotifier(MemoryResource& memory_resource);

    void subscribe(ContainerPrototype& container_prototype, ContainerPrototypeListener& container_prototype_listener);
    void unsubscribe(ContainerPrototype& container_prototype, ContainerPrototypeListener& container_prototype_listener);

    void notify(ContainerPrototype& container_prototype);

private:
    MemoryResource& m_memory_resource;
    UnorderedMap<ContainerPrototype*, Vector<ContainerPrototypeListener*>> m_listeners;
    std::recursive_mutex m_mutex;
};

} // namespace kw
