#pragma once

#include "render/container/container_prototype_listener.h"
#include "render/scene/primitive.h"

#include <core/containers/shared_ptr.h>
#include <core/containers/vector.h>

namespace kw {

class ContainerPrototype;
struct PrimitiveReflectionDescriptor;

class ContainerPrimitive : public Primitive, public ContainerPrototypeListener {
public:
    static UniquePtr<Primitive> create_from_markdown(const PrimitiveReflectionDescriptor& primitive_reflection_descriptor);

    explicit ContainerPrimitive(MemoryResource& persistent_memory_resource,
                                SharedPtr<ContainerPrototype> container_prototype = nullptr,
                                const transform& local_transform = transform());
    ContainerPrimitive(const ContainerPrimitive& other);
    ~ContainerPrimitive() override;
    ContainerPrimitive& operator=(const ContainerPrimitive& other);

    // When container prototype is loaded, the old children are removed (regardless they were
    // from previous container prototype or added manually via `add_child` method).
    const SharedPtr<ContainerPrototype>& get_container_prototype() const;
    void set_container_prototype(SharedPtr<ContainerPrototype> container_prototype);

    // Given primitive must not have a parent. Updates child's global transform and bounds.
    void add_child(UniquePtr<Primitive>&& primitive);
    
    template <typename T>
    void add_child(UniquePtr<T>&& primitive) {
        add_child(static_pointer_cast<Primitive>(std::move(primitive)));
    }

    // Same, but pre-allocates memory for all children.
    void add_children(Vector<UniquePtr<Primitive>>& children);

    // Given primitive must be a child of this container. Updates child's global transform and bounds.
    UniquePtr<Primitive> remove_child(Primitive& primitive);

    const Vector<UniquePtr<Primitive>>& get_children() const;

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void global_transform_updated() override;
    void container_prototype_loaded() override;

    virtual void child_added(Primitive& primitive);
    virtual void child_removed(Primitive& primitive);
    
private:
    Vector<UniquePtr<Primitive>> m_children;
    SharedPtr<ContainerPrototype> m_container_prototype;
};

} // namespace kw
