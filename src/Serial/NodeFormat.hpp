#pragma once

#include <codecvt>
#include <sstream>

#include "NodeView.hpp"

namespace serial {

/**
 * @brief Class that represents an abstract node format parser and writer.
 */
class NodeFormat {
public:
    /**
     * @brief Tokens used in tokenizers.
     */
    class Token {
    public:
        constexpr Token() = default;
        constexpr Token(NodeType type, std::string_view view) :
            _type(type),
            _view(view) {
        }

        /**
         * Compares if two tokens have the same type and string contents.
         * @param rhs The other token to compare.
         * @return If the tokens are equal.
         */
        constexpr bool operator==(const Token &rhs) const {
            return _type == rhs._type && _view == rhs._view.data();
        }

        constexpr bool operator!=(const Token &rhs) const {
            return !operator==(rhs);
        }

        NodeType _type = NodeType::Unknown;
        std::string_view _view;
    };

    /**
     * @brief Class that is used to print a char, and ignore null char.
     */
    class NullableChar {
    public:
        constexpr NullableChar(char val) :
            _val(val) {
        }

        constexpr operator const char &() const noexcept { return _val; }

        friend std::ostream &operator<<(std::ostream &stream, const NullableChar &c) {
            if (c._val != '\0') stream << c._val;
            return stream;
        }

        char _val;
    };

    /**
     * @brief Class that represents a configurable output format.
     */
    class Format {
    public:
        constexpr Format(int8_t spacesPerIndent, NullableChar newLine, NullableChar space, bool inlineArrays) :
            _spacesPerIndent(spacesPerIndent),
            _newLine(newLine),
            _space(space),
            _inlineArrays(inlineArrays) {
        }

        /**
         * Creates a string for the indentation level.
         * @param indent The node level to get indentation for.
         * @return The indentation string.
         */
        std::string indents(int8_t indent) const {
            return std::string(_spacesPerIndent * indent, ' ');
        }

        int8_t _spacesPerIndent;
        NullableChar _newLine, _space;
        bool _inlineArrays;
    };

    /// Writes a node with full padding.
    inline static const Format Beautified = Format(2, '\n', ' ', true);
    /// Writes a node with no padding.
    inline static const Format Minified = Format(0, '\0', '\0', false);

    virtual ~NodeFormat() = default;
    
    virtual void parseString(Node &node, std::string_view string) = 0;
    virtual void writeStream(const Node &node, std::ostream &stream, Format format = Minified) const = 0;

    // TODO: Duplicate parseStream/writeString templates from Node.
    template<typename _Elem = char
#if !defined(_MSC_VER) && !defined(__EMSCRIPTEN__) && !defined(ANDROID)
        // Cannot dynamicly parse wide streams on GCC or Clang
        , typename = std::enable_if_t<std::is_same_v<_Elem, char>>
#endif
    >
    void parseStream(Node &node, std::basic_istream<_Elem> &stream) {
#if !defined(_MSC_VER) && !defined(__EMSCRIPTEN__) && !defined(ANDROID)
        // We must read as UTF8 chars.
        if constexpr (!std::is_same_v<_Elem, char>)
            stream.imbue(std::locale(stream.getloc(), new std::codecvt_utf8<char>));
#endif

        // Reading into a string before iterating is much faster.
        std::string s(std::istreambuf_iterator<_Elem>(stream), {});
        return parseString(node, s);
    }
    
    template<typename _Elem = char>
    std::basic_string<_Elem> writeString(const Node &node, Format format = Minified) const {
        std::basic_ostringstream<_Elem> stream;
        writeStream(node, stream, format);
        return stream.str();
    }
};

template<typename T>
class NodeFormatType : public NodeFormat {
public:
    void parseString(Node &node, std::string_view string) override {
        T::Load(node, string);
    }
    
    void writeStream(const Node &node, std::ostream &stream, Format format = Minified) const override {
        T::Write(node, stream, format);
    }
};

}
