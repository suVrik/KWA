#pragma once

#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <mutex>

namespace kw {

class Geometry;
class GeometryListener;

class GeometryNotifier {
public:
    explicit GeometryNotifier(MemoryResource& memory_resource);

    void subscribe(Geometry& geometry, GeometryListener& geometry_listener);
    void unsubscribe(Geometry& geometry, GeometryListener& geometry_listener);

    void notify(Geometry& geometry);

private:
    MemoryResource& m_memory_resource;
    UnorderedMap<Geometry*, Vector<GeometryListener*>> m_listeners;
    std::mutex m_mutex;
};

} // namespace kw
