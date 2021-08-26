#pragma once

#include "core/io/markdown.h"
#include "core/math/aabbox.h"
#include "core/math/transform.h"

namespace kw::MarkdownUtils {

float2 float2_from_markdown(MarkdownNode& node);
float3 float3_from_markdown(MarkdownNode& node);
float4 float4_from_markdown(MarkdownNode& node);
quaternion quaternion_from_markdown(MarkdownNode& node);
transform transform_from_markdown(MarkdownNode& node);
aabbox aabbox_from_markdown(MarkdownNode& node);

} // namespace kw::MarkdownUtils
