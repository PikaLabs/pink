#include <time.h>
#include <string.h>
#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>

#include "utils.h"



namespace pink {

int32_t Replace(std::string& str, const std::string &pattern, const std::string &replace) {
	int32_t count = 0;
	std::string::size_type pos = 0;
	while ((pos = str.find(pattern, pos)) != std::string::npos) {
		str.replace(pos, pattern.size(), replace);
		pos += replace.size();
		++count;
	}
	return count;
}

int32_t Pour2File(const std::string &file_name, const std::string &content) {
	std::ofstream writer(file_name.c_str());
	if (writer.bad()) {
		writer.close();
		return -1;
	}
	writer.write(content.data(), content.size());
	writer.close();
	return content.size();
}

std::string ToUpper(const std::string &origin_str) {
	std::string str = origin_str;
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}


bool EndsWith(const std::string& str, const std::vector<std::string> &suffixes) {
	std::string::size_type ori_size = str.size(), suf_size;
	for (std::vector<std::string>::const_iterator iter = suffixes.begin(); 
			iter != suffixes.end();
			++iter) {
		suf_size = iter->size();
		if (ori_size >= suf_size && str.substr(ori_size - suf_size) == *iter) {
			return true;
		}
	}
	return false;
}

bool GetDirFiles(const std::string &dir_name, std::vector<std::string> &file_fnames, std::function<bool(const std::string &)> match) {
	DIR* dir = opendir(dir_name.c_str());
	if (dir == NULL) {
		return false;
	}

	std::string con_path = dir_name + (dir_name.back() == '/' ? "": "/");
	int32_t len = offsetof(struct dirent, d_name) + pathconf(dir_name.c_str(), _PC_NAME_MAX);
	struct dirent *ent = static_cast<struct dirent *>(malloc(len)), *res = NULL;
	if (!ent) {
		closedir(dir);
		return false;
	}
	
	bool ret = true;
	while (!readdir_r(dir, ent, &res) && res) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}
		if (match(ent->d_name)) {
			file_fnames.push_back(con_path + ent->d_name);
		}
	}
	if (res) {
		ret = false;
	}
	free(ent);
	closedir(dir);
	return ret;	
}

}
