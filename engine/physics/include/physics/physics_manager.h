#pragma once

#include "physics/physics_ptr.h"

#include <core/containers/pair.h>
#include <core/containers/unique_ptr.h>

namespace physx {

class PxCooking;
class PxCpuDispatcher;
class PxDefaultCpuDispatcher;
class PxFoundation;
class PxMaterial;
class PxPhysics;
class PxPvd;
class PxPvdTransport;

} // namespace physx

namespace kw {

class PhysicsAllocator;
class PhysicsErrorCallback;

struct PhysicsManagerDescriptor {
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class PhysicsManager {
public:
    explicit PhysicsManager(const PhysicsManagerDescriptor& descriptor);
    ~PhysicsManager();

    physx::PxPhysics& get_physics() const;
    physx::PxCooking& get_cooking() const;
    physx::PxCpuDispatcher& get_cpu_dispatcher();
    physx::PxMaterial& get_default_material() const;

private:
    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UniquePtr<PhysicsErrorCallback> m_error_callback;
    UniquePtr<PhysicsAllocator> m_allocator_callback;
    PhysicsPtr<physx::PxFoundation> m_foundation;
    PhysicsPtr<physx::PxPvdTransport> m_visual_debugger_transport;
    PhysicsPtr<physx::PxPvd> m_visual_debugger;
    PhysicsPtr<physx::PxPhysics> m_physics;
    PhysicsPtr<physx::PxCooking> m_cooking;
    PhysicsPtr<physx::PxDefaultCpuDispatcher> m_cpu_dispatcher;
    PhysicsPtr<physx::PxMaterial> m_default_material;
};

} // namespace kw
