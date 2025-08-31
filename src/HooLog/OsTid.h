#ifndef OS_TID_H
#define OS_TID_H

#include <thread>

#ifdef _WIN32
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#include<windows.h>
inline unsigned long os_tid() noexcept {
	return ::GetCurrentThreadId();
}

#elif defined(__linux__)
#include <unistd.h>
#include <sys/syscall.h>
inline pid_t os_tid() noexcept {
	return static_cast<pid_t>(::syscall(SYS_gettid));
}

#else
inline unsigned long long os_tid() noexcept {
	return std::hash<std::thread::id>()(std::this_thread::get_id());
}

#endif
#endif