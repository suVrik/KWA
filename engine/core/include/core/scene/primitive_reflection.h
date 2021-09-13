#pragma once

#include "core/containers/pair.h"
#include "core/containers/string.h"
#include "core/containers/unique_ptr.h"
#include "core/containers/unordered_map.h"

namespace kw {

class PrefabManager;
class ObjectNode;
class Primitive;

struct PrimitiveReflectionDescriptor {
    PrefabManager* prefab_manager;
    MemoryResource* memory_resource;
};

class PrimitiveReflection {
public:
    explicit PrimitiveReflection(const PrimitiveReflectionDescriptor& descriptor);
    virtual ~PrimitiveReflection() = default;

    UniquePtr<Primitive> create_from_markdown(const ObjectNode& node);

    PrefabManager& prefab_manager;
    MemoryResource& memory_resource;

protected:
    UnorderedMap<String, UniquePtr<Primitive>(*)(PrimitiveReflection&, const ObjectNode&)> m_primitives;
};

} // namespace kw
