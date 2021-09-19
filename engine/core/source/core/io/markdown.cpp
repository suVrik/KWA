#include "core/io/markdown.h"
#include "core/error.h"

namespace kw {

template <typename T>
T& MarkdownNode::as() {
    T* result = dynamic_cast<T*>(this);

    KW_ERROR(
        result != nullptr,
        "Unexpected markdown node type."
    );

    return *result;
}

template NumberNode& MarkdownNode::as<NumberNode>();
template StringNode& MarkdownNode::as<StringNode>();
template BooleanNode& MarkdownNode::as<BooleanNode>();
template ObjectNode& MarkdownNode::as<ObjectNode>();
template ArrayNode& MarkdownNode::as<ArrayNode>();

template <typename T>
const T& MarkdownNode::as() const {
    const T* result = dynamic_cast<const T*>(this);

    KW_ERROR(
        result != nullptr,
        "Unexpected markdown node type."
    );

    return *result;
}

template const NumberNode& MarkdownNode::as<NumberNode>() const;
template const StringNode& MarkdownNode::as<StringNode>() const;
template const BooleanNode& MarkdownNode::as<BooleanNode>() const;
template const ObjectNode& MarkdownNode::as<ObjectNode>() const;
template const ArrayNode& MarkdownNode::as<ArrayNode>() const;

ObjectNode::ObjectNode(Vector<Pair<UniquePtr<MarkdownNode>, UniquePtr<MarkdownNode>>>&& elements)
    : m_elements(std::move(elements))
{
}

MarkdownNode& ObjectNode::operator[](const char* key) const {
    auto it = std::find_if(m_elements.begin(), m_elements.end(), [key](const Pair<UniquePtr<MarkdownNode>, UniquePtr<MarkdownNode>>& element) {
        StringNode* string_node = element.first->is<StringNode>();
        return string_node != nullptr && string_node->get_value() == key;
    });

    KW_ERROR(
        it != m_elements.end(),
        "Unexpected markdown object key."
    );

    return *it->second;
}

MarkdownNode* ObjectNode::find(const char* key) const {
    auto it = std::find_if(m_elements.begin(), m_elements.end(), [key](const Pair<UniquePtr<MarkdownNode>, UniquePtr<MarkdownNode>>& element) {
        StringNode* string_node = element.first->is<StringNode>();
        return string_node != nullptr && string_node->get_value() == key;
    });

    if (it != m_elements.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

size_t ObjectNode::get_size() const {
    return m_elements.size();
}

ObjectNode::iterator ObjectNode::begin() const {
    return m_elements.begin();
}

ObjectNode::iterator ObjectNode::end() const {
    return m_elements.end();
}

ArrayNode::ArrayNode(Vector<UniquePtr<MarkdownNode>>&& elements)
    : m_elements(std::move(elements))
{
}

MarkdownNode& ArrayNode::operator[](size_t index) const {
    KW_ERROR(
        index < m_elements.size(),
        "Unexpected markdown array index."
    );

    return *m_elements[index];
}

size_t ArrayNode::get_size() const {
    return m_elements.size();
}

ArrayNode::iterator ArrayNode::begin() const {
    return m_elements.begin();
}

ArrayNode::iterator ArrayNode::end() const {
    return m_elements.end();
}

} // namespace kw
