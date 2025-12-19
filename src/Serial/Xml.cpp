#include "Xml.hpp"

namespace serial {

void Xml::Load(Node &node, std::string_view string) {
    // Tokenizes the string view into small views that are used to build a Node tree.
    std::vector<Token> tokens;

    std::size_t tokenStart = 0;
    enum class QuoteState : char {
        None, Single, Double
    } quoteState = QuoteState::None;
    enum class TagState : char {
        None, Open, Close
    } tagState = TagState::None;

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
            if (tagState == TagState::Open && utils::IsWhitespace(c)) {
                // On whitespace start save current token.
                AddToken(std::string_view(string.data() + tokenStart, index - tokenStart), tokens);
                tokenStart = index + 1;
            } else if (c == '<' || c == '>' || (tagState == TagState::Open && (c == '?' || c == '!' || c == '=' || c == '/'))) {
                // Tokens used to read XML nodes.
                AddToken(std::string_view(string.data() + tokenStart, index - tokenStart), tokens);
                tokens.emplace_back(NodeType::Token, std::string_view(string.data() + index, 1));
                if (c == '<')
                    tagState = TagState::Open;
                else if (c == '>')
                    tagState = TagState::Close;
                tokenStart = index + 1;
            }
        }
    }

    //if (tokens.empty())
    //    throw std::runtime_error("No tokens found in document");

    // Converts the tokens into nodes.
    int32_t k = 0;
    Convert(node, tokens, k);
}

void Xml::Write(const Node &node, std::ostream &stream, Format format) {
    if (!node.has("?xml")) {
        Node xmldecl;
        xmldecl.add("@version").set("1.0");
        xmldecl.add("@encoding").set("utf-8");
        AppendData("?xml", xmldecl, stream, format, 0);
    }

    for (const auto &[propertyName, property] : node.properties()) {
        AppendData(propertyName, property, stream, format, 0);
    }
}

void Xml::AddToken(std::string_view view, std::vector<Token> &tokens) {
    if (view.length() != 0 && !std::all_of(view.cbegin(), view.cend(), utils::IsWhitespace))
        tokens.emplace_back(NodeType::String, view);
}

void Xml::Convert(Node &current, const std::vector<Token> &tokens, int32_t &k) {
	if (k >= tokens.size()) throw std::runtime_error("Likely missing closing token(s)! (\"/>\")");

    // Only start to parse if we are at the start of a tag.
    if (tokens[k] != Token(NodeType::Token, "<"))
        return;
    k++;

    // Ignore comments.
    if (tokens[k] == Token(NodeType::Token, "!") && (tokens[k + 1]._view.find("--") == 0)) {
        k += 2;
        while (tokens[k + 1] != Token(NodeType::Token, ">") && (tokens[k]._view.find("--") == std::string::npos))
            k++;
        k += 2;
        Convert(current, tokens, k);
        return;
    }

    // The next token after the open tag is the name.
    std::string name(tokens[k]._view);
    // First token in tag might have been prolog or XMLDecl char, name will be in the following token.
    if (tokens[k] == Token(NodeType::Token, "?") || tokens[k] == Token(NodeType::Token, "!")) {
        name += tokens[k + 1]._view;
        k++;
    }
    k++;

    // Create the property that will contain the attributes and children found in the tag.
    auto &property = CreateProperty(current, name);

    while (tokens[k] != Token(NodeType::Token, ">")) {
        // Attributes are added as properties.
        if (tokens[k] == Token(NodeType::Token, "=")) {
            property.add(AttributePrefix + std::string(tokens[k - 1]._view)).set(tokens[k + 1]._view.substr(1, tokens[k + 1]._view.size() - 2));
            k++;
        }
        k++;
    }
    k++;

    // More tags will follow after prolog and XMLDecl.
    if (tokens[k - 2] == Token(NodeType::Token, "?") || name[0] == '!') {
        Convert(current, tokens, k);
        return;
    }
    // Inline tag has no children.
    if (tokens[k - 2] == Token(NodeType::Token, "/"))
        return;
    // Continue through all children until the end tag is found.
    while (!(tokens[k] == Token(NodeType::Token, "<") && tokens[k + 1] == Token(NodeType::Token, "/") && tokens[k + 2]._view == name)) {
        if (tokens[k]._type == NodeType::String) {
            property.set(tokens[k]._view);
            k++;
        } else {
            // TODO: If the token at k is not a '<' this will cause a infinite loop, or if k + 2 > tokens.size() vector access will be violated.
            Convert(property, tokens, k);
        }
    }
    k += 4;
}

Node &Xml::CreateProperty(Node &current, const std::string &name) {
    // Combine duplicate tags.
    if (auto duplicate = current[name]) {
        // If the node is already an array add the new property to it.
        if (duplicate->type() == NodeType::Array)
            return duplicate->add();

        // Copy the duplicate node so we can add it to the new array.
        auto original = current.remove(*duplicate);
        auto &array = current.add(name);
        array.type(NodeType::Array);
        array.add(std::move(original));
        return array.add();
    }

    return current.add(name);
}

void Xml::AppendData(const std::string &nodeName, const Node &node, std::ostream &stream, Format format, int32_t indent) {
    if (nodeName.rfind(AttributePrefix, 0) == 0) return;

    if (node.type() == NodeType::Array) {
        // If the node is an array, then all properties will inherit the array name.
        for (const auto &[propertyName, property] : node.properties())
            AppendData(nodeName, property, stream, format, indent);
        return;
    }

    auto indents = format.indents(indent);
    stream << indents << '<' << nodeName;

    // Add attributes to opening tag.
    int attributeCount = 0;
    for (const auto &[propertyName, property] : node.properties()) {
        if (propertyName.rfind(AttributePrefix, 0) != 0) continue;
        stream << " " << propertyName.substr(1) << "=\"" << property.value() << "\"";
        attributeCount++;
    }

    // When the property has a value or children recursively append them, otherwise shorten tag ending.
    if (node.properties().size() - attributeCount != 0 || !node.value().empty()) {
        stream << '>';
        stream << node.value();

        if (node.properties().size() - attributeCount != 0) {
            stream << format._newLine;
            // Output each property.
            for (const auto &[propertyName, property] : node.properties())
                AppendData(propertyName, property, stream, format, indent + 1);
            stream << indents;
        } else {
            // Output each property.
            for (const auto &[propertyName, property] : node.properties())
                AppendData(propertyName, property, stream, format, indent + 1);
        }
        stream << "</" << nodeName << '>' << format._newLine;
    } else if (nodeName[0] == '?') {
        stream << "?>" << format._newLine;
    } else if (nodeName[0] == '!') {
        stream << '>' << format._newLine;
    } else {
        stream << "/>" << format._newLine;
    }
}

}
