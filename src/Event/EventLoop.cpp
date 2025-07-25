#include "EventLoop.h"

#include "NetWork/Channel.h"
#include "NetWork/Epoller.h"

#include "Timer/TimerQueue.h"
#include "Timer/TimeStamp.h"
#include <memory>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include "Util/CurrentThread.h"
#include <sys/eventfd.h>
#include <assert.h>


EventLoop::EventLoop() : tid_(CurrentThread::tid()) { 
    // 将Loop函数分配给了不同的线程，获取执行该函数的线程
	poller_ = std::unique_ptr<Epoller>(new Epoller());

    timer_queue_ = std::unique_ptr<TimerQueue>(new TimerQueue(this));

    wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    wakeup_channel_ = std::unique_ptr<Channel>(new Channel(wakeup_fd_, this));
    calling_functors_ = false;
    wakeup_channel_->set_read_callback(std::bind(&EventLoop::HandleRead, this));
    wakeup_channel_->EnableRead();
}

EventLoop::~EventLoop() {
    DeleteChannel(wakeup_channel_.get());
    ::close(wakeup_fd_);
}

void EventLoop::Loop(){
    while (run_)
    {
        for (Channel *active_ch : poller_->Poll()){
            active_ch->HandleEvent();
        }
        DoToDoList();
    }
}

void EventLoop::Stop() {
    run_ = false;
}

void EventLoop::UpdateChannel(Channel *ch) { poller_->UpdateChannel(ch); }
void EventLoop::DeleteChannel(Channel *ch) { poller_->DeleteChannel(ch); }

bool EventLoop::IsInLoopThread(){
    return CurrentThread::tid() == tid_;
}

void EventLoop::RunOneFunc(std::function<void()> cb){
    if(IsInLoopThread()){
        cb();
    }else{
        QueueOneFunc(cb);
    }
}

void EventLoop::QueueOneFunc(std::function<void()> cb){
    {
        // 加锁，保证线程同步
        std::unique_lock<std::mutex> lock(mutex_);
        to_do_list_.emplace_back(std::move(cb));
    }

    // 如果调用当前函数的并不是当前当前EventLoop对应的的线程，将其唤醒。主要用于关闭TcpConnection
    // 由于关闭连接是由对应`TcpConnection`所发起的，但是关闭连接的操作应该由main_reactor所进行(为了释放ConnectionMap的所持有的TcpConnection)
    if (!IsInLoopThread() || calling_functors_) {
        uint64_t write_one_byte = 1;  
        ssize_t write_size = ::write(wakeup_fd_, &write_one_byte, sizeof(write_one_byte));
        (void) write_size;
        assert(write_size == sizeof(write_one_byte));
    } 
}

void EventLoop::DoToDoList(){
    // 此时已经epoll_wait出来，可能存在阻塞在epoll_wait的可能性。
    calling_functors_ = true;

    std::vector < std::function<void()>> functors;
    {
        // 加锁 保证线程同步
        std::unique_lock<std::mutex> lock(mutex_); 
        functors.swap(to_do_list_);
    }
    for(const auto& func: functors){
        func();
    }

    calling_functors_ = false;
}

void EventLoop::HandleRead(){
    // 用于唤醒EventLoop
    uint64_t read_one_byte = 1;
    ssize_t read_size = ::read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));
    (void) read_size;
    assert(read_size == sizeof(read_one_byte));
    return;
}

void EventLoop::RunAt(TimeStamp timestamp, std::function<bool()>const& cb) {
    timer_queue_->AddTimer(timestamp,std::move(cb),0.0);
}

void EventLoop::RunAfter(double wait_time, std::function<bool()>const& cb, TimeUnit unit) {
    TimeStamp timestamp(TimeStamp::AddTime(TimeStamp::Now(), wait_time, unit));
    timer_queue_->AddTimer(timestamp, std::move(cb), 0.0, unit);
}

Timer::TimerPtr EventLoop::RunEvery(double interval, std::function<bool()>const& cb, TimeUnit unit) {
    TimeStamp timestamp(TimeStamp::AddTime(TimeStamp::Now(), interval, unit));
    return timer_queue_->AddTimer(timestamp, std::move(cb), interval, unit);
}