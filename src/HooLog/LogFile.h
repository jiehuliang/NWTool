#ifndef LOG_FILE_H
#define LOG_FILE_H

#include <chrono>
#include <stdio.h>
#include <string>
#include <ctime>


#if defined(_WIN32)
#include <io.h>
#include <direct.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/stat.h>
#endif 

#if defined(_WIN32) || defined(_WIN64)
#define fwrite_unlock _fwrite_nolock
static const char* path_assign = "\\";
static const std::string DefaultPath = ".\\Log\\hoolog.log";


#define fseek64 _fseeki64
#define ftell64 _ftelli64

#define access _access

inline int mkdir(const char* path, int mode) {
	return _mkdir(path);
}
#elif defined(__linux__)
#define fwrite_unlock fwrite_unlocked
static const char* path_assign = "/";
static const std::string DefaultPath = "./Log/hoolog.log";

#define fseek64 fseek
#define ftell64 ftell

#endif


static const int FlushInterval = 1;


class LogFile {
public:
	LogFile(const std::string filepath = "") : fp_(nullptr), lastwrite_(0), lastflush_(0) {
		if (!filepath.empty()) {
			log_path_ = filepath;
		}
		else {
			log_path_ = DefaultPath;
		}
		written_bytes_ = LogFileSize(log_path_);
	}

	~LogFile() {
		Flush();
		if (fp_) {
			fclose(fp_);
			fp_ = nullptr;
		}
	}

	static FILE* CreateLogFile(const std::string& path,const std::string& mod) {
		std::string localpath = path;
		std::string dir;
		size_t index = 1;
		while (true) {
			index = localpath.find(path_assign, index) + 1;
			dir = localpath.substr(0, index);
			if (dir.length() == 0) {
				break;
			}
			if (access(dir.c_str(), 0) == -1) {
				if (mkdir(dir.c_str(), 0777) == -1) {
					return nullptr;
				}
			}
		}
		FILE* ret = nullptr;
		if (localpath[localpath.size() - 1] != path_assign[sizeof(path_assign) - 1]) {
			ret = fopen(path.c_str(), mod.c_str());
		}
		return ret;
	}

	static bool LogFileExist(const std::string& path) {
		auto fp = fopen(path.c_str(), "rb");
		if (!fp) {
			return false;
		}
		fclose(fp);
		return true;
	}

	static uint64_t LogFileSize(const std::string& path) {
		if (path.empty()) {
			return 0;
		}
		auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(path.data(), "rb"), fclose);
		if (!fp.get()) {
			return 0;
		}
		auto current = ftell64(fp.get());
		fseek64(fp.get(), 0L, SEEK_END); /* 定位到文件末尾 */
		auto end = ftell64(fp.get()); /* 得到文件大小 */
		fseek64(fp.get(), current, SEEK_SET);
		return end - 0;
	}

	void Write(const char* data, int len) {
		if (!fp_) {
			fp_ = CreateLogFile(log_path_, "a+");
		}
		int pos = 0;
		while (pos != len) {
			int written = fwrite_unlock(data + pos, sizeof(char), len - pos, fp_);
			if (written <= 0) {
				return ;
			}
			pos += written;
		}

		std::chrono::steady_clock::duration duration = std::chrono::steady_clock::now().time_since_epoch();
		int64_t now = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

		if (len != 0) {
			lastwrite_ = now;
			written_bytes_ += len;
		}
		//定时刷新太慢了,实时性太差
		//if (lastwrite_ - lastflush_ > FlushInterval) {
		//	Flush();
		//	lastflush_ = now;
		//}
		Flush();
	}

	void Flush() {
		if (fp_) {
			fflush(fp_);
		}
	}

	void ReCurrentLog() {
		if (fp_) {
			fclose(fp_);
			fp_ = nullptr;
		}
		if (LogFileExist(log_path_)) {
			std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			time_t now_c = std::chrono::system_clock::to_time_t(now);
			tm* local_time = localtime(&now_c);
			char time[64]{ 0 };
			snprintf(time, sizeof(time), "%4d%02d%02d%02d%02d%02d", local_time->tm_year + 1900, local_time->tm_mon + 1
				, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

			std::string new_log_file_name = log_path_ + "." + time;
			rename(log_path_.c_str(), new_log_file_name.c_str());
		}
	}

	int64_t writtenbytes() const {
		return written_bytes_;
	}

private:
	std::string log_path_;
	FILE* fp_;
	int64_t written_bytes_;
	int64_t lastwrite_;
	int64_t lastflush_;
};

#endif 
