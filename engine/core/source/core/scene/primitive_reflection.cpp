#include "core/scene/primitive_reflection.h"
#include "core/debug/assert.h"
#include "core/error.h"
#include "core/io/markdown.h"
#include "core/prefab/prefab_primitive.h"

namespace kw {

PrimitiveReflection::PrimitiveReflection(const PrimitiveReflectionDescriptor& descriptor)
    : memory_resource(*descriptor.memory_resource)
    , prefab_manager(*descriptor.prefab_manager)
    , m_primitives(*descriptor.memory_resource)
{
    KW_ASSERT(descriptor.memory_resource != nullptr, "Invalid memory resource.");
    KW_ASSERT(descriptor.prefab_manager != nullptr, "Invalid prefab manager.");

    m_primitives.emplace(String("PrefabPrimitive", *descriptor.memory_resource), &PrefabPrimitive::create_from_markdown);
}

UniquePtr<Primitive> PrimitiveReflection::create_from_markdown(const ObjectNode& node) {
    const StringNode& type_name = node["type"].as<StringNode>();

    auto it = m_primitives.find(type_name.get_value());
    KW_ERROR(it != m_primitives.end(), "Invalid primitive type.");

    return it->second(*this, node);
}

} // namespace kw
