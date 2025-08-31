#ifndef LOG_STREAM_H
#define LOG_STREAM_H

#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include <utility>

namespace HooLog {
static const int FixedBufferSize = 4096;
static const int kMaxNumericSize = 48;

template<int SIZE>
class FixedBuffer {
public:
	FixedBuffer();
	~FixedBuffer();

	void append(const char* buf, int len);
	void bzero();

	char* current();
	const char* start();
	const char* end();

	int len();
	int avail();
	void add(int len);


private:
	char data_[SIZE];
	char* cur_;
};


class LogStream {
public:
	LogStream(const LogStream&) = delete;
	LogStream& operator=(const LogStream&) = delete;
	LogStream(LogStream&&) = delete;
	LogStream& operator=(LogStream&&) = delete;

	using Buffer = FixedBuffer<FixedBufferSize>;

	LogStream() {}
	~LogStream() {}

	void append(const char* data, int len);

	template<class... Args>
	void fmt(const char* fmt , Args&&... args);
	
	const Buffer& buffer();

	const char* data();

	int length();

	void resetBuffer();

	void toString();

	LogStream& operator<<(bool v);

	LogStream& operator<<(short num);
	LogStream& operator<<(int num);
	LogStream& operator<<(long num);
	LogStream& operator<<(long long num);

	LogStream& operator<<(unsigned short num);
	LogStream& operator<<(unsigned int num);
	LogStream& operator<<(unsigned long num);
	LogStream& operator<<(unsigned long long num);

	LogStream& operator<<(const float& num);
	LogStream& operator<<(const double& num);

	LogStream& operator<<(char data);
	LogStream& operator<<(const char* data);
	LogStream& operator<<(const std::string& str);

private:
	template<class T>
	void formatInteger(T value);
	Buffer buffer_;
};

template<int SIZE>
FixedBuffer<SIZE>::FixedBuffer() :cur_(data_) {}

template<int SIZE>
FixedBuffer<SIZE>::~FixedBuffer() {}

template<int SIZE>
void FixedBuffer<SIZE>::append(const char* buf, int len) {
	if (avail() > len) {
		memcpy(cur_, buf, len);
		cur_ += len;
	}
}

template<int SIZE>
void FixedBuffer<SIZE>::bzero() {
	memset(data_, '\0', sizeof(data_));
	cur_ = data_;
}

template<int SIZE>
char* FixedBuffer<SIZE>::current() {
	return cur_;
}

template<int SIZE>
const char* FixedBuffer<SIZE>::start() {
	return data_;
}

template<int SIZE>
const char* FixedBuffer<SIZE>::end() {
	return data_ + sizeof(data_);
}

template<int SIZE>
int FixedBuffer<SIZE>::len() {
	return static_cast<int>(cur_ - data_);
}

template<int SIZE>
int FixedBuffer<SIZE>::avail() {
	return static_cast<int>(end() - cur_);
}

template<int SIZE>
void FixedBuffer<SIZE>::add(int len) {
	cur_ += len;
}

template<class... Args>
void LogStream::fmt(const char* fmt, Args&&... args) {
	int avail = buffer_.avail();
	int length = snprintf(buffer_.current(), avail,fmt,std::forward<Args>(args)...);
	int add = length > avail - 1 ? avail - 1 : length;
	buffer_.add(add);
}

inline void LogStream::append(const char* data, int len) {
	buffer_.append(data, len);
}

inline const LogStream::Buffer& LogStream::buffer() {
	return buffer_;
}

inline const char* LogStream::data() {
	return buffer_.start();
}

inline int LogStream::length() {
	return buffer_.len();
}

inline void LogStream::resetBuffer() {
	buffer_.bzero();
}

inline void LogStream::toString() {
	std::cout << buffer_.start();
}

inline LogStream& LogStream::operator<<(bool v) {
	buffer_.append(v ? "true" : "false", v ? 4 : 5);
	return *this;
}

inline LogStream& LogStream::operator<<(short num) {
	return *this << static_cast<int>(num);
}
inline LogStream& LogStream::operator<<(int num) {
	formatInteger(num);
	return *this;
}
inline LogStream& LogStream::operator<<(long num) {
	formatInteger(num);
	return *this;
}
inline LogStream& LogStream::operator<<(long long num) {
	formatInteger(num);
	return *this;
}

inline LogStream& LogStream::operator<<(unsigned short num) {
	return *this << static_cast<unsigned int>(num);
}
inline LogStream& LogStream::operator<<(unsigned int num) {
	formatInteger(num);
	return *this;
}
inline LogStream& LogStream::operator<<(unsigned long num) {
	formatInteger(num);
	return *this;
}
inline LogStream& LogStream::operator<<(unsigned long long num) {
	formatInteger(num);
	return *this;
}

inline LogStream& LogStream::operator<<(const float& num) {
	return *this << static_cast<double>(num);
}
inline LogStream& LogStream::operator<<(const double& num) {
	char buf[32];
	int len = snprintf(buf, sizeof(buf), "%g", num);
	len = len > static_cast<int>(sizeof(buf) - 1) ? sizeof(buf) - 1 : len;
	buffer_.append(buf, len);
	return *this;
}

inline LogStream& LogStream::operator<<(char data) {
	buffer_.append(&data, 1);
	return *this;
}
inline LogStream& LogStream::operator<<(const char* data) {
	buffer_.append(data, strlen(data));
	return *this;
}
inline LogStream& LogStream::operator<<(const std::string& v) {
	buffer_.append(v.c_str(), v.size());
	return *this;
}

template<class T>
void LogStream::formatInteger(T value) {
	if (buffer_.avail() > kMaxNumericSize) {
		char* current = buffer_.current();
		char* now = current;

		bool negative = false;
		if (value < 0) {
			value = -value;
			negative = true;
		}
		do {
			unsigned int remainder = static_cast<unsigned int> (value % 10);
			*(now++) = '0' + remainder;
			value /= 10;
		} while (value != 0);
		if (negative) {
			*(now++) = '-';
		}

		std::reverse(current,now);
		buffer_.add(now - current);
	} 
}





}

#endif 