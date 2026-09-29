#include "rocklaunch/core/utils/file_lock.h"

#include "rocklaunch/core/logger.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/file.h>
#include <unistd.h>

namespace rocklaunch
{

namespace
{

constexpr const char *kLockDirName = ".locks";
constexpr mode_t kLockFileMode = 0600;

} // anonymous namespace

fs::path PathLock::LockPath(const fs::path &resource)
{
    return resource.parent_path() / kLockDirName / (resource.filename().string() + ".lock");
}

PathLock::PathLock(const fs::path &resource)
{
    const fs::path lockPath = LockPath(resource);

    std::error_code error;
    fs::create_directories(lockPath.parent_path(), error);
    if (error) {
        Logger().Error("PathLock: cannot create lock directory "
                       + lockPath.parent_path().string() + ": " + error.message());
        throw std::runtime_error("locking needs a writable directory beside the resource "
                                 "it guards");
    }

    // O_CLOEXEC: the tar child must not inherit and outlive the lock.
    // flock rather than fcntl: fcntl locks are per process, so they would not
    // stop a second worker inside the same GUI process.
    m_fd = ::open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, kLockFileMode);
    if (m_fd < 0) {
        Logger().Error("PathLock: cannot open lock file " + lockPath.string() + ": "
                       + std::strerror(errno));
        throw std::runtime_error("an already-held lock is reported as 'not acquired', "
                                 "not as this failure");
    }

    if (::flock(m_fd, LOCK_EX | LOCK_NB) != 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

PathLock::~PathLock()
{
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

} // namespace rocklaunch
