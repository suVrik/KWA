#pragma once

#include "core/containers/shared_ptr.h"
#include "core/containers/vector.h"
#include "core/prefab/prefab_prototype_listener.h"
#include "core/scene/primitive.h"

namespace kw {

class ObjectNode;
class PrefabPrototype;
class PrimitiveReflection;

class PrefabPrimitive : public Primitive, public PrefabPrototypeListener {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    explicit PrefabPrimitive(MemoryResource& persistent_memory_resource,
                             SharedPtr<PrefabPrototype> prefab_prototype = nullptr,
                             const transform& local_transform = transform());
    PrefabPrimitive(const PrefabPrimitive& other);
    ~PrefabPrimitive() override;
    PrefabPrimitive& operator=(const PrefabPrimitive& other);

    // When prefab prototype is loaded, the old children are removed (regardless they were
    // from previous prefab prototype or added manually via `add_child` method).
    const SharedPtr<PrefabPrototype>& get_prefab_prototype() const;
    void set_prefab_prototype(SharedPtr<PrefabPrototype> prefab_prototype);

    // Given primitive must not have a parent. Updates child's global transform and bounds.
    void add_child(UniquePtr<Primitive>&& primitive);
    
    template <typename T>
    void add_child(UniquePtr<T>&& primitive) {
        add_child(static_pointer_cast<Primitive>(std::move(primitive)));
    }

    // Same, but pre-allocates memory for all children.
    void add_children(Vector<UniquePtr<Primitive>>& children);

    // Given primitive must be a child of this prefab. Updates child's global transform and bounds.
    UniquePtr<Primitive> remove_child(Primitive& primitive);

    const Vector<UniquePtr<Primitive>>& get_children() const;

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void global_transform_updated() override;
    void prefab_prototype_loaded() override;

    virtual void child_added(Primitive& primitive);
    virtual void child_removed(Primitive& primitive);
    
private:
    Vector<UniquePtr<Primitive>> m_children;
    SharedPtr<PrefabPrototype> m_prefab_prototype;
};

} // namespace kw
