#include "render/blend_tree/blend_tree_manager.h"
#include "render/animation/animation_manager.h"
#include "render/blend_tree/blend_tree.h"
#include "render/blend_tree/nodes/blend_tree_animation_node.h"
#include "render/blend_tree/nodes/blend_tree_lerp_node.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown_reader.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

class BlendTreeManager::PendingTask : public Task {
public:
    PendingTask(BlendTreeManager& manager, BlendTree& blend_tree, const char* relative_path)
        : m_manager(manager)
        , m_blend_tree(blend_tree)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        MarkdownReader reader(m_manager.m_transient_memory_resource, m_relative_path);
        KW_ERROR(reader.get_size() == 1, "Invalid blend tree.");

        m_blend_tree = BlendTree(parse_node(reader[0].as<ObjectNode>()));
    }

    const char* get_name() const override {
        return "Blend Tree Manager Pending";
    }

private:
    UniquePtr<BlendTreeNode> parse_node(ObjectNode& node) {
        StringNode& type = node["type"].as<StringNode>();
        if (type.get_value() == "BlendTreeLerpNode") {
            return static_pointer_cast<BlendTreeNode>(parse_lerp_node(node));
        } else if (type.get_value() == "BlendTreeAnimationNode") {
            return static_pointer_cast<BlendTreeNode>(parse_animation_node(node));
        } else {
            KW_ERROR(false, "Invalid blend tree node type.");
        }
    }

    UniquePtr<BlendTreeLerpNode> parse_lerp_node(ObjectNode& node) {
        StringNode& attribute_node = node["attribute"].as<StringNode>();
        KW_ERROR(!attribute_node.get_value().empty(), "Invalid blend tree. Attribute name must not be empty.");

        String attribute(attribute_node.get_value(), m_manager.m_persistent_memory_resource);

        ObjectNode& children_node = node["children"].as<ObjectNode>();
        KW_ERROR(children_node.get_size() != 0, "Invalid blend tree. At least one child is required.");

        Map<float, UniquePtr<BlendTreeNode>> children(m_manager.m_persistent_memory_resource);

        for (const auto& [key_node, value_node] : children_node) {
            auto [_, success] = children.emplace(key_node->as<NumberNode>().get_value(), parse_node(value_node->as<ObjectNode>()));
            KW_ERROR(success, "Invalid blend tree. Children with the same key are illegal.");
        }

        return allocate_unique<BlendTreeLerpNode>(m_manager.m_persistent_memory_resource, std::move(attribute), std::move(children));
    }

    UniquePtr<BlendTreeAnimationNode> parse_animation_node(ObjectNode& node) {
        StringNode& animation_node = node["animation"].as<StringNode>();
        KW_ERROR(!animation_node.get_value().empty(), "Invalid blend tree. Animation is required.");

        return allocate_unique<BlendTreeAnimationNode>(m_manager.m_persistent_memory_resource, m_manager.m_animation_manager.load(animation_node.get_value().c_str()));
    }

    BlendTreeManager& m_manager;
    BlendTree& m_blend_tree;
    const char* m_relative_path;
};

class BlendTreeManager::BeginTask : public Task {
public:
    BeginTask(BlendTreeManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load blend trees are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_blend_trees_mutex);

        //
        // Destroy blend trees that only referenced from `BlendTreeManager`.
        //

        for (auto it = m_manager.m_blend_trees.begin(); it != m_manager.m_blend_trees.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_blend_trees.erase(it);
            } else {
                ++it;
            }
        }

        //
        // Start loading brand new blend trees.
        //

        for (auto& [relative_path, blend_tree] : m_manager.m_pending_blend_trees) {
            PendingTask* pending_task = m_manager.m_transient_memory_resource.construct<PendingTask>(m_manager, *blend_tree, relative_path.c_str());
            KW_ASSERT(pending_task != nullptr);

            pending_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, pending_task);
        }

        m_manager.m_pending_blend_trees.clear();
    }

    const char* get_name() const override {
        return "Blend Tree Manager Begin";
    }

private:
    BlendTreeManager& m_manager;
    Task* m_end_task;
};

BlendTreeManager::BlendTreeManager(const BlendTreeManagerDescriptor& descriptor)
    : m_animation_manager(*descriptor.animation_manager)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_blend_trees(*descriptor.persistent_memory_resource)
    , m_pending_blend_trees(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.animation_manager != nullptr, "Invalid animation manager.");
    KW_ASSERT(descriptor.task_scheduler != nullptr, "Invalid task scheduler.");
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ASSERT(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");

    m_blend_trees.reserve(32);
    m_pending_blend_trees.reserve(32);
}

BlendTreeManager::~BlendTreeManager() {
    m_pending_blend_trees.clear();

    for (auto& [relative_path, blend_tree] : m_blend_trees) {
        KW_ASSERT(blend_tree.use_count() == 1, "Not all blend trees are released.");
    }
}

SharedPtr<BlendTree> BlendTreeManager::load(const char* relative_path) {
    if (relative_path[0] == '\0') {
        // Empty string is allowed.
        return nullptr;
    }

    {
        std::shared_lock shared_lock(m_blend_trees_mutex);

        auto it = m_blend_trees.find(String(relative_path, m_transient_memory_resource));
        if (it != m_blend_trees.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_blend_trees_mutex);

        auto [it, success] = m_blend_trees.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<BlendTree>());
        if (!success) {
            // Could return here if blend tree is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<BlendTree>(m_persistent_memory_resource);

        m_pending_blend_trees.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& BlendTreeManager::get_relative_path(const SharedPtr<BlendTree>& blend_tree) const {
    for (auto& [relative_path, stored_blend_tree] : m_blend_trees) {
        if (blend_tree == stored_blend_tree) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> BlendTreeManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Blend Tree Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
