#include "core/io/markdown_utils.h"

namespace kw::MarkdownUtils {

float2 float2_from_markdown(MarkdownNode& node) {
    ArrayNode& array_node = node.as<ArrayNode>();

    return float2(
        static_cast<float>(array_node[0].as<NumberNode>().get_value()),
        static_cast<float>(array_node[1].as<NumberNode>().get_value())
    );
}

float3 float3_from_markdown(MarkdownNode& node) {
    ArrayNode& array_node = node.as<ArrayNode>();

    return float3(
        static_cast<float>(array_node[0].as<NumberNode>().get_value()),
        static_cast<float>(array_node[1].as<NumberNode>().get_value()),
        static_cast<float>(array_node[2].as<NumberNode>().get_value())
    );
}

float4 float4_from_markdown(MarkdownNode& node) {
    ArrayNode& array_node = node.as<ArrayNode>();

    return float4(
        static_cast<float>(array_node[0].as<NumberNode>().get_value()),
        static_cast<float>(array_node[1].as<NumberNode>().get_value()),
        static_cast<float>(array_node[2].as<NumberNode>().get_value()),
        static_cast<float>(array_node[3].as<NumberNode>().get_value())
    );
}

quaternion quaternion_from_markdown(MarkdownNode& node) {
    ArrayNode& array_node = node.as<ArrayNode>();

    return quaternion(
        static_cast<float>(array_node[0].as<NumberNode>().get_value()),
        static_cast<float>(array_node[1].as<NumberNode>().get_value()),
        static_cast<float>(array_node[2].as<NumberNode>().get_value()),
        static_cast<float>(array_node[3].as<NumberNode>().get_value())
    );
}

transform transform_from_markdown(MarkdownNode& node) {
    ObjectNode& object_node = node.as<ObjectNode>();

    return transform(
        float3_from_markdown(object_node["translation"]),
        quaternion_from_markdown(object_node["rotation"]),
        float3_from_markdown(object_node["scale"])
    );
}

aabbox aabbox_from_markdown(MarkdownNode& node) {
    ObjectNode& object_node = node.as<ObjectNode>();

    return aabbox(
        float3_from_markdown(object_node["center"]),
        float3_from_markdown(object_node["extent"])
    );
}

} // namespace kw::MarkdownUtils
