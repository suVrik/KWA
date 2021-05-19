#pragma once

#include "render/scene/primitive.h"

#include <core/containers/vector.h>

namespace kw {

class ContainerPrimitive final : public Primitive {
public:
    ContainerPrimitive(MemoryResource& persistent_memory_resource);
    ~ContainerPrimitive() override;

    // Given primitive must not have a parent. Updates child's global transform and bounds.
    void add_child(Primitive* primitive);

    // Same, but pre-allocates memory for all children.
    void add_children(const Vector<Primitive*>& children);

    // Given primitive must be a child of this container. Updates child's global transform and bounds.
    void remove_child(Primitive* primitive);

    inline const Vector<Primitive*>& get_children() const {
        return m_children;
    }

protected:
    void global_transform_updated() override;

    Vector<Primitive*> m_children;
};

} // namespace kw