#pragma once

#include <core/containers/pair.h>
#include <core/containers/string.h>
#include <core/containers/unique_ptr.h>
#include <core/containers/unordered_map.h>

namespace kw {

class AnimationManager;
class ContainerManager;
class GeometryManager;
class MaterialManager;
class ObjectNode;
class ParticleSystemManager;
class Primitive;
class TextureManager;

struct PrimitiveReflectionDescriptor {
    ObjectNode* primitive_node;
    TextureManager* texture_manager;
    GeometryManager* geometry_manager;
    MaterialManager* material_manager;
    AnimationManager* animation_manager;
    ParticleSystemManager* particle_system_manager;
    ContainerManager* container_manager;
    MemoryResource* persistent_memory_resource;
};

class PrimitiveReflection {
public:
    static PrimitiveReflection& instance();

    UniquePtr<Primitive> create_from_markdown(const PrimitiveReflectionDescriptor& descriptor);

private:
    PrimitiveReflection();

    UnorderedMap<String, UniquePtr<Primitive> (*)(const PrimitiveReflectionDescriptor&)> m_primitives;
};

} // namespace kw
