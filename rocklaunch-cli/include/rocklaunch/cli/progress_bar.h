#pragma once

#include "rocklaunch/core/progress.h"

#include <chrono>
#include <string>

// One-line console bar fed from a core ProgressCallback. Draws nothing when
// stdout is not a terminal. Never cancels: operator() always returns true.
class ConsoleProgressBar
{
public:
    ConsoleProgressBar();

    ConsoleProgressBar(const ConsoleProgressBar &) = delete;
    ConsoleProgressBar &operator=(const ConsoleProgressBar &) = delete;

    bool operator()(const rocklaunch::Progress &progress);

    // Ends the current line so the next output starts clean.
    void Finish();

private:
    bool m_interactive = false;
    bool m_lineOpen = false;
    bool m_done = false;
    std::string m_lastKey;
    std::string m_lastLine;
    std::chrono::steady_clock::time_point m_lastDraw{};
};
