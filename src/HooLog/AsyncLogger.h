#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include "LogStream.h"
#include "LogFile.h"

#include <memory>
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace HooLog {
static const int FixedLargeBufferSize = 1024;
static const int64_t FileMaximumSize = 1024 * 1024 * 10;//单个文件最大的容量

class AsyncLogger {
public:
	using Buffer = FixedBuffer<FixedLargeBufferSize>;
	AsyncLogger(const std::string filepath = "") :filepath_(filepath),running_(false) {
		logfile_ = std::unique_ptr<LogFile>(new LogFile(filepath_));
		current_ = std::unique_ptr<Buffer>(new Buffer());
		next_ = std::unique_ptr<Buffer>(new Buffer());
	}

	~AsyncLogger() {
		if (running_.load()) {
			Stop();
		}
	}

	void Start() {
		if (!running_.load()) {
			running_ = true;
			thread_ = std::thread(std::bind(&AsyncLogger::ThreadFunc, this));
		}
	}

	void Stop() {
		running_ = false;
		conv_.notify_one();
		thread_.join();
	}

	void AppendNonCache(const char* msg,int length) {
		fwrite(msg, 1, length, stdout);	// 默认写出到stdout

		std::unique_lock<std::mutex> lock(mutex_);
		current_->append(msg, length);
		buffers_.push_back(std::move(current_));
		if (next_) {
			current_ = std::move(next_);
		}
		else {
			current_.reset(new Buffer);
		}
		conv_.notify_one();
	}

	void AppendCache(const char* msg, int length) {
		fwrite(msg, 1, length, stdout);	// 默认写出到stdout

		std::unique_lock<std::mutex> lock(mutex_);
		if (current_->avail() > length) {
			current_->append(msg, length);
		}
		else {
			buffers_.push_back(std::move(current_));
			if (next_) {
				current_ = std::move(next_);
			}
			else {
				current_.reset(new Buffer);
			}
			current_->append(msg, length);
		}
		conv_.notify_one();
	}

	void Flush() {
		fflush(stdout);		// 默认flush到stdout

		logfile_->Flush();
	}

	void ThreadFunc() {

		std::unique_ptr<Buffer> new_current = std::unique_ptr<Buffer>(new Buffer);
		std::unique_ptr<Buffer> new_next = std::unique_ptr<Buffer>(new Buffer);
		new_current->bzero();
		new_next->bzero();

		std::vector<std::unique_ptr<Buffer>> active_buffers;

		while (running_.load()) {
			std::unique_lock<std::mutex> lock(mutex_);
			if (buffers_.empty()) {
				conv_.wait_for(lock, std::chrono::milliseconds(500), [this]() {return !buffers_.empty();});
			}

			buffers_.push_back(std::move(current_));
			active_buffers.swap(buffers_);

			current_ = std::move(new_current);

			if (!next_) {
				next_ = std::move(new_next);
			}

			for (const auto& buffer : active_buffers) {
				logfile_->Write(buffer->start(),buffer->len());
			}

			if (logfile_->writtenbytes() >= FileMaximumSize) {
				logfile_->ReCurrentLog();
				logfile_.reset(new LogFile(filepath_));
			}

			if (!new_current && !active_buffers.empty()) {
				new_current = std::move(active_buffers.back());
				active_buffers.pop_back();
				new_current->bzero();
			}

			if (!new_next && !active_buffers.empty()) {
				new_next = std::move(active_buffers.back());
				active_buffers.pop_back();
				new_next->bzero();
			}

			active_buffers.clear();
		}
	}


private:
	std::atomic<bool> running_;
	std::thread thread_;
	std::mutex mutex_;
	std::condition_variable conv_;

	std::string filepath_;
	std::unique_ptr<LogFile> logfile_;
	std::unique_ptr<Buffer> current_;
	std::unique_ptr<Buffer> next_;
	std::vector<std::unique_ptr<Buffer>> buffers_;
};

}

#endif