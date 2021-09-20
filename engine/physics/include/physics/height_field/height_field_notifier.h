#pragma once

#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <mutex>

namespace kw {

class HeightField;
class HeightFieldListener;

class HeightFieldNotifier {
public:
    explicit HeightFieldNotifier(MemoryResource& memory_resource);
    ~HeightFieldNotifier() = default;

    void subscribe(HeightField& height_field, HeightFieldListener& height_field_listener);
    void unsubscribe(HeightField& height_field, HeightFieldListener& height_field_listener);

    void notify(HeightField& height_field);

private:
    MemoryResource& m_memory_resource;
    UnorderedMap<HeightField*, Vector<HeightFieldListener*>> m_listeners;
    std::mutex m_mutex;
};

} // namespace kw
