#include "rocklaunch/core/utils/atomic_file.h"

#include "rocklaunch/core/logger.h"

#include <fstream>
#include <stdexcept>

namespace rocklaunch
{

void WriteFileAtomic(const fs::path &path, std::string_view text,
                     std::string_view logPrefix, std::string_view consequence)
{
    const std::string tag(logPrefix);
    const std::string lead(consequence);
    const fs::path tempPath = path.string() + ".tmp";
    std::error_code error;
    {
        std::ofstream output(tempPath);
        if (!output.is_open()) {
            Logger().Error(tag + ": cannot write " + tempPath.string());
            throw std::runtime_error(lead + ": cannot write " + path.string()
                                     + "\n  check free space and permissions on "
                                     + path.parent_path().string());
        }
        output << text;
        output.flush();
        if (!output) {
            output.close();
            fs::remove(tempPath, error);
            Logger().Error(tag + ": write to " + tempPath.string() + " failed");
            throw std::runtime_error(lead + ": the write to " + path.string()
                                     + " ran out of space or was interrupted");
        }
    }

    fs::rename(tempPath, path, error);
    if (error) {
        const std::string reason = error.message();
        fs::remove(tempPath, error);
        Logger().Error(tag + ": cannot replace " + path.string() + ": " + reason);
        throw std::runtime_error(lead + ": cannot replace " + path.string() + "\n  " + reason);
    }
}

} // namespace rocklaunch
