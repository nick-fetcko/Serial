#include "Utils.hpp"

#include <codecvt>
#include <locale>

namespace serial::utils {


std::string ReplaceAll(std::string str, std::string_view token, std::string_view to) {
    auto pos = str.find(token);
    while (pos != std::string::npos) {
        str.replace(pos, token.size(), to);
        pos = str.find(token, pos + token.size());
    }

    return str;
}

std::string_view Trim(std::string_view str, std::string_view whitespace) {
    auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return "";

    auto strEnd = str.find_last_not_of(whitespace);
    auto strRange = strEnd - strBegin + 1;
    return str.substr(strBegin, strRange);
}

bool IsWhitespace(char c) noexcept {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

bool IsNumber(std::string_view str) noexcept {
    return std::all_of(str.cbegin(), str.cend(), [](auto c) {
        return (c >= '0' && c <= '9') || c == '.' || c == '-';
    });
}

std::string FixEscapedChars(std::string str) {
    static const std::vector<std::pair<char, std::string_view>> replaces = {{'\\', "\\\\"}, {'\n', "\\n"}, {'\r', "\\r"}, {'\t', "\\t"}, {'\"', "\\\""}};

    for (const auto &[from, to] : replaces) {
        auto pos = str.find(from);
        while (pos != std::string::npos) {
            str.replace(pos, 1, to);
            pos = str.find(from, pos + 2);
        }
    }

    return str;
}

std::string UnfixEscapedChars(std::string str) {
    static const std::vector<std::pair<std::string_view, char>> replaces = {{"\\n", '\n'}, {"\\r", '\r'}, {"\\t", '\t'}, {"\\\"", '\"'}, {"\\\\", '\\'}};

    for (const auto &[from, to] : replaces) {
        auto pos = str.find(from);
        while (pos != std::string::npos) {
            if (pos != 0 && str[pos - 1] == '\\')
                str.erase(str.begin() + --pos);
            else
                str.replace(pos, from.size(), 1, to);
            pos = str.find(from, pos + 1);
        }
    }

    return str;
}

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> Utf8ToUtf16Converter;

std::string ConvertUtf8(const std::wstring_view &string) {
    return Utf8ToUtf16Converter.to_bytes(string.data(), string.data() + string.length());
}

char ConvertUtf8(wchar_t c) {
    return Utf8ToUtf16Converter.to_bytes(c)[0];
}

std::wstring ConvertUtf16(const std::string_view &string) {
    return Utf8ToUtf16Converter.from_bytes(string.data(), string.data() + string.length());
}

wchar_t ConvertUtf16(char c) {
    return Utf8ToUtf16Converter.from_bytes(c)[0];
}

}
