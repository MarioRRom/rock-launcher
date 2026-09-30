#include "rocklaunch/cli/runners/runner_match.h"

#include "rocklaunch/core/utils/string_util.h"

#include <algorithm>
#include <sstream>

#include <nlohmann/json.hpp>

namespace runner_match
{

namespace
{

std::string LowerText(const nlohmann::json &row)
{
    static const char *kFields[] = { "source", "name", "asset" };
    std::string text;
    for (const char *field : kFields) {
        if (!text.empty()) {
            text += ' ';
        }
        text += row.value(field, "");
    }
    return rocklaunch::ToLower(text);
}

bool IsAmbiguousMatch(const nlohmann::json &row, const RunnerToken &token)
{
    return rocklaunch::ToLower(row.value("name", "")) == rocklaunch::ToLower(token.name);
}

} // anonymous namespace

RunnerToken ParseToken(std::string_view token)
{
    RunnerToken parsed;
    const std::size_t slash = token.find_last_of('/');
    if (slash == std::string_view::npos) {
        parsed.name = std::string(token);
        return parsed;
    }

    parsed.source = std::string(token.substr(0, slash));
    parsed.name = std::string(token.substr(slash + 1));
    return parsed;
}

std::vector<const nlohmann::json *> Filter(const nlohmann::json &rows, std::string_view query)
{
    const std::string text(query);
    std::vector<std::string> terms;
    std::istringstream words(text);
    std::string word;
    while (words >> word) {
        terms.push_back(rocklaunch::ToLower(word));
    }

    std::vector<const nlohmann::json *> matched;
    for (const nlohmann::json &row : rows) {
        const std::string haystack = LowerText(row);
        const bool all = std::all_of(terms.begin(), terms.end(),
                                     [&haystack](const std::string &term) {
                                         return haystack.find(term) != std::string::npos;
                                     });
        if (all) {
            matched.push_back(&row);
        }
    }
    return matched;
}

const nlohmann::json *Resolve(const nlohmann::json &rows,
                              const RunnerToken &token,
                              std::string &error)
{
    if (!token.source.empty()) {
        for (const nlohmann::json &row : rows) {
            if (row.value("source", "") == token.source
                && rocklaunch::ToLower(row.value("name", ""))
                       == rocklaunch::ToLower(token.name)) {
                return &row;
            }
        }
        return nullptr;
    }

    // Only a different source is ambiguity; one pair listed twice is not.
    const nlohmann::json *match = nullptr;
    for (const nlohmann::json &row : rows) {
        if (!IsAmbiguousMatch(row, token)) {
            continue;
        }
        if (match == nullptr) {
            match = &row;
            continue;
        }
        if (row.value("source", "") == match->value("source", "")) {
            continue;
        }
        error = "'" + token.name + "' exists in more than one source: "
            + Qualify(*match) + " and " + Qualify(row) + ". Use <source>/<name>.";
        return nullptr;
    }
    return match;
}

std::string Qualify(const nlohmann::json &row)
{
    return row.value("source", "") + "/" + row.value("name", "");
}

} // namespace runner_match
