#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H

#include <set>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include "Util/common.h"
#include "TimeStamp.h"
#include "Timer.h"

class EventLoop;
class Channel;
class TimerQueue
{
public:
	DISALLOW_COPY_AND_MOVE(TimerQueue)
	TimerQueue(EventLoop* loop);
	~TimerQueue();

	void CreateTimerfd(); //创建timerfd
    void ReadTimerFd(); // 读取timerfd事件
	void HandleRead(); //timerfd可读时调用

	void ResetTimerFd(Timer::TimerPtr timer); //重新设置timerfd超时时间，关注新的定时任务
	void ResetTimers(); //将具有重复属性的定时器重新加入队列

	bool Insert(Timer::TimerPtr timer); //将定时任务插入队列
	Timer::TimerPtr AddTimer(TimeStamp timestamp, std::function<bool()>const& cb, double interval, TimeUnit unit = TimeUnit::SECONDS); //添加一个定时任务

private:
	using  Entry =  std::pair<TimeStamp, Timer::TimerPtr>;

	EventLoop* loop_;
	int timerfd_;
	std::unique_ptr<Channel> channel_;

	std::set<Entry> timers_; //定时器集合
	std::vector<Entry> active_timers_; //激活的定时器
};

#endif // TIMERQUEUE_H