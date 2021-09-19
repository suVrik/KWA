#pragma once

#include <core/math/float2.h>
#include <core/math/float3.h>
#include <core/math/float4.h>
#include <core/math/quaternion.h>
#include <core/math/transform.h>

#include <foundation/PxVec2.h>
#include <foundation/PxVec3.h>
#include <foundation/PxVec4.h>
#include <foundation/PxQuat.h>
#include <foundation/PxTransform.h>
#include <characterkinematic/PxExtended.h>

namespace kw::PhysicsUtils {

inline float2 physx_to_kw(const physx::PxVec2& value) {
    return float2(value.x, value.y);
}

inline float3 physx_to_kw(const physx::PxVec3& value) {
    return float3(value.x, value.y, value.z);
}

inline float4 physx_to_kw(const physx::PxVec4& value) {
    return float4(value.x, value.y, value.z, value.w);
}

inline quaternion physx_to_kw(const physx::PxQuat& value) {
    return quaternion(value.x, value.y, value.z, value.w);
}

inline transform physx_to_kw(const physx::PxTransform& value) {
    return transform(physx_to_kw(value.p), physx_to_kw(value.q));
}

inline float3 physx_extended_to_kw(const physx::PxExtendedVec3& value) {
    return float3(static_cast<float>(value.x),
                  static_cast<float>(value.y),
                  static_cast<float>(value.z));
}

inline physx::PxVec2 kw_to_physx(const float2& value) {
    return physx::PxVec2(value.x, value.y);
}

inline physx::PxVec3 kw_to_physx(const float3& value) {
    return physx::PxVec3(value.x, value.y, value.z);
}

inline physx::PxVec4 kw_to_physx(const float4& value) {
    return physx::PxVec4(value.x, value.y, value.z, value.w);
}

inline physx::PxQuat kw_to_physx(const quaternion& value) {
    return physx::PxQuat(value.x, value.y, value.z, value.w);
}

inline physx::PxTransform kw_to_physx(const transform& value) {
    return physx::PxTransform(kw_to_physx(value.translation), kw_to_physx(value.rotation));
}

inline physx::PxExtendedVec3 kw_to_physx_extended(const float3& value) {
    return physx::PxExtendedVec3(static_cast<physx::PxExtended>(value.x),
                                 static_cast<physx::PxExtended>(value.y),
                                 static_cast<physx::PxExtended>(value.z));
}

} // namespace kw::PhysicsUtils
