#ifndef CHANNEL_H
#define CHANNEL_H
#include "Util/common.h"

#include <functional>
#include <memory>

class EventLoop;
class Channel {
    public:
        DISALLOW_COPY_AND_MOVE(Channel);
        Channel(int fd, EventLoop * loop);
        
        ~Channel();

        void HandleEvent() const; // 处理事件
        void HandleEventWithGuard() const;
        void EnableRead();  // 允许读
        void EnableWrite(); // 允许写
        void EnableET(); // 以ET形式触发
        void disableAll(); // 以ET形式触发

        int fd() const;  // 获取fd
        short listen_events() const; // 监听的事件
        short ready_events() const; // 准备好的事件

        bool IsInEpoll() const; // 判断当前channel是否在poller中
        void SetInEpoll(bool in = true); // 设置当前状态为poller中
        

        void SetReadyEvents(int ev);
        void set_read_callback(std::function<void()> const &callback);// 设置回调函数
        void set_write_callback(std::function<void()> const &callback);

        void Tie(const std::shared_ptr<void> &ptr); // 设定tie
     
    private:
        int fd_;
        EventLoop *loop_;
        
        short listen_events_;
        short ready_events_;
        bool in_epoll_{false};
        std::function<void()> read_callback_;
        std::function<void()> write_callback_;

        bool tied_;
        std::weak_ptr<void> tie_;

};

#endif // CHANNEL_H
