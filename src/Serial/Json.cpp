#include "Json.hpp"

#define ATTRIBUTE_TEXT_SUPPORT 1

namespace serial {

void Json::Load(Node &node, std::string_view string) {
    // Tokenizes the string view into small views that are used to build a Node tree.
    std::vector<Token> tokens;

    std::size_t tokenStart = 0;
    enum class QuoteState : char {
        None, Single, Double
    } quoteState = QuoteState::None;

    // Iterates over all the characters in the string view.
    for (const auto [index, c] : utils::Enumerate(string)) {
        // If the previous character was a backslash the quote will not break the string.
        if (c == '\'' && quoteState != QuoteState::Double && string[index - 1] != '\\')
            quoteState = quoteState == QuoteState::None ? QuoteState::Single : QuoteState::None;
        else if (c == '"' && quoteState != QuoteState::Single && string[index - 1] != '\\')
            quoteState = quoteState == QuoteState::None ? QuoteState::Double : QuoteState::None;

        // When not reading a string tokens can be found.
        // While in a string whitespace and tokens are added to the strings view.
        if (quoteState == QuoteState::None) {
            if (utils::IsWhitespace(c)) {
                // On whitespace start save current token.
                AddToken(std::string_view(string.data() + tokenStart, index - tokenStart), tokens);
                tokenStart = index + 1;
            } else if (c == ':' || c == '{' || c == '}' || c == ',' || c == '[' || c == ']') {
                // Tokens used to read json nodes.
                AddToken(std::string_view(string.data() + tokenStart, index - tokenStart), tokens);
                tokens.emplace_back(NodeType::Token, std::string_view(string.data() + index, 1));
                tokenStart = index + 1;
            }
        }
    }

    if (tokens.empty())
        throw std::runtime_error("No tokens found in document");

    // Converts the tokens into nodes.
    int32_t k = 0;
    Convert(node, tokens, k);
}

void Json::Write(const Node &node, std::ostream &stream, Format format) {
    stream << (node.type() == NodeType::Array ? '[' : '{') << format._newLine;
    AppendData(node, stream, format, 1);
    stream << (node.type() == NodeType::Array ? ']' : '}');
}

void Json::AddToken(std::string_view view, std::vector<Token> &tokens) {
    if (view.length() != 0) {
        // Finds the node value type of the string and adds it to the tokens vector.
        if (view == "null") {
            tokens.emplace_back(NodeType::Null, std::string_view());
        } else if (view == "true" || view == "false") {
            tokens.emplace_back(NodeType::Boolean, view);
        } else if (utils::IsNumber(view)) {
            // This is a quick hack to get if the number is a decimal.
            if (view.find('.') != std::string::npos) {
                if (view.size() >= std::numeric_limits<long double>::digits)
                    throw std::runtime_error("Decimal number is too long");
                tokens.emplace_back(NodeType::Decimal, view);
            } else {
                if (view.size() >= std::numeric_limits<uint64_t>::digits)
                    throw std::runtime_error("Integer number is too long");
                tokens.emplace_back(NodeType::Integer, view);
            }
        } else { // if (view.front() == view.back() == '\"')
            tokens.emplace_back(NodeType::String, view.substr(1, view.length() - 2));
        }
    }
}

void Json::Convert(Node &current, const std::vector<Token> &tokens, int32_t &k) {
    if (tokens[k] == Token(NodeType::Token, "{")) {
        k++;

        while (tokens[k] != Token(NodeType::Token, "}")) {
            auto key = tokens[k]._view;
            if (k + 2 >= tokens.size())
                throw std::runtime_error("Missing end of {} array");
            if (tokens[k + 1]._view != ":")
                throw std::runtime_error("Missing object colon");
            k += 2;
#if ATTRIBUTE_TEXT_SUPPORT
            // Write value string into current value, then continue parsing properties into current.
            if (key == "#text")
                Convert(current, tokens, k);
            else
#endif
                Convert(current.add(std::string(key)), tokens, k);
            if (tokens[k]._view == ",")
                k++;
        }
        k++;

        current.type(NodeType::Object);
    } else if (tokens[k] == Token(NodeType::Token, "[")) {
        k++;

        while (tokens[k] != Token(NodeType::Token, "]")) {
            if (k >= tokens.size())
                throw std::runtime_error("Missing end of [] object");
            Convert(current.add(), tokens, k);
            if (tokens[k]._view == ",")
                k++;
        }
        k++;

        current.type(NodeType::Array);
    } else {
        std::string str(tokens[k]._view);
        if (tokens[k]._type == NodeType::String)
            str = utils::UnfixEscapedChars(str);
        current.value(str);
        current.type(tokens[k]._type);
        k++;
    }
}

void Json::AppendData(const Node &node, std::ostream &stream, Format format, int32_t indent) {
    auto indents = format.indents(indent);

    // Only output the value if no properties exist.
    if (node.properties().empty()) {
        if (node.type() == NodeType::String)
            stream << '\"' << utils::FixEscapedChars(node.value()) << '\"';
        else if (node.type() == NodeType::Null)
            stream << "null";
        else
            stream << node.value();
    }

#if ATTRIBUTE_TEXT_SUPPORT
    // If the Json Node has both properties and a value, value will be written as a "#text" property.
    // XML is the only format that allows a Node to have both a value and properties.
    if (!node.properties().empty() && !node.value().empty()) {
        stream << indents;
        stream << "\"#text\":" << format._space << "\"" << node.value() << "\",";
        // No new line if the indent level is zero (if primitive array type).
        stream << (indent != 0 ? format._newLine : format._space);
    }
#endif

    // Output each property.
    for (auto it = node.properties().begin(); it != node.properties().end(); ++it) {
        const auto &[propertyName, property] = *it;
        // TODO: if this *it is in an array and there are elements missing between *(it-1) and *it fill with null.

        stream << indents;
        // Output name for property if it exists.
        if (!propertyName.empty()) {
            stream << '\"' << propertyName << "\":" << format._space;
        }

        bool isArray = false;
        if (!property.properties().empty()) {
            // If all properties have no names, then this must be an array.
            // TODO: this does not look over all properties, handle where we have mixed mapped names and array elements.
            for (const auto &[name2, property2] : property.properties()) {
                if (name2.empty()) {
                    isArray = true;
                    break;
                }
            }

            stream << (isArray ? '[' : '{') << format._newLine;
        } else if (property.type() == NodeType::Object) {
            stream << '{';
        } else if (property.type() == NodeType::Array) {
            stream << '[';
        }

        // If a node type is a primitive type.
        constexpr static auto IsPrimitive = [](const Node &type) {
            return type.properties().empty() && type.type() != NodeType::Object && type.type() != NodeType::Array && type.type() != NodeType::Unknown;
        };

        // Shorten primitive array output length.
        if (isArray && format._inlineArrays && !property.properties().empty() && IsPrimitive(*property.property(0))) {
            stream << format.indents(indent + 1);
            // New lines are printed a a space, no spaces are ever emitted by primitives.
            AppendData(property, stream, Format(0, '\0', '\0', false), indent);
            stream << '\n';
        } else {
            AppendData(property, stream, format, indent + 1);
        }

        if (!property.properties().empty()) {
            stream << indents << (isArray ? ']' : '}');
        } else if (property.type() == NodeType::Object) {
            stream << '}';
        } else if (property.type() == NodeType::Array) {
            stream << ']';
        }

        // Separate properties by comma.
        if (it != std::prev(node.properties().end()))
            stream << ',';
        // No new line if the indent level is zero (if primitive array type).
        stream << (indent != 0 ? format._newLine : format._space);
    }
}

}
