#ifndef _RPC_LOG_H
#define _RPC_LOG_H

#include "rpc_common.h"
#include "rpc_conf.h"

#include <string>
#include <unistd.h>
#include <stdio.h>

namespace pink {

enum RpcLogLevel {
	kDebug,
	kTrace,
	kInfo,
	kWarn,
	kError,
	kFatal,
	kMaxLevel
};

#define        LOG_DEBUG(M, ...)          g_log->Log(kDebug, M, ##__VA_ARGS__)
#define        LOG_TRACE(M, ...)          g_log->Log(kTrace, M, ##__VA_ARGS__)
#define        LOG_INFO(M, ...)           g_log->Log(kInfo, M, ##__VA_ARGS__)
#define        LOG_WARN(M, ...)           g_log->Log(kWarn, M, ##__VA_ARGS__)
#define        LOG_ERROR(M, ...)          g_log->Log(kError, M, ##__VA_ARGS__)
#define        LOG_FATAL(M, ...)          g_log->Log(kFatal, M, ##__VA_ARGS__)

class RpcLog {
public:

	
	static std::string rpc_log_level_str[kMaxLevel];
public:
	RpcLog(RpcLogLevel log_level = RpcLogLevel::kError, const std::string &log_file_name = "") :
		log_level_(log_level),
		log_file_name_(log_file_name),
		log_file_(NULL),
		log_daemon_(false) {
		if (log_file_name_.empty()) {
			log_file_name_ = "./rpc_log_" + GetDate();
		}
	}

	~RpcLog() {
		if (log_file_) {
			fclose(log_file_);
			log_file_ = NULL;
		}
	}
	
	void Init(const RpcConf *conf);
	void Log(RpcLogLevel level, char *format, ...);

private:
	std::string log_file_name_;
	FILE *log_file_;
	RpcLogLevel log_level_;
	bool log_daemon_;
};

}

#endif
