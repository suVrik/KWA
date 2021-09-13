#pragma once

#include "core/prefab/prefab_primitive.h"

namespace kw {

struct SceneDescriptor {
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class Scene : public PrefabPrimitive {
public:
    explicit Scene(const SceneDescriptor& scene_descriptor);

protected:
    void child_added(Primitive& primitive) override;
    void child_removed(Primitive& primitive) override;

    MemoryResource& m_transient_memory_resource;
};

} // namespace kw
