#include "rocklaunch/cli/progress_bar.h"

#include "rocklaunch/cli/cli_ui.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include <sys/ioctl.h>
#include <unistd.h>

namespace
{

constexpr const char *kStageColor = "32";
constexpr auto kRedrawInterval = std::chrono::milliseconds(100);
constexpr std::size_t kMinNameWidth = 12;
constexpr std::size_t kFallbackWidth = 80;

std::size_t TerminalWidth()
{
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
    return kFallbackWidth;
}

// Asset names come from a remote API; a control byte would corrupt the line.
std::string Printable(std::string text)
{
    for (char &c : text) {
        if (c < ' ' || c == '\x7f') {
            c = ' ';
        }
    }
    return text;
}

std::string FormatEta(double seconds)
{
    if (!(seconds >= 0.0) || seconds > 99.0 * 3600.0) {
        return "--:--";
    }
    const auto total = static_cast<long long>(seconds + 0.5);
    std::ostringstream out;
    if (total >= 3600) {
        out << total / 3600 << ':';
    }
    out << std::setfill('0') << std::setw(2) << (total % 3600) / 60 << ':'
        << std::setw(2) << total % 60;
    return out.str();
}

std::string RenderBar(double percent, std::size_t cells)
{
    const auto filled = static_cast<std::size_t>(percent / 100.0 * static_cast<double>(cells));
    return "[" + std::string(filled, '#') + std::string(cells - filled, '.') + "]";
}

// Right-hand side: "[####....]  38%  256.3 MB  643.0 KB/s  07:28". Steps that
// measure no bytes (verify, extract) have none.
std::string FormatTail(const rocklaunch::Progress &progress, std::size_t width)
{
    if (progress.totalBytes == 0 && progress.bytesTransferred == 0) {
        return {};
    }

    std::ostringstream out;
    if (progress.totalBytes > 0) {
        const std::size_t cells = width >= 90 ? 20 : (width >= 70 ? 10 : 0);
        if (cells > 0) {
            out << RenderBar(progress.percentage, cells) << ' ';
        }
        out << std::setw(3) << static_cast<int>(progress.percentage) << "%  ";
    }
    out << HumanSize(progress.bytesTransferred);
    if (progress.bytesPerSecond > 0) {
        out << "  " << HumanSize(progress.bytesPerSecond) << "/s";
        if (progress.totalBytes > progress.bytesTransferred) {
            out << "  " << FormatEta(static_cast<double>(progress.totalBytes - progress.bytesTransferred)
                                     / static_cast<double>(progress.bytesPerSecond));
        }
    }
    return out.str();
}

// Stays one column short of the terminal width so the line never wraps.
std::string FormatLine(const rocklaunch::Progress &progress, std::size_t width)
{
    const std::string stage = rocklaunch::StageName(progress.stage);
    const std::string tail = FormatTail(progress, width);
    std::string name = Printable(progress.file);

    const std::size_t usable = width - 1;
    const std::size_t taken = stage.size() + (tail.empty() ? 0 : tail.size() + 2);
    const std::size_t room = usable > taken + 1 ? usable - taken - 1 : 0;
    if (room < kMinNameWidth) {
        name.clear();
    } else if (name.size() > room) {
        name = "..." + name.substr(name.size() - (room - 3));
    }

    std::string line = Color(stage, kStageColor);
    std::size_t visible = stage.size();
    if (!name.empty()) {
        line += " " + Color(name, kRunnerColor);
        visible += 1 + name.size();
    }
    if (!tail.empty()) {
        line += std::string(usable > visible + tail.size() ? usable - visible - tail.size() : 2, ' ')
              + tail;
    }
    return line;
}

} // anonymous namespace

ConsoleProgressBar::ConsoleProgressBar()
    : m_interactive(isatty(STDOUT_FILENO) == 1)
{
}

bool ConsoleProgressBar::operator()(const rocklaunch::Progress &progress)
{
    if (!m_interactive) {
        return true;
    }

    const std::string key = std::string(rocklaunch::StageName(progress.stage)) + '\0' + progress.file;
    if (key != m_lastKey) {
        Finish();
        m_lastKey = key;
        m_done = false;
    }
    if (m_done) {
        return true;
    }

    const bool finished = progress.percentage >= 100.0;
    const auto now = std::chrono::steady_clock::now();
    if (m_lineOpen && !finished && now - m_lastDraw < kRedrawInterval) {
        return true;
    }

    m_lastDraw = now;
    m_lineOpen = true;
    std::cout << '\r' << FormatLine(progress, TerminalWidth()) << "\x1b[K" << std::flush;

    // A finished step ends its line right away: the logger writes to stderr and
    // would otherwise land on the same line as the bar.
    if (finished) {
        Finish();
        m_done = true;
    }
    return true;
}

void ConsoleProgressBar::Finish()
{
    if (m_lineOpen) {
        std::cout << '\n';
        m_lineOpen = false;
    }
}
