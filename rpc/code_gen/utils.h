#ifndef _RPC_COMMON_H
#define _RPC_COMMON_H

#include <signal.h>
#include <string>
#include <stdint.h>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace pink {

int32_t Replace(std::string& str, const std::string &pattern, const std::string &replcace);
int32_t Pour2File(const std::string &file_name, const std::string &content);
std::string ToUpper(const std::string &origin_str);
bool EndsWith(const std::string& str, const std::vector<std::string> &suffixes);
bool GetDirFiles(const std::string &dir, std::vector<std::string> &file_fnames, std::function<bool(const std::string &)> match = [](const std::string &str) {return true;});

}
#endif
