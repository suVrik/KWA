#include "core/scene/scene.h"
#include "core/debug/assert.h"

namespace kw {

Scene::Scene(const SceneDescriptor& descriptor)
    : PrefabPrimitive(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
{
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);
}

void Scene::child_added(Primitive& primitive) {
    if (PrefabPrimitive* prefab_primitive = dynamic_cast<PrefabPrimitive*>(&primitive)) {
        const Vector<UniquePtr<Primitive>>& children = prefab_primitive->get_children();
        for (const UniquePtr<Primitive>& child_primitive : children) {
            child_added(*child_primitive);
        }
    }
}

void Scene::child_removed(Primitive& primitive) {
    if (PrefabPrimitive* prefab_primitive = dynamic_cast<PrefabPrimitive*>(&primitive)) {
        const Vector<UniquePtr<Primitive>>& children = prefab_primitive->get_children();
        for (const UniquePtr<Primitive>& child_primitive : children) {
            child_removed(*child_primitive);
        }
    }
}

} // namespace kw
