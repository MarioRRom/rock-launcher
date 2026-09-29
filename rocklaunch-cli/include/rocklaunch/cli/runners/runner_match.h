#pragma once

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <string_view>
#include <vector>

// A runner as typed on the command line: "source/name", or a bare name that is
// refused when several sources carry it.
struct RunnerToken
{
    std::string name;
    std::string source;
};

namespace runner_match
{

// "source/name" or a bare "name". A token with no slash has an empty source.
RunnerToken ParseToken(std::string_view token);

// The rows matching every whitespace-separated term, case insensitively, over source,
// name and asset. An empty query keeps them all. Pointers into rows.
std::vector<const nlohmann::json *> Filter(const nlohmann::json &rows, std::string_view query);

// The unique row matching token, pointing into rows. Null when nothing matches, or
// when a bare name exists in several sources, in which case error names them.
const nlohmann::json *Resolve(const nlohmann::json &rows,
                              const RunnerToken &token,
                              std::string &error);

// "source/name" as the listings print it.
std::string Qualify(const nlohmann::json &row);

} // namespace runner_match
