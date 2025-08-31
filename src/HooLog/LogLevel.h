#ifndef LOG_LEVEL_H
#define LOG_LEVEL_H

#include <string>

namespace HooLog {

class LogLevel {
public:
	enum class value{
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL
	};

	static const char* toString(value level) {
		switch (level) {
		case value::TRACE:
			return "TRACE";
		case value::DEBUG:
			return "DEBUG";
		case value::INFO:
			return "INFO";
		case value::WARN:
			return "WARN";
		case value::ERROR:
			return "ERROR";
		case value::FATAL:
			return "FATAL";
		}
		return nullptr;
	}
};

}


#endif //LOG_LEVEL_H