#include "physics/physics_manager.h"

#include <core/debug/log.h>
#include <core/error.h>

#include <PxFoundation.h>
#include <PxMaterial.h>
#include <PxPhysics.h>
#include <PxPhysicsVersion.h>
#include <cooking/PxCooking.h>
#include <extensions/PxDefaultCpuDispatcher.h>
#include <extensions/PxExtensionsAPI.h>
#include <pvd/PxPvd.h>
#include <pvd/PxPvdTransport.h>

namespace kw {

class PhysicsAllocator : public physx::PxAllocatorCallback {
public:
    PhysicsAllocator(MemoryResource& memory_resource)
        : m_memory_resource(memory_resource)
    {
    }

    void* allocate(size_t size, const char* typeName, const char* filename, int line) override {
        return m_memory_resource.allocate(size, 16);
    }

    void deallocate(void* ptr) override {
        m_memory_resource.deallocate(ptr);
    }

private:
    MemoryResource& m_memory_resource;
};

class PhysicsErrorCallback : public physx::PxErrorCallback {
public:
    void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override {
        Log::print("[PHYSICS] %s at %s:%d", message, file, line);
    }
};

PhysicsManager::PhysicsManager(const PhysicsManagerDescriptor& descriptor)
    : m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_error_callback(allocate_unique<PhysicsErrorCallback>(*descriptor.persistent_memory_resource))
    , m_allocator_callback(allocate_unique<PhysicsAllocator>(*descriptor.persistent_memory_resource, *descriptor.persistent_memory_resource))
{
    m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, *m_allocator_callback, *m_error_callback);
    KW_ERROR(m_foundation != nullptr, "Failed to create PhysX foundation.");

    // TODO: Enable/disable PVD on runtime via CVars or on startup via command line?
#ifdef KW_DEBUG
    m_visual_debugger_transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    if (m_visual_debugger_transport != nullptr) {
        m_visual_debugger = physx::PxCreatePvd(*m_foundation);
        if (!m_visual_debugger->connect(*m_visual_debugger_transport, physx::PxPvdInstrumentationFlag::eALL)) {
            Log::print("[PHYSICS] Failed to connect to PhysX Visual Debugger.");
        } else {
            Log::print("[PHYSICS] Successfully connected to PhysX Visual Debugger.");
        }
    } else {
        Log::print("[PHYSICS] Failed to create PhysX Visual Debugger transport.");
    }
#endif // KW_DEBUG

    physx::PxTolerancesScale scale;

    m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, false, m_visual_debugger.get());
    KW_ERROR(m_physics != nullptr, "Failed to create PhysX physics.");

    KW_ERROR(PxInitExtensions(*m_physics, m_visual_debugger.get()), "Failed to init PhysX extensions.");

    physx::PxCookingParams cooking_params(scale);
    m_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation, cooking_params);
    KW_ERROR(m_cooking != nullptr, "Failed to create PhysX cooking.");

    // TODO: Custom CPU dispatcher that makes use of task scheduler.
    m_cpu_dispatcher = physx::PxDefaultCpuDispatcherCreate(0);
    KW_ERROR(m_cpu_dispatcher != nullptr, "Failed to create PhysX CPU dispatcher.");

    // TODO: Allow custom materials per rigid actor.
    m_default_material = m_physics->createMaterial(0.5f, 1.f, 0.1f);
    KW_ERROR(m_default_material != nullptr, "Failed to create default material.");
}

PhysicsManager::~PhysicsManager() = default;

physx::PxPhysics& PhysicsManager::get_physics() const {
    return *m_physics;
}

physx::PxCooking& PhysicsManager::get_cooking() const {
    return *m_cooking;
}

physx::PxCpuDispatcher& PhysicsManager::get_cpu_dispatcher() {
    return *m_cpu_dispatcher;
}

physx::PxMaterial& PhysicsManager::get_default_material() const {
    return *m_default_material;
}

} // namespace kw
