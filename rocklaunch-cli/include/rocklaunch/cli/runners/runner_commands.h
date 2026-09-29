#pragma once

#include <string>

int RunnerList();
int RunnerSet(const std::string &profileId, const std::string &runnerToken);
int RunnerSearch(const std::string &query, bool forceRefresh);
int RunnerInstall(const std::string &runnerToken,
                  const std::string &fileName,
                  bool force);
int RunnerRemove(const std::string &runnerToken, bool force);
