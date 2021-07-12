#include "core/io/markdown.h"
#include "core/error.h"

namespace kw {

template <>
NumberNode& MarkdownNode::as<NumberNode>() {
    KW_ERROR(
        get_type() == MarkdownType::NUMBER,
        "Unexpected markdown node type."
    );

    return *static_cast<NumberNode*>(this);
}

template <>
StringNode& MarkdownNode::as<StringNode>() {
    KW_ERROR(
        get_type() == MarkdownType::STRING,
        "Unexpected markdown node type."
    );

    return *static_cast<StringNode*>(this);
}

template <>
BooleanNode& MarkdownNode::as<BooleanNode>() {
    KW_ERROR(
        get_type() == MarkdownType::BOOLEAN,
        "Unexpected markdown node type."
    );

    return *static_cast<BooleanNode*>(this);
}

template <>
ObjectNode& MarkdownNode::as<ObjectNode>() {
    KW_ERROR(
        get_type() == MarkdownType::OBJECT,
        "Unexpected markdown node type."
    );

    return *static_cast<ObjectNode*>(this);
}

template <>
ArrayNode& MarkdownNode::as<ArrayNode>() {
    KW_ERROR(
        get_type() == MarkdownType::ARRAY,
        "Unexpected markdown node type."
    );

    return *static_cast<ArrayNode*>(this);
}

template <>
const NumberNode& MarkdownNode::as<NumberNode>() const {
    KW_ERROR(
        get_type() == MarkdownType::NUMBER,
        "Unexpected markdown node type."
    );

    return *static_cast<const NumberNode*>(this);
}

template <>
const StringNode& MarkdownNode::as<StringNode>() const {
    KW_ERROR(
        get_type() == MarkdownType::STRING,
        "Unexpected markdown node type."
    );

    return *static_cast<const StringNode*>(this);
}

template <>
const BooleanNode& MarkdownNode::as<BooleanNode>() const {
    KW_ERROR(
        get_type() == MarkdownType::BOOLEAN,
        "Unexpected markdown node type."
    );

    return *static_cast<const BooleanNode*>(this);
}

template <>
const ObjectNode& MarkdownNode::as<ObjectNode>() const {
    KW_ERROR(
        get_type() == MarkdownType::OBJECT,
        "Unexpected markdown node type."
    );

    return *static_cast<const ObjectNode*>(this);
}

template <>
const ArrayNode& MarkdownNode::as<ArrayNode>() const {
    KW_ERROR(
        get_type() == MarkdownType::ARRAY,
        "Unexpected markdown node type."
    );

    return *static_cast<const ArrayNode*>(this);
}

template <>
NumberNode* MarkdownNode::is<NumberNode>() {
    if (get_type() == MarkdownType::NUMBER) {
        return static_cast<NumberNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
StringNode* MarkdownNode::is<StringNode>() {
    if (get_type() == MarkdownType::STRING) {
        return static_cast<StringNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
BooleanNode* MarkdownNode::is<BooleanNode>() {
    if (get_type() == MarkdownType::BOOLEAN) {
        return static_cast<BooleanNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
ObjectNode* MarkdownNode::is<ObjectNode>() {
    if (get_type() == MarkdownType::OBJECT) {
        return static_cast<ObjectNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
ArrayNode* MarkdownNode::is<ArrayNode>() {
    if (get_type() == MarkdownType::ARRAY) {
        return static_cast<ArrayNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
const NumberNode* MarkdownNode::is<NumberNode>() const {
    if (get_type() == MarkdownType::NUMBER) {
        return static_cast<const NumberNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
const StringNode* MarkdownNode::is<StringNode>() const {
    if (get_type() == MarkdownType::STRING) {
        return static_cast<const StringNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
const BooleanNode* MarkdownNode::is<BooleanNode>() const {
    if (get_type() == MarkdownType::BOOLEAN) {
        return static_cast<const BooleanNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
const ObjectNode* MarkdownNode::is<ObjectNode>() const {
    if (get_type() == MarkdownType::OBJECT) {
        return static_cast<const ObjectNode*>(this);
    } else {
        return nullptr;
    }
}

template <>
const ArrayNode* MarkdownNode::is<ArrayNode>() const {
    if (get_type() == MarkdownType::ARRAY) {
        return static_cast<const ArrayNode*>(this);
    } else {
        return nullptr;
    }
}

bool ObjectNode::TransparentLess::operator()(const String& lhs, const String& rhs) const {
    return lhs < rhs;
}

bool ObjectNode::TransparentLess::operator()(const char* lhs, const String& rhs) const {
    return lhs < rhs;
}

bool ObjectNode::TransparentLess::operator()(const String& lhs, const char* rhs) const {
    return lhs < rhs;
}

ObjectNode::ObjectNode(Map<String, UniquePtr<MarkdownNode>, TransparentLess>&& elements)
    : m_elements(std::move(elements))
{
}

MarkdownNode& ObjectNode::operator[](const char* key) const {
    auto it = m_elements.find(key);

    KW_ERROR(
        it != m_elements.end(),
        "Unexpected markdown object key."
    );

    return *it->second;
}

MarkdownNode* ObjectNode::find(const char* key) {
    auto it = m_elements.find(key);
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

MarkdownType ObjectNode::get_type() const {
    return MarkdownType::OBJECT;
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

MarkdownType ArrayNode::get_type() const {
    return MarkdownType::ARRAY;
}

} // namespace kw
