
#include "TimeStamp.h"
#include "TimerQueue.h"

#include "NetWork/Channel.h"
#include "Event/EventLoop.h"
#include  <cstring>
#include <sys/timerfd.h>
#include <assert.h>
#include <iostream>
#include <unistd.h>
TimerQueue::TimerQueue(EventLoop *loop) :loop_(loop){
	CreateTimerfd();
	channel_ = std::unique_ptr<Channel>(new Channel(timerfd_,loop_));
	channel_->set_read_callback(std::bind(&TimerQueue::HandleRead,this));
	channel_->EnableRead();
}
void TimerQueue::CreateTimerfd(){
	timerfd_ = ::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);
	if(timerfd_ < 0){
        printf("TimerQueue::CreateTimerfd error");
	}
}
TimerQueue::~TimerQueue(){
	loop_->DeleteChannel(channel_.get());
	::close(timerfd_);
	//for(const auto& entry : timers_){	
	//	delete entry.second;
	//}
	timers_.clear();
}
void TimerQueue::ReadTimerFd(){
	uint64_t read_byte;
	ssize_t readn = ::read(timerfd_,&read_byte,sizeof(read_byte));
	if(readn != sizeof(read_byte)){
        printf("TimerQueue::ReadTimerFd read error");
	}
}
void TimerQueue::HandleRead(){
	ReadTimerFd();
	active_timers_.clear();
	//auto end = timers_.lower_bound(Entry(TimeStamp::Now(), Timer::TimerPtr(reinterpret_cast<Timer *>(UINTPTR_MAX))));
	auto end = timers_.lower_bound(Entry(TimeStamp::Now(), nullptr));
	active_timers_.insert(active_timers_.end(),timers_.begin(),end);
	timers_.erase(timers_.begin(),end);
	for (const auto &entry : active_timers_)
	{
		entry.second->run();
	}
	ResetTimers();
}
Timer::TimerPtr TimerQueue::AddTimer(TimeStamp timestamp,std::function<bool()>const &cb,double interval, TimeUnit unit){
	Timer::TimerPtr timer = std::make_shared<Timer>(timestamp, cb, interval, unit);
	if(Insert(timer))
	{
		ResetTimerFd(timer);
	}
	return timer;
}
bool TimerQueue::Insert(Timer::TimerPtr timer){
	bool reset_instantly = false;
	if(timers_.empty() || timer->expiration() < timers_.begin()->first){
		reset_instantly = true;
	}
	timers_.emplace(std::move(Entry(timer->expiration(),timer)));
	return reset_instantly;
}
void TimerQueue::ResetTimers(){
	for(auto &entry : active_timers_){
		if((entry.second)->repeat()){
			auto timer = entry.second;
			timer->ReStart(TimeStamp::Now());
			Insert(timer);
		}
		//else{
		//	delete entry.second;
		//}
	}
	active_timers_.clear();
	if (!timers_.empty()){
			ResetTimerFd(timers_.begin()->second);
	}
}
void TimerQueue::ResetTimerFd(Timer::TimerPtr timer) {
	struct itimerspec new_;
	struct itimerspec old_;
	memset(&new_, '\0', sizeof(new_));
	memset(&old_, '\0', sizeof(old_));
	int64_t micro_seconds_dif = timer->expiration().microseconds() - TimeStamp::Now().microseconds();
	if (micro_seconds_dif < 100) {
		micro_seconds_dif = 100;
	}
	new_.it_value.tv_sec = static_cast<time_t>(micro_seconds_dif / kMicrosecond2Second);
	new_.it_value.tv_nsec = static_cast<long>((micro_seconds_dif % kMicrosecond2Second) * 1000);

	int ret = ::timerfd_settime(timerfd_, 0, &new_, &old_);
	assert(ret != -1);
	(void)ret;
}