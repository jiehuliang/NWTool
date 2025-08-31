#ifndef LOGGER_H
#define LOGGER_H

#include "LogLevel.h"
#include "LogStream.h"
#include "OsTid.h"

#include <chrono>
#include <ctime>
#include <functional>



namespace HooLog {

using loglevel = LogLevel::value;

using OutputFunc = std::function<void(const char* msg, int length)>;
using FlushFunc = std::function<void()>;

//缓存当前线程存日期时间字符串、上一次日志记录的秒数,提升日期格式化效率
thread_local static char t_time[64];	//当前线程存日期的时间字符串 “年:月:日 时:分:秒”
thread_local static int64_t t_lastseconds;	//当前线程上一次记录日志的秒数

inline uint64_t getCacheTid() {
	static thread_local uint64_t t_cacheTid = 0;
	return t_cacheTid ? t_cacheTid : (t_cacheTid = static_cast<uint64_t>(os_tid()));
}

inline loglevel& GetLogLevel() {
	static loglevel g_loglevel = loglevel::INFO;
	return g_loglevel;
}
inline void defaultOutput(const char* msg, int len) {
	fwrite(msg, 1, len, stdout);	// 默认写出到stdout
}

inline void defaultFlush() {
	fflush(stdout);		// 默认flush到stdout
}

inline OutputFunc& GetOutputFunc() {
	static OutputFunc g_output = defaultOutput;
	return g_output;
}

inline FlushFunc& GetFlushFunc() {
	static FlushFunc g_flush = defaultFlush;
	return g_flush;
}

inline void setLogLevel(loglevel level) {
	GetLogLevel() = level;
}

inline void setOutputFunc(OutputFunc output) {
	GetOutputFunc() = output;
}

inline void setFlushFunc(FlushFunc flush) {
	GetFlushFunc() = flush;
}

class Logger {
public:
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	Logger(Logger&&) = delete;
	Logger& operator=(Logger&&) = delete;

	Logger(loglevel level,const char* filename,int line) :level_(level),filename_(filename),line_(line){
		formatDate();
		formatTid();
		formatLevel();
		
	}

	~Logger() {
		formatSourceFile();
		GetOutputFunc()(stream_.data(), stream_.length());

		//当日志级别为FATAL时，flush设备缓冲区并终止程序
		if (level_ == loglevel::FATAL) {
			GetFlushFunc()();
			abort();
		}
	}

	LogStream& stream() {
		return stream_;
	}

private:
	loglevel level_;
	const char* filename_;
	int line_;
	LogStream stream_;

	void formatDate() {
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		if (t_lastseconds != seconds) {
			std::time_t now_c = std::chrono::system_clock::to_time_t(now);
			std::tm* local_time = std::localtime(&now_c);
			snprintf(t_time, sizeof(t_time), "%4d-%02d-%02d %02d:%02d:%02d", local_time->tm_year + 1900, local_time->tm_mon + 1,
				local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
			t_lastseconds = seconds;
		}
		int32_t milli_part = ms % 1000;
		stream_.fmt("[ %s.%03d ]",  t_time,milli_part);
	}

	void formatTid() {
		stream_.fmt("[ %llu ]", getCacheTid());
	}

	void formatLevel() {
		stream_.fmt("[ %s ]", LogLevel::toString(level_));
	}

	void formatSourceFile() {
		const char* forward_slash = strrchr(filename_, '/');
#if defined(_WIN32) || defined(_WIN64)
		const char* win_forward_slash = strrchr(filename_, '\\');
		if (win_forward_slash && (!win_forward_slash || win_forward_slash > forward_slash)) {
			forward_slash = win_forward_slash;
		}
#endif
		if (forward_slash)
		{
			filename_ = forward_slash + 1;
		}
		stream_.fmt(" [ %s:%d ]\n", filename_, line_);
	}
};
}

#define LOG_TRACE if(HooLog::GetLogLevel() <= HooLog::loglevel::TRACE) \
	HooLog::Logger(HooLog::loglevel::TRACE,__FILE__,__LINE__).stream()

#define LOG_DEBUG if(HooLog::GetLogLevel() <= HooLog::loglevel::DEBUG) \
	HooLog::Logger(HooLog::loglevel::DEBUG,__FILE__,__LINE__).stream()

#define LOG_INFO if(HooLog::GetLogLevel() <= HooLog::loglevel::INFO) \
	HooLog::Logger(HooLog::loglevel::INFO,__FILE__,__LINE__).stream()

#define LOG_WARN if(HooLog::GetLogLevel() <= HooLog::loglevel::WARN) \
	HooLog::Logger(HooLog::loglevel::WARN,__FILE__,__LINE__).stream()

#define LOG_ERROR if(HooLog::GetLogLevel() <= HooLog::loglevel::ERROR) \
	HooLog::Logger(HooLog::loglevel::ERROR,__FILE__,__LINE__).stream()

#define LOG_FATAL if(HooLog::GetLogLevel() <= HooLog::loglevel::FATAL) \
	HooLog::Logger(HooLog::loglevel::FATAL,__FILE__,__LINE__).stream()

#endif //LOGGER_H