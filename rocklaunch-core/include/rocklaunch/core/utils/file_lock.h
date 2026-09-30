#pragma once

#include <filesystem>

namespace rocklaunch
{

namespace fs = std::filesystem;

// Exclusive advisory lock for a path, held in a sibling file so the kernel
// releases it when the holder dies, SIGKILL included.
class PathLock
{
public:
    // The lock file is <dir>/.locks/<name>.lock. A resource held elsewhere leaves
    // the lock unheld rather than throwing; only an unusable lock file throws.
    explicit PathLock(const fs::path &resource);
    ~PathLock();

    PathLock(const PathLock &) = delete;
    PathLock &operator=(const PathLock &) = delete;

    // False when someone else already holds the lock.
    bool Acquired() const { return m_fd >= 0; }

    static fs::path LockPath(const fs::path &resource);

private:
    int m_fd = -1;
};

} // namespace rocklaunch
