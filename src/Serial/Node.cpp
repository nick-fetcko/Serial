#include "Node.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace serial {

static const NodeProperty NullNode = NodeProperty("", Node().set(nullptr));

void Node::clear() {
    _properties.clear();
}

bool Node::valid() const {
    switch (_type) {
    case NodeType::Token:
    case NodeType::Unknown:
        return false;
    case NodeType::Object:
    case NodeType::Array:
        return !_properties.empty();
    case NodeType::Null:
        return true;
    default:
        return !_value.empty();
    }
}

bool Node::has(const std::string &name) const {
    for (const auto &[propertyName, property] : _properties) {
        if (propertyName == name)
            return true;
    }

    return false;
}

bool Node::has(uint32_t index) const {
    return index < _properties.size();
}

NodeConstView Node::property(const std::string &name) const {
    for (const auto &[propertyName, property] : _properties) {
        if (propertyName == name)
            return {this, name, &property};
    }
    return {this, name, nullptr};
}

NodeConstView Node::property(uint32_t index) const {
    if (index < _properties.size())
        return {this, index, &_properties[index].second};

    return {this, index, nullptr};
}

// TODO: Duplicate
NodeView Node::property(const std::string &name) {
    for (auto &[propertyName, property] : _properties) {
        if (propertyName == name)
            return {this, name, &property};
    }
    return {this, name, nullptr};
}

// TODO: Duplicate
NodeView Node::property(uint32_t index) {
    if (index < _properties.size())
        return {this, index, &_properties[index].second};

    return {this, index, nullptr};
}

Node &Node::add(const Node &node) {
    _type = NodeType::Array;
    return _properties.emplace_back(NodeProperty("", node)).second;
}

Node &Node::add(Node &&node) {
    _type = NodeType::Array;
    return _properties.emplace_back(NodeProperty("", std::move(node))).second;
}

Node &Node::add(const std::string &name, const Node &node) {
    return _properties.emplace_back(NodeProperty(name, node)).second;
}

Node &Node::add(const std::string &name, Node &&node) {
    return _properties.emplace_back(NodeProperty(name, std::move(node))).second;
}

Node &Node::add(uint32_t index, const Node &node) {
    _type = NodeType::Array;
    _properties.resize(std::max(_properties.size(), static_cast<std::size_t>(index + 1)), NullNode);
    return _properties[index].second = node;
}

Node &Node::add(uint32_t index, Node &&node) {
    _type = NodeType::Array;
    _properties.resize(std::max(_properties.size(), static_cast<std::size_t>(index + 1)), NullNode);
    return _properties[index].second = std::move(node);
}

Node Node::remove(const std::string &name) {
    for (auto it = _properties.begin(); it != _properties.end(); ) {
        if (it->first == name) {
            auto result = std::move(it->second);
            _properties.erase(it);
            return result;
        }
        ++it;
    }
    return {};
}

Node Node::remove(const Node &node) {
    for (auto it = _properties.begin(); it != _properties.end(); ) {
        if (it->second == node) {
            auto result = std::move(it->second);
            it = _properties.erase(it);
            return result;
        }
        ++it;
    }
    return {};
}

NodeConstView Node::propertyWithBackup(const std::string &name, const std::string &backupName) const {
    if (auto p1 = property(name))
        return p1;
    if (auto p2 = property(backupName))
        return p2;
    return {this, name, nullptr};
}

// TODO: Duplicate
NodeView Node::propertyWithBackup(const std::string &name, const std::string &backupName) {
    if (auto p1 = property(name))
        return p1;
    if (auto p2 = property(backupName))
        return p2;
    return {this, name, nullptr};
}

NodeConstView Node::propertyWithValue(const std::string &name, const NodeValue &propertyValue) const {
    for (const auto &[propertyName, property] : _properties) {
        if (auto property1 = property.property(name); property1->value() == propertyValue)
            return {this, name, &property};
    }

    return {this, name, nullptr};
}

// TODO: Duplicate
NodeView Node::propertyWithValue(const std::string &name, const NodeValue &propertyValue) {
    for (auto &[propertyName, property] : _properties) {
        if (auto property1 = property.property(name); property1->value() == propertyValue)
            return {this, name, &property};
    }

    return {this, name, nullptr};
}

NodeConstView Node::operator[](const std::string &name) const {
    return property(name);
}

NodeConstView Node::operator[](uint32_t index) const {
    return property(index);
}

// TODO: Duplicate
NodeView Node::operator[](const std::string &name) {
    return property(name);
}

// TODO: Duplicate
NodeView Node::operator[](uint32_t index) {
    return property(index);
}

bool Node::operator==(const Node &rhs) const {
    return _value == rhs._value && _properties.size() == rhs._properties.size() &&
        std::equal(_properties.begin(), _properties.end(), rhs._properties.begin(), [](const auto &left, const auto &right) {
        return left == right;
    });
}

bool Node::operator!=(const Node &rhs) const {
    return !operator==(rhs);
}

bool Node::operator<(const Node &rhs) const {
    if (_value < rhs._value) return true;
    if (rhs._value < _value) return false;

    if (_properties < rhs._properties) return true;
    if (rhs._properties < _properties) return false;

    return false;
}

}
