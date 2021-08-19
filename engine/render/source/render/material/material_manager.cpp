#include "render/material/material_manager.h"
#include "render/frame_graph.h"
#include "render/geometry/geometry.h"
#include "render/material/material.h"
#include "render/texture/texture_manager.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown_reader.h>

namespace kw {

MaterialManager::GraphicsPipelineKey::GraphicsPipelineKey(String&& vertex_shader_, String&& fragment_shader_, bool is_shadow_)
    : vertex_shader(std::move(vertex_shader_))
    , fragment_shader(std::move(fragment_shader_))
    , is_shadow(is_shadow_)
{
}

bool MaterialManager::GraphicsPipelineKey::operator==(const GraphicsPipelineKey& other) const {
    return vertex_shader == other.vertex_shader && fragment_shader == other.fragment_shader && is_shadow == other.is_shadow;
}

size_t MaterialManager::GraphicsPipelineHash::operator()(const GraphicsPipelineKey& value) const {
    return std::hash<String>()(value.vertex_shader) ^ std::hash<String>()(value.fragment_shader) ^ (value.is_shadow ? SIZE_MAX : 0);
}

MaterialManager::GraphicsPipelineContext::GraphicsPipelineContext(MemoryResource& memory_resource)
    : textures(memory_resource)
{
}

class MaterialManager::GraphicsPipelineTask : public Task {
public:
    GraphicsPipelineTask(MaterialManager& manager, GraphicsPipelineContext& graphics_pipeline_context, const String& vertex_shader, const String& fragment_shader, bool is_shadow)
        : m_manager(manager)
        , m_graphics_pipeline_context(graphics_pipeline_context)
        , m_vertex_shader(vertex_shader)
        , m_fragment_shader(fragment_shader)
        , m_is_shadow(is_shadow)
    {
    }

    void run() override {
        if (m_graphics_pipeline_context.is_particle) {
            create_particle();
        } else {
            create_geometry();
        }
    }

    const char* get_name() const override {
        return "Material Manager: Graphics Pipeline";
    }

private:
    void create_geometry() {
        AttributeDescriptor vertex_attribute_descriptors[4]{};
        vertex_attribute_descriptors[0].semantic = Semantic::POSITION;
        vertex_attribute_descriptors[0].format = TextureFormat::RGB32_FLOAT;
        vertex_attribute_descriptors[0].offset = offsetof(Geometry::Vertex, position);
        vertex_attribute_descriptors[1].semantic = Semantic::NORMAL;
        vertex_attribute_descriptors[1].format = TextureFormat::RGB32_FLOAT;
        vertex_attribute_descriptors[1].offset = offsetof(Geometry::Vertex, normal);
        vertex_attribute_descriptors[2].semantic = Semantic::TANGENT;
        vertex_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
        vertex_attribute_descriptors[2].offset = offsetof(Geometry::Vertex, tangent);
        vertex_attribute_descriptors[3].semantic = Semantic::TEXCOORD;
        vertex_attribute_descriptors[3].format = TextureFormat::RG32_FLOAT;
        vertex_attribute_descriptors[3].offset = offsetof(Geometry::Vertex, texcoord_0);

        AttributeDescriptor joint_attribute_descriptors[2]{};
        joint_attribute_descriptors[0].semantic = Semantic::JOINTS;
        joint_attribute_descriptors[0].format = TextureFormat::RGBA8_UINT;
        joint_attribute_descriptors[0].offset = offsetof(Geometry::SkinnedVertex, joints);
        joint_attribute_descriptors[1].semantic = Semantic::WEIGHTS;
        joint_attribute_descriptors[1].format = TextureFormat::RGBA8_UNORM;
        joint_attribute_descriptors[1].offset = offsetof(Geometry::SkinnedVertex, weights);

        // Only the first binding will be used for solid geometry.
        BindingDescriptor binding_descriptors[2]{};
        binding_descriptors[0].attribute_descriptors = vertex_attribute_descriptors;
        binding_descriptors[0].attribute_descriptor_count = std::size(vertex_attribute_descriptors);
        binding_descriptors[0].stride = sizeof(Geometry::Vertex);
        binding_descriptors[1].attribute_descriptors = joint_attribute_descriptors;
        binding_descriptors[1].attribute_descriptor_count = std::size(joint_attribute_descriptors);
        binding_descriptors[1].stride = sizeof(Geometry::SkinnedVertex);

        AttributeDescriptor instance_attribute_descriptors[8]{};
        instance_attribute_descriptors[0].semantic = Semantic::POSITION;
        instance_attribute_descriptors[0].semantic_index = 1;
        instance_attribute_descriptors[0].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[0].offset = offsetof(Material::GeometryInstanceData, model) + offsetof(float4x4, _r0);
        instance_attribute_descriptors[1].semantic = Semantic::POSITION;
        instance_attribute_descriptors[1].semantic_index = 2;
        instance_attribute_descriptors[1].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[1].offset = offsetof(Material::GeometryInstanceData, model) + offsetof(float4x4, _r1);
        instance_attribute_descriptors[2].semantic = Semantic::POSITION;
        instance_attribute_descriptors[2].semantic_index = 3;
        instance_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[2].offset = offsetof(Material::GeometryInstanceData, model) + offsetof(float4x4, _r2);
        instance_attribute_descriptors[3].semantic = Semantic::POSITION;
        instance_attribute_descriptors[3].semantic_index = 4;
        instance_attribute_descriptors[3].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[3].offset = offsetof(Material::GeometryInstanceData, model) + offsetof(float4x4, _r3);
        instance_attribute_descriptors[4].semantic = Semantic::POSITION;
        instance_attribute_descriptors[4].semantic_index = 5;
        instance_attribute_descriptors[4].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[4].offset = offsetof(Material::GeometryInstanceData, inverse_transpose_model) + offsetof(float4x4, _r0);
        instance_attribute_descriptors[5].semantic = Semantic::POSITION;
        instance_attribute_descriptors[5].semantic_index = 6;
        instance_attribute_descriptors[5].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[5].offset = offsetof(Material::GeometryInstanceData, inverse_transpose_model) + offsetof(float4x4, _r1);
        instance_attribute_descriptors[6].semantic = Semantic::POSITION;
        instance_attribute_descriptors[6].semantic_index = 7;
        instance_attribute_descriptors[6].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[6].offset = offsetof(Material::GeometryInstanceData, inverse_transpose_model) + offsetof(float4x4, _r2);
        instance_attribute_descriptors[7].semantic = Semantic::POSITION;
        instance_attribute_descriptors[7].semantic_index = 8;
        instance_attribute_descriptors[7].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[7].offset = offsetof(Material::GeometryInstanceData, inverse_transpose_model) + offsetof(float4x4, _r3);

        // Won't be used for skinned geometry and for shadow render pass.
        BindingDescriptor instance_binding_descriptor{};
        instance_binding_descriptor.attribute_descriptors = instance_attribute_descriptors;
        instance_binding_descriptor.attribute_descriptor_count = std::size(instance_attribute_descriptors);
        instance_binding_descriptor.stride = sizeof(Material::GeometryInstanceData);

        AttributeDescriptor shadow_instance_attribute_descriptors[4]{};
        shadow_instance_attribute_descriptors[0].semantic = Semantic::POSITION;
        shadow_instance_attribute_descriptors[0].semantic_index = 1;
        shadow_instance_attribute_descriptors[0].format = TextureFormat::RGBA32_FLOAT;
        shadow_instance_attribute_descriptors[0].offset = offsetof(Material::ShadowInstanceData, model) + offsetof(float4x4, _r0);
        shadow_instance_attribute_descriptors[1].semantic = Semantic::POSITION;
        shadow_instance_attribute_descriptors[1].semantic_index = 2;
        shadow_instance_attribute_descriptors[1].format = TextureFormat::RGBA32_FLOAT;
        shadow_instance_attribute_descriptors[1].offset = offsetof(Material::ShadowInstanceData, model) + offsetof(float4x4, _r1);
        shadow_instance_attribute_descriptors[2].semantic = Semantic::POSITION;
        shadow_instance_attribute_descriptors[2].semantic_index = 3;
        shadow_instance_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
        shadow_instance_attribute_descriptors[2].offset = offsetof(Material::ShadowInstanceData, model) + offsetof(float4x4, _r2);
        shadow_instance_attribute_descriptors[3].semantic = Semantic::POSITION;
        shadow_instance_attribute_descriptors[3].semantic_index = 4;
        shadow_instance_attribute_descriptors[3].format = TextureFormat::RGBA32_FLOAT;
        shadow_instance_attribute_descriptors[3].offset = offsetof(Material::ShadowInstanceData, model) + offsetof(float4x4, _r3);

        // Won't be used for solid and skinned geometry.
        BindingDescriptor shadow_instance_binding_descriptor{};
        shadow_instance_binding_descriptor.attribute_descriptors = shadow_instance_attribute_descriptors;
        shadow_instance_binding_descriptor.attribute_descriptor_count = std::size(shadow_instance_attribute_descriptors);
        shadow_instance_binding_descriptor.stride = sizeof(Material::ShadowInstanceData);

        // Won't be used for solid geometry and for shadow render pass.
        UniformBufferDescriptor uniform_buffer_descriptor{};
        uniform_buffer_descriptor.variable_name = "GeometryUniform";
        uniform_buffer_descriptor.size = sizeof(Material::UniformData);
        
        // Will be used for skinned geometry on shadow render pass.
        UniformBufferDescriptor shadow_uniform_buffer_descriptor{};
        shadow_uniform_buffer_descriptor.variable_name = "ShadowUniformBuffer";
        shadow_uniform_buffer_descriptor.size = sizeof(Material::ShadowUniformData);

        Vector<UniformTextureDescriptor> uniform_texture_descriptors(m_manager.m_transient_memory_resource);
        uniform_texture_descriptors.reserve(m_graphics_pipeline_context.textures.size());
        for (const String& texture : m_graphics_pipeline_context.textures) {
            UniformTextureDescriptor uniform_texture_descriptor{};
            uniform_texture_descriptor.variable_name = texture.c_str();
            uniform_texture_descriptors.push_back(uniform_texture_descriptor);
        }

        // Keep sampler for shadow render pass too, yet fragment shader is expected to be absent.
        UniformSamplerDescriptor uniform_sampler_descriptor{};
        uniform_sampler_descriptor.variable_name = "sampler_uniform";
        uniform_sampler_descriptor.anisotropy_enable = !m_is_shadow;
        uniform_sampler_descriptor.max_anisotropy = 8.f;
        uniform_sampler_descriptor.max_lod = 15.f;

        String graphics_pipeline_name(m_manager.m_transient_memory_resource);
        graphics_pipeline_name.reserve(m_vertex_shader.size() + m_fragment_shader.size() + 1);
        graphics_pipeline_name += m_vertex_shader;
        if (!m_fragment_shader.empty()) {
            graphics_pipeline_name += "+";
            graphics_pipeline_name += m_fragment_shader;
        }

        GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
        graphics_pipeline_descriptor.graphics_pipeline_name = graphics_pipeline_name.c_str();
        graphics_pipeline_descriptor.render_pass_name = m_is_shadow ? "opaque_shadow_render_pass" : "geometry_render_pass";
        graphics_pipeline_descriptor.vertex_shader_filename = m_vertex_shader.c_str();
        graphics_pipeline_descriptor.fragment_shader_filename = m_fragment_shader.empty() ? nullptr : m_fragment_shader.c_str();
        graphics_pipeline_descriptor.vertex_binding_descriptors = binding_descriptors;
        graphics_pipeline_descriptor.vertex_binding_descriptor_count = m_graphics_pipeline_context.is_skinned ? 2 : 1;
        graphics_pipeline_descriptor.instance_binding_descriptors = m_is_shadow ? &shadow_instance_binding_descriptor : &instance_binding_descriptor;
        graphics_pipeline_descriptor.instance_binding_descriptor_count = m_graphics_pipeline_context.is_skinned ? 0 : 1;
        graphics_pipeline_descriptor.front_face = m_is_shadow ? FrontFace::CLOCKWISE : FrontFace::COUNTER_CLOCKWISE;
        graphics_pipeline_descriptor.depth_bias_constant_factor = m_is_shadow ? 2.f : 0.f;
        graphics_pipeline_descriptor.depth_bias_slope_factor = m_is_shadow ? 1.5f : 0.f;
        graphics_pipeline_descriptor.is_depth_test_enabled = true;
        graphics_pipeline_descriptor.is_depth_write_enabled = true;
        graphics_pipeline_descriptor.depth_compare_op = CompareOp::LESS;
        graphics_pipeline_descriptor.is_stencil_test_enabled = !m_is_shadow;
        graphics_pipeline_descriptor.stencil_write_mask = 0xFF;
        graphics_pipeline_descriptor.front_stencil_op_state.pass_op = StencilOp::REPLACE;
        graphics_pipeline_descriptor.front_stencil_op_state.compare_op = CompareOp::ALWAYS;
        graphics_pipeline_descriptor.uniform_texture_descriptors = uniform_texture_descriptors.data();
        graphics_pipeline_descriptor.uniform_texture_descriptor_count = uniform_texture_descriptors.size();
        graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
        graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
        graphics_pipeline_descriptor.uniform_buffer_descriptors = m_is_shadow ? &shadow_uniform_buffer_descriptor : &uniform_buffer_descriptor;
        graphics_pipeline_descriptor.uniform_buffer_descriptor_count = m_graphics_pipeline_context.is_skinned ? 1 : 0;
        graphics_pipeline_descriptor.push_constants_name = m_is_shadow ? "shadow_push_constants" : "geometry_push_constants";
        graphics_pipeline_descriptor.push_constants_size = m_is_shadow ? sizeof(Material::ShadowPushConstants) : sizeof(Material::GeometryPushConstants);

        *m_graphics_pipeline_context.graphics_pipeline = m_manager.m_frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
    }

    void create_particle() {
        AttributeDescriptor vertex_attribute_descriptors[4]{};
        vertex_attribute_descriptors[0].semantic = Semantic::POSITION;
        vertex_attribute_descriptors[0].format = TextureFormat::RGB32_FLOAT;
        vertex_attribute_descriptors[0].offset = offsetof(Geometry::Vertex, position);
        vertex_attribute_descriptors[1].semantic = Semantic::NORMAL;
        vertex_attribute_descriptors[1].format = TextureFormat::RGB32_FLOAT;
        vertex_attribute_descriptors[1].offset = offsetof(Geometry::Vertex, normal);
        vertex_attribute_descriptors[2].semantic = Semantic::TANGENT;
        vertex_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
        vertex_attribute_descriptors[2].offset = offsetof(Geometry::Vertex, tangent);
        vertex_attribute_descriptors[3].semantic = Semantic::TEXCOORD;
        vertex_attribute_descriptors[3].format = TextureFormat::RG32_FLOAT;
        vertex_attribute_descriptors[3].offset = offsetof(Geometry::Vertex, texcoord_0);

        BindingDescriptor vertex_binding_descriptor{};
        vertex_binding_descriptor.attribute_descriptors = vertex_attribute_descriptors;
        vertex_binding_descriptor.attribute_descriptor_count = std::size(vertex_attribute_descriptors);
        vertex_binding_descriptor.stride = sizeof(Geometry::Vertex);

        AttributeDescriptor instance_attribute_descriptors[6]{};
        instance_attribute_descriptors[0].semantic = Semantic::POSITION;
        instance_attribute_descriptors[0].semantic_index = 1;
        instance_attribute_descriptors[0].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[0].offset = offsetof(Material::ParticleInstanceData, model) + offsetof(float4x4, _r0);
        instance_attribute_descriptors[1].semantic = Semantic::POSITION;
        instance_attribute_descriptors[1].semantic_index = 2;
        instance_attribute_descriptors[1].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[1].offset = offsetof(Material::ParticleInstanceData, model) + offsetof(float4x4, _r1);
        instance_attribute_descriptors[2].semantic = Semantic::POSITION;
        instance_attribute_descriptors[2].semantic_index = 3;
        instance_attribute_descriptors[2].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[2].offset = offsetof(Material::ParticleInstanceData, model) + offsetof(float4x4, _r2);
        instance_attribute_descriptors[3].semantic = Semantic::POSITION;
        instance_attribute_descriptors[3].semantic_index = 4;
        instance_attribute_descriptors[3].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[3].offset = offsetof(Material::ParticleInstanceData, model) + offsetof(float4x4, _r3);
        instance_attribute_descriptors[4].semantic = Semantic::COLOR;
        instance_attribute_descriptors[4].semantic_index = 0;
        instance_attribute_descriptors[4].format = TextureFormat::RGBA32_FLOAT;
        instance_attribute_descriptors[4].offset = offsetof(Material::ParticleInstanceData, color);
        instance_attribute_descriptors[5].semantic = Semantic::TEXCOORD;
        instance_attribute_descriptors[5].semantic_index = 1;
        instance_attribute_descriptors[5].format = TextureFormat::RG32_FLOAT;
        instance_attribute_descriptors[5].offset = offsetof(Material::ParticleInstanceData, uv_translation);

        BindingDescriptor instance_binding_descriptor{};
        instance_binding_descriptor.attribute_descriptors = instance_attribute_descriptors;
        instance_binding_descriptor.attribute_descriptor_count = std::size(instance_attribute_descriptors);
        instance_binding_descriptor.stride = sizeof(Material::ParticleInstanceData);

        AttachmentBlendDescriptor attachment_blend_descriptor{};
        attachment_blend_descriptor.attachment_name = m_is_shadow ? "proxy_color_attachment" : "lighting_attachment";
        attachment_blend_descriptor.source_color_blend_factor = BlendFactor::SOURCE_ALPHA;
        attachment_blend_descriptor.destination_color_blend_factor = BlendFactor::SOURCE_INVERSE_ALPHA;
        attachment_blend_descriptor.color_blend_op = BlendOp::ADD;
        attachment_blend_descriptor.source_alpha_blend_factor = BlendFactor::ONE;
        attachment_blend_descriptor.destination_alpha_blend_factor = BlendFactor::SOURCE_INVERSE_ALPHA;
        attachment_blend_descriptor.alpha_blend_op = BlendOp::ADD;

        Vector<UniformTextureDescriptor> uniform_texture_descriptors(m_manager.m_transient_memory_resource);
        uniform_texture_descriptors.reserve(m_graphics_pipeline_context.textures.size());
        for (const String& texture : m_graphics_pipeline_context.textures) {
            UniformTextureDescriptor uniform_texture_descriptor{};
            uniform_texture_descriptor.variable_name = texture.c_str();
            uniform_texture_descriptors.push_back(uniform_texture_descriptor);
        }

        UniformSamplerDescriptor uniform_sampler_descriptor{};
        uniform_sampler_descriptor.variable_name = "sampler_uniform";
        uniform_sampler_descriptor.max_lod = 15.f;

        String graphics_pipeline_name(m_manager.m_transient_memory_resource);
        graphics_pipeline_name.reserve(m_vertex_shader.size() + m_fragment_shader.size() + 1);
        graphics_pipeline_name += m_vertex_shader;
        graphics_pipeline_name += "+";
        graphics_pipeline_name += m_fragment_shader;

        GraphicsPipelineDescriptor graphics_pipeline_descriptor{};
        graphics_pipeline_descriptor.graphics_pipeline_name = graphics_pipeline_name.c_str();
        graphics_pipeline_descriptor.render_pass_name = m_is_shadow ? "translucent_shadow_render_pass" : "particle_system_render_pass";
        graphics_pipeline_descriptor.vertex_shader_filename = m_vertex_shader.c_str();
        graphics_pipeline_descriptor.fragment_shader_filename = m_fragment_shader.c_str();
        graphics_pipeline_descriptor.vertex_binding_descriptors = &vertex_binding_descriptor;
        graphics_pipeline_descriptor.vertex_binding_descriptor_count = 1;
        graphics_pipeline_descriptor.instance_binding_descriptors = &instance_binding_descriptor;
        graphics_pipeline_descriptor.instance_binding_descriptor_count = 1;
        graphics_pipeline_descriptor.front_face = m_is_shadow ? FrontFace::CLOCKWISE : FrontFace::COUNTER_CLOCKWISE;
        graphics_pipeline_descriptor.is_depth_test_enabled = !m_is_shadow;
        graphics_pipeline_descriptor.depth_compare_op = CompareOp::LESS;
        graphics_pipeline_descriptor.attachment_blend_descriptors = &attachment_blend_descriptor;
        graphics_pipeline_descriptor.attachment_blend_descriptor_count = 1;
        graphics_pipeline_descriptor.uniform_texture_descriptors = uniform_texture_descriptors.data();
        graphics_pipeline_descriptor.uniform_texture_descriptor_count = uniform_texture_descriptors.size();
        graphics_pipeline_descriptor.uniform_sampler_descriptors = &uniform_sampler_descriptor;
        graphics_pipeline_descriptor.uniform_sampler_descriptor_count = 1;
        graphics_pipeline_descriptor.push_constants_name = "particle_system_push_constants";
        graphics_pipeline_descriptor.push_constants_size = sizeof(Material::ParticlePushConstants);

        *m_graphics_pipeline_context.graphics_pipeline = m_manager.m_frame_graph.create_graphics_pipeline(graphics_pipeline_descriptor);
    }

    MaterialManager& m_manager;
    GraphicsPipelineContext& m_graphics_pipeline_context;
    const String& m_vertex_shader;
    const String& m_fragment_shader;
    bool m_is_shadow;
};

class MaterialManager::MaterialTask : public Task {
public:
    MaterialTask(MaterialManager& manager, Material& material, const String& relative_path, Task* graphics_pipeline_end)
        : m_manager(manager)
        , m_material(material)
        , m_relative_path(relative_path)
        , m_graphics_pipeline_end(graphics_pipeline_end)
    {
    }

    void run() override {
        //
        // Load markdown file.
        //

        MarkdownReader reader(m_manager.m_transient_memory_resource, m_relative_path.c_str());

        ObjectNode& material_descriptor = reader[0].as<ObjectNode>();
        StringNode& vertex_shader = material_descriptor["vertex_shader"].as<StringNode>();
        StringNode& fragment_shader = material_descriptor["fragment_shader"].as<StringNode>();
        ObjectNode& textures = material_descriptor["textures"].as<ObjectNode>();
        BooleanNode& is_shadow = material_descriptor["is_shadow"].as<BooleanNode>();
        BooleanNode& is_skinned = material_descriptor["is_skinned"].as<BooleanNode>();
        BooleanNode& is_particle = material_descriptor["is_particle"].as<BooleanNode>();

        //
        // Load material graphics pipeline.
        //

        Vector<String> texture_names(m_manager.m_transient_memory_resource);
        texture_names.reserve(textures.get_size());
        for (const auto& [name, _] : textures) {
            // We won't use `textures` names anymore anywawy.
            texture_names.push_back(std::move(name));
        }

        SharedPtr<GraphicsPipeline*> graphics_pipeline_context = m_manager.load(
            vertex_shader.get_value().c_str(), fragment_shader.get_value().c_str(), texture_names,
            is_shadow.get_value(), is_skinned.get_value(), is_particle.get_value(), m_graphics_pipeline_end
        );

        //
        // Load material textures. 
        //

        Vector<SharedPtr<Texture*>> material_textures(m_manager.m_persistent_memory_resource);
        material_textures.reserve(textures.get_size());

        for (const auto& [_, value] : textures) {
            material_textures.push_back(m_manager.m_texture_manager.load(value->as<StringNode>().get_value().c_str()));
        }

        //
        // Create material.
        //

        m_material = Material(graphics_pipeline_context, std::move(material_textures),
                              is_shadow.get_value(), is_skinned.get_value(), is_particle.get_value());
    }

    const char* get_name() const override {
        return "Material Manager: Material";
    }

private:
    MaterialManager& m_manager;
    Material& m_material;
    const String& m_relative_path;
    Task* m_graphics_pipeline_end;
};

class MaterialManager::BeginTask : public Task {
public:
    BeginTask(MaterialManager& manager, Task* material_end_task, Task* graphics_pipeline_end_task)
        : m_manager(manager)
        , m_material_end_task(material_end_task)
        , m_graphics_pipeline_end_task(graphics_pipeline_end_task)
    {
    }

    void run() override {
        // Tasks that load materials are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_materials_mutex);

        //
        // Start loading brand new materials.
        //

        for (auto& [relative_path, material] : m_manager.m_pending_materials) {
            MaterialTask* material_task = m_manager.m_transient_memory_resource.construct<MaterialTask>(m_manager, *material, relative_path, m_graphics_pipeline_end_task);
            KW_ASSERT(material_task != nullptr);

            material_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_material_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, material_task);
        }

        m_manager.m_pending_materials.clear();

        //
        // Destroy materials that only referenced from `MaterialManager`.
        //

        for (auto it = m_manager.m_materials.begin(); it != m_manager.m_materials.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_materials.erase(it);
            } else {
                ++it;
            }
        }

        //
        // Destroy graphics pipelines that only referenced from `MaterialManager`.
        //

        for (auto it = m_manager.m_graphics_pipelines.begin(); it != m_manager.m_graphics_pipelines.end(); ) {
            if (it->second.graphics_pipeline.use_count() == 1) {
                m_manager.m_frame_graph.destroy_graphics_pipeline(*it->second.graphics_pipeline);
                it = m_manager.m_graphics_pipelines.erase(it);
            } else {
                ++it;
            }
        }
    }

    const char* get_name() const override {
        return "Material Manager Begin";
    }

private:
    MaterialManager& m_manager;
    Task* m_material_end_task;
    Task* m_graphics_pipeline_end_task;
};

MaterialManager::MaterialManager(const MaterialManagerDescriptor& descriptor)
    : m_frame_graph(*descriptor.frame_graph)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_texture_manager(*descriptor.texture_manager)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_graphics_pipelines(*descriptor.persistent_memory_resource)
    , m_materials(*descriptor.persistent_memory_resource)
    , m_pending_materials(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.frame_graph != nullptr, "Invalid frame graph.");
    KW_ASSERT(descriptor.task_scheduler != nullptr, "Invalid task scheduler.");
    KW_ASSERT(descriptor.texture_manager != nullptr, "Invalid texture manager.");
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ASSERT(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");

    m_graphics_pipelines.reserve(8);
    m_materials.reserve(16);
    m_pending_materials.reserve(16);
}

MaterialManager::~MaterialManager() {
    m_pending_materials.clear();

    for (auto& [_, material] : m_materials) {
        KW_ASSERT(material.use_count() == 1, "Not all materials are released.");
    }
    m_materials.clear();

    for (const auto& [_, graphics_pipeline_context] : m_graphics_pipelines) {
        KW_ASSERT(
            graphics_pipeline_context.graphics_pipeline.use_count() == 1,
            "Not all graphics pipelines are released."
        );
        
        m_frame_graph.destroy_graphics_pipeline(*graphics_pipeline_context.graphics_pipeline);
    }
}

SharedPtr<Material> MaterialManager::load(const char* relative_path) {
    {
        std::shared_lock shared_lock(m_materials_mutex);

        auto it = m_materials.find(String(relative_path, m_transient_memory_resource));
        if (it != m_materials.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_materials_mutex);

        auto [it, success] = m_materials.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<Material>());
        if (!success) {
            // Could return here if materials is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<Material>(m_persistent_memory_resource);

        m_pending_materials.emplace_back(it->first, it->second);

        return it->second;
    }
}

MaterialManagerTasks MaterialManager::create_tasks() {
    Task* material_end_task = m_transient_memory_resource.construct<NoopTask>("Material Manager Material End");
    Task* graphics_pipeline_end_task = m_transient_memory_resource.construct<NoopTask>("Material Manager Graphics Pipeline End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, material_end_task, graphics_pipeline_end_task);

    return { begin_task, material_end_task, graphics_pipeline_end_task };
}

SharedPtr<GraphicsPipeline*> MaterialManager::load(const char* vertex_shader, const char* fragment_shader, const Vector<String>& textures,
                                                   bool is_shadow, bool is_skinned, bool is_particle, Task* graphics_pipeline_end) {
    {
        std::shared_lock shared_lock(m_graphics_pipelines_mutex);

        auto it = m_graphics_pipelines.find(GraphicsPipelineKey(
            String(vertex_shader, m_transient_memory_resource),
            String(fragment_shader, m_transient_memory_resource),
            is_shadow
        ));

        if (it != m_graphics_pipelines.end()) {
            KW_ERROR(
                it->second.is_skinned == is_skinned,
                "The same graphics pipeline is queried with different is_skinned values."
            );

            KW_ERROR(
                it->second.is_particle == is_particle,
                "The same graphics pipeline is queried with different is_particle values."
            );

            KW_ERROR(
                it->second.textures.size() == textures.size(),
                "The same graphics pipeline is queried with different texture count."
            );

            for (size_t i = 0; i < textures.size(); i++) {
                KW_ERROR(
                    it->second.textures[i] == textures[i],
                    "The same graphics pipeline is queried with different textures."
                );
            }

            return it->second.graphics_pipeline;
        }
    }

    {
        std::lock_guard lock_guard(m_graphics_pipelines_mutex);

        auto [it, success] = m_graphics_pipelines.emplace(GraphicsPipelineKey(
            String(vertex_shader, m_persistent_memory_resource),
            String(fragment_shader, m_persistent_memory_resource),
            is_shadow
        ), GraphicsPipelineContext(m_persistent_memory_resource));

        if (!success) {
            // Could return here if material is enqueued from multiple threads.

            KW_ERROR(
                it->second.is_skinned == is_skinned,
                "The same graphics pipeline is queried with different is_skinned values."
            );

            KW_ERROR(
                it->second.is_particle == is_particle,
                "The same graphics pipeline is queried with different is_particle values."
            );

            KW_ERROR(
                it->second.textures.size() == textures.size(),
                "The same graphics pipeline is queried with different texture count."
            );

            for (size_t i = 0; i < textures.size(); i++) {
                KW_ERROR(
                    it->second.textures[i] == textures[i],
                    "The same graphics pipeline is queried with different textures."
                );
            }

            return it->second.graphics_pipeline;
        }

        it->second.graphics_pipeline = allocate_shared<GraphicsPipeline*>(m_persistent_memory_resource);
        it->second.textures.reserve(textures.size());
        for (const String& texture : textures) {
            it->second.textures.push_back(String(texture, m_persistent_memory_resource));
        }
        it->second.is_skinned = is_skinned;
        it->second.is_particle = is_particle;

        GraphicsPipelineTask* material_task = m_transient_memory_resource.construct<GraphicsPipelineTask>(*this, it->second, it->first.vertex_shader, it->first.fragment_shader, it->first.is_shadow);
        material_task->add_output_dependencies(m_transient_memory_resource, { graphics_pipeline_end });
        m_task_scheduler.enqueue_task(m_transient_memory_resource, material_task);

        return it->second.graphics_pipeline;
    }
}

} // namespace kw
