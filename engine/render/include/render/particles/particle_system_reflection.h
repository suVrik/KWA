#pragma once

#include <core/containers/pair.h>
#include <core/containers/string.h>
#include <core/containers/vector.h>

namespace kw {

class GeometryManager;
class MaterialManager;
class ObjectNode;
class ParticleSystemDescriptor;
class ParticleSystemEmitter;
class ParticleSystemGenerator;
class ParticleSystemNotifier;
class ParticleSystemUpdater;

struct ParticleSystemReflectionDescriptor {
    ObjectNode* particle_system_node;
    ParticleSystemNotifier* particle_system_notifier;
    GeometryManager* geometry_manager;
    MaterialManager* material_manager;
    MemoryResource* persistent_memory_resource;
};

class ParticleSystemReflection {
public:
    static ParticleSystemReflection& instance();

    ParticleSystemDescriptor create_from_markdown(const ParticleSystemReflectionDescriptor& descriptor);

private:
    ParticleSystemReflection();

    Vector<Pair<String, ParticleSystemEmitter* (*)(MemoryResource& memory_resource, ObjectNode& node)>> m_emitters;
    Vector<Pair<String, ParticleSystemGenerator* (*)(MemoryResource& memory_resource, ObjectNode& node)>> m_generators;
    Vector<Pair<String, ParticleSystemUpdater* (*)(MemoryResource& memory_resource, ObjectNode& node)>> m_updaters;
};

} // namespace kw
