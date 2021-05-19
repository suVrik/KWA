#include "core/math/float3x3.h"
#include "core/debug/assert.h"
#include "core/math/quaternion.h"

namespace kw {

float3x3::float3x3(const quaternion& quaternion) {
    KW_ASSERT(isfinite(quaternion));

    float xx = quaternion.x * quaternion.x;
    float xy = quaternion.x * quaternion.y;
    float xz = quaternion.x * quaternion.z;
    float xw = quaternion.x * quaternion.w;
    float yy = quaternion.y * quaternion.y;
    float yz = quaternion.y * quaternion.z;
    float yw = quaternion.y * quaternion.w;
    float zz = quaternion.z * quaternion.z;
    float zw = quaternion.z * quaternion.w;

    _m00 = 1.f - 2.f * (yy + zz);
    _m01 = 2.f * (xy + zw);
    _m02 = 2.f * (xz - yw);
    _m10 = 2.f * (xy - zw);
    _m11 = 1.f - 2.f * (xx + zz);
    _m12 = 2.f * (yz + xw);
    _m20 = 2.f * (xz + yw);
    _m21 = 2.f * (yz - xw);
    _m22 = 1.f - 2.f * (xx + yy);
}

} // namespace kw
