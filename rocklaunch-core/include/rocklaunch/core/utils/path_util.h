#pragma once

#include <stdexcept>
#include <string>

namespace rocklaunch
{

// Names from a remote API end up as directories that are later removed, so
// anything able to leave the parent is refused.
inline bool IsPathComponent(const std::string &name)
{
    return !name.empty() && name[0] != '.'
        && name.find_first_of(std::string("/\0", 2)) == std::string::npos;
}

inline const std::string &RequirePathComponent(const std::string &name)
{
    if (!IsPathComponent(name)) {
        throw std::runtime_error("Invalid name: '" + name + "'");
    }
    return name;
}

} // namespace rocklaunch
