#pragma once

#include <ostream>

#include "NodeFormat.hpp"
#include "NodeView.hpp"

namespace serial {

/**
 * @brief Class that is used to represent a tree of UFT-8 values, used in serialization.
 */
class Node final {
public:
    Node() noexcept {} // = default;
    Node(const Node &node) = default;
    Node(Node &&node) noexcept = default;

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T *, NodeFormat *>>>
    void parseString(std::string_view string);
    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T *, NodeFormat *>>>
    void writeStream(std::ostream &stream, NodeFormat::Format format = NodeFormat::Minified) const;

    template<typename T, typename _Elem = char, typename = std::enable_if_t<
        std::is_convertible_v<T *, NodeFormat *>
#if !defined(_MSC_VER) && !defined(__EMSCRIPTEN__) && !defined(ANDROID)
        // Cannot dynamicly parse wide streams on GCC or Clang
        && std::is_same_v<_Elem, char>
#endif
    >>
    void parseStream(std::basic_istream<_Elem> &stream);
    template<typename T, typename _Elem = char, typename = std::enable_if_t<std::is_convertible_v<T *, NodeFormat *>>>
    std::basic_string<_Elem> writeString(NodeFormat::Format format = NodeFormat::Minified) const;

    template<typename T>
    T get() const;
    template<typename T>
    T getWithFallback(const T &fallback) const;
    template<typename T>
    bool get(T &dest) const;
    template<typename T, typename K>
    bool getWithFallback(T &dest, const K &fallback) const;
    template<typename T>
    bool get(T &&dest) const;
    template<typename T, typename K>
    bool getWithFallback(T &&dest, const K &fallback) const;
    template<typename T>
    Node &set(const T &value);
    template<typename T>
    Node &set(T &&value);
    
    /**
     * Clears all properties from this node.
     */
    void clear();

    /**
     * Gets if the node has a value, or has properties that have values.
     * @return If the node is internally valid.
     */
    bool valid() const;

    template<typename T>
    Node &append(const T &value);
    template<typename ...Args>
    Node &append(const Args &...args);
    
    //Node &merge(Node &&node);

    bool has(const std::string &name) const;
    bool has(uint32_t index) const;

    NodeConstView property(const std::string &name) const;
    Node *mutableProperty(const std::string &name);
    NodeConstView property(uint32_t index) const;
    NodeView property(const std::string &name);
    NodeView property(uint32_t index);

    Node &add(const Node &node);
    Node &add(Node &&node = {});
    Node &add(const std::string &name, const Node &node);
    Node &add(const std::string &name, Node &&node = {});
    Node &add(uint32_t index, const Node &node);
    Node &add(uint32_t index, Node &&node = {});

    Node remove(const std::string &name);
    Node remove(const Node &node);

    NodeConstView propertyWithBackup(const std::string &name, const std::string &backupName) const;
    NodeView propertyWithBackup(const std::string &name, const std::string &backupName);

    NodeConstView propertyWithValue(const std::string &name, const NodeValue &propertyValue) const;
    NodeView propertyWithValue(const std::string &name, const NodeValue &propertyValue);

    NodeConstView operator[](const std::string &name) const;
    NodeConstView operator[](uint32_t index) const;
    NodeView operator[](const std::string &name);
    NodeView operator[](uint32_t index);

    Node &operator=(const Node &rhs) = default;
    Node &operator=(Node &&rhs) noexcept = default;

    bool operator==(const Node &rhs) const;
    bool operator!=(const Node &rhs) const;
    bool operator<(const Node &rhs) const;

    const NodeProperties &properties() const { return _properties; }
    NodeProperties &properties() { return _properties; }

    const NodeValue &value() const { return _value; }
    void value(NodeValue value) { _value = std::move(value); }

    const NodeType &type() const { return _type; }
    void type(NodeType type) { _type = type; }

protected:
    NodeProperties _properties;
    NodeValue _value;
    NodeType _type = NodeType::Object;
};

}

#include "Node.inl"
