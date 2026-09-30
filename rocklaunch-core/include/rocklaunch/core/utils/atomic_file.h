#pragma once

#include <filesystem>
#include <string_view>

namespace rocklaunch
{

namespace fs = std::filesystem;

// logPrefix names the module for the logfile; consequence leads the exception,
// which the CLI prints verbatim, so it states what the user lost.
//
// The ".tmp" name derives from the destination, so two concurrent writers of one
// path share it and interleave; nothing here serialises them.
void WriteFileAtomic(const fs::path &path, std::string_view text,
                     std::string_view logPrefix, std::string_view consequence);

} // namespace rocklaunch
