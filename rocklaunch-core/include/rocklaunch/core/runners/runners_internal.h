#pragma once

#include "rocklaunch/core/runners/runners.h"

#include <nlohmann/json.hpp>

#include <string>

namespace rocklaunch
{
namespace Runners
{
namespace detail
{

fs::path RunnersDir();

// <runners>/<source>/<name>; both parts are validated as single path components.
fs::path RunnerDir(const std::string &source, const std::string &name);

// Steam and the system provide these; the launcher never installs or removes them.
bool IsExternal(const std::string &source);

inline std::string Qualified(const std::string &source, const std::string &name)
{
    return source + "/" + name;
}

// A string field of a cache row, "" when it is absent or of another type.
inline std::string Text(const nlohmann::json &row, const char *key)
{
    if (!row.is_object()) {
        return {};
    }
    const auto field = row.find(key);
    return field != row.end() && field->is_string() ? field->get<std::string>() : std::string();
}

} // namespace detail
} // namespace Runners
} // namespace rocklaunch
