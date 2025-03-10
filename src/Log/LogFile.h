#ifndef LOG_FILE_H
#define LOG_FILE_H

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>
#include "Timer/TimeStamp.h"

static const  time_t FlushInterval = 3;

class LogFile {
public:
	LogFile(const char* filepath = nullptr);
	~LogFile();

	void Write(const char* data, int len);

	void Flush();

	int64_t writtenbytes() const;
private:
	FILE* fp_;
	int64_t written_bytes_;
	time_t lastwrite_;
	time_t lastflush_;
};

#endif // LOG_FILE_H