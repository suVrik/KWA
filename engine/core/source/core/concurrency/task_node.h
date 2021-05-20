#pragma once

namespace kw {

class Task;

struct TaskNode {
    Task* task;
    TaskNode* next;
};

extern TaskNode INVALID_TASK_NODE;

} // namespace kw
