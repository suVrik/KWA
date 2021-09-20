#include "render/motion/motion_graph_manager.h"
#include "render/blend_tree/blend_tree_manager.h"
#include "render/motion/motion_graph.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown_reader.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

class MotionGraphManager::PendingTask : public Task {
public:
    PendingTask(MotionGraphManager& manager, MotionGraph& motion_graph, const char* relative_path)
        : m_manager(manager)
        , m_motion_graph(motion_graph)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        MarkdownReader reader(m_manager.m_transient_memory_resource, m_relative_path);
        KW_ERROR(reader.get_size() == 1, "Invalid motion graph.");

        ObjectNode& root_node = reader[0].as<ObjectNode>();
        StringNode& default_node = root_node["default"].as<StringNode>();
        ObjectNode& motions_node = root_node["motions"].as<ObjectNode>();
        ArrayNode& transitions_node = root_node["transitions"].as<ArrayNode>();

        Vector<MotionGraph::Motion> motions(m_manager.m_persistent_memory_resource);
        Vector<MotionGraph::Transition> transitions(m_manager.m_persistent_memory_resource);
        UnorderedMap<String, uint32_t> mapping(m_manager.m_persistent_memory_resource);

        motions.reserve(motions_node.get_size());
        transitions.reserve(transitions_node.get_size());
        mapping.reserve(motions_node.get_size());

        for (const auto& [key_node, value_node] : motions_node) {
            StringNode& key_string_node = key_node->as<StringNode>();
            ObjectNode& value_object_node = value_node->as<ObjectNode>();

            StringNode& blend_tree_node = value_object_node["blend_tree"].as<StringNode>();
            NumberNode& duration_node = value_object_node["duration"].as<NumberNode>();

            String key(key_string_node.get_value(), m_manager.m_persistent_memory_resource);
            KW_ERROR(!key.empty(), "Invalid motion graph. Motion name must not be empty.");

            auto [_, success] = mapping.emplace(std::move(key), static_cast<uint32_t>(motions.size()));
            KW_ERROR(success, "Invalid motion graph. Motions with the same name are illegal.");

            motions.push_back(MotionGraph::Motion{
                m_manager.m_blend_tree_manager.load(blend_tree_node.get_value().c_str()),
                Vector<uint32_t>(m_manager.m_persistent_memory_resource),
                duration_node.get_value()
            });
        }

        for (const UniquePtr<MarkdownNode>& transition_node : transitions_node) {
            ObjectNode& transition_object_node = transition_node->as<ObjectNode>();

            ArrayNode& sources_node = transition_object_node["sources"].as<ArrayNode>();
            StringNode& destination_node = transition_object_node["destination"].as<StringNode>();
            NumberNode& duration_node = transition_object_node["duration"].as<NumberNode>();
            StringNode& trigger_event_node = transition_object_node["trigger_event"].as<StringNode>();

            auto it1 = mapping.find(destination_node.get_value());
            KW_ERROR(it1 != mapping.end(), "Invalid motion graph. Destination node is not found.");

            String trigger_event(trigger_event_node.get_value(), m_manager.m_persistent_memory_resource);
            KW_ERROR(!trigger_event.empty(), "Invalid motion graph. Trigger event name must not be empty.");

            uint32_t destination_index = it1->second;

            for (const UniquePtr<MarkdownNode>& source_node : sources_node) {
                StringNode& source_string_node = source_node->as<StringNode>();
                
                auto it2 = mapping.find(source_string_node.get_value());
                KW_ERROR(it2 != mapping.end(), "Invalid motion graph. Source node is not found.");

                uint32_t source_index = it2->second;

                for (uint32_t transition_index : motions[source_index].transitions) {
                    KW_ERROR(
                        transitions[transition_index].destination != destination_index,
                        "Invalid motion graph. Only one transition from one motion to another is allowed."
                    );
                }

                motions[source_index].transitions.push_back(static_cast<uint32_t>(transitions.size()));
            }

            transitions.push_back(MotionGraph::Transition{
                destination_index,
                duration_node.get_value(),
                std::move(trigger_event)
            });
        }

        auto it = mapping.find(default_node.get_value());
        KW_ERROR(it != mapping.end(), "Invalid motion graph. Invalid default motion.");

        m_motion_graph = MotionGraph(std::move(motions), std::move(transitions), std::move(mapping), it->second);
    }

    const char* get_name() const override {
        return "Motion Graph Manager Pending";
    }

private:
    MotionGraphManager& m_manager;
    MotionGraph& m_motion_graph;
    const char* m_relative_path;
};

class MotionGraphManager::BeginTask : public Task {
public:
    BeginTask(MotionGraphManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load motion graphs are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_motion_graphs_mutex);

        //
        // Destroy motion graphs that only referenced from `MotionGraphManager`.
        //

        for (auto it = m_manager.m_motion_graphs.begin(); it != m_manager.m_motion_graphs.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_motion_graphs.erase(it);
            } else {
                ++it;
            }
        }

        //
        // Start loading brand new motion graphs.
        //

        for (auto& [relative_path, motion_graph] : m_manager.m_pending_motion_graphs) {
            PendingTask* pending_task = m_manager.m_transient_memory_resource.construct<PendingTask>(m_manager, *motion_graph, relative_path.c_str());
            KW_ASSERT(pending_task != nullptr);

            pending_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, pending_task);
        }

        m_manager.m_pending_motion_graphs.clear();
    }

    const char* get_name() const override {
        return "Motion Graph Manager Begin";
    }

private:
    MotionGraphManager& m_manager;
    Task* m_end_task;
};

MotionGraphManager::MotionGraphManager(const MotionGraphManagerDescriptor& descriptor)
    : m_blend_tree_manager(*descriptor.blend_tree_manager)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_motion_graphs(*descriptor.persistent_memory_resource)
    , m_pending_motion_graphs(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.blend_tree_manager != nullptr, "Invalid blend tree manager.");
    KW_ASSERT(descriptor.task_scheduler != nullptr, "Invalid task scheduler.");
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ASSERT(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");

    m_motion_graphs.reserve(32);
    m_pending_motion_graphs.reserve(32);
}

MotionGraphManager::~MotionGraphManager() {
    m_pending_motion_graphs.clear();

    for (auto& [relative_path, motion_graph] : m_motion_graphs) {
        KW_ASSERT(motion_graph.use_count() == 1, "Not all motion graphs are released.");
    }
}

SharedPtr<MotionGraph> MotionGraphManager::load(const char* relative_path) {
    if (relative_path[0] == '\0') {
        // Empty string is allowed.
        return nullptr;
    }

    {
        std::shared_lock shared_lock(m_motion_graphs_mutex);

        auto it = m_motion_graphs.find(String(relative_path, m_transient_memory_resource));
        if (it != m_motion_graphs.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_motion_graphs_mutex);

        auto [it, success] = m_motion_graphs.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<MotionGraph>());
        if (!success) {
            // Could return here if motion graph is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<MotionGraph>(m_persistent_memory_resource);

        m_pending_motion_graphs.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& MotionGraphManager::get_relative_path(const SharedPtr<MotionGraph>& motion_graph) const {
    for (auto& [relative_path, stored_motion_graph] : m_motion_graphs) {
        if (motion_graph == stored_motion_graph) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> MotionGraphManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Motion Graph Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
