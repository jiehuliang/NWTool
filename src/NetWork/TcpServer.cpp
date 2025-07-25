#include "TcpServer.h"
#include "TcpConnection.h"
#include "Event/EventLoop.h"
#include "Acceptor.h"
#include "Thread/EventLoopThreadPool.h"
#include "Util/common.h"
#include "Util/CurrentThread.h"
#include <memory>
#include <assert.h>
#include <iostream>
#include "Log/Logging.h"

TcpServer::TcpServer(EventLoop *loop, const char * ip, const int port): main_reactor_(loop), next_conn_id_(1){
    //
    acceptor_ = std::unique_ptr<Acceptor>(new Acceptor(main_reactor_, ip, port));
    std::function<void(int)> cb = std::bind(&TcpServer::HandleNewConnection, this, std::placeholders::_1);
    acceptor_->set_newconnection_callback(cb);

    // 创建线程池
    thread_pool_ = std::unique_ptr<EventLoopThreadPool>(new EventLoopThreadPool(loop));
    // std::cout << "Tcpserver listening on " << ip << ":" << port << std::endl;
}

TcpServer::~TcpServer(){
};

void TcpServer::Start(){
    // 创建子线程和对应的EventLoop
    thread_pool_->start();

    // 主线程启动
    main_reactor_->Loop();
}

inline void TcpServer::HandleNewConnection(int fd){
    assert(fd != -1);
    // uint64_t random = fd % sub_reactors_.size();

    // 从线程池中获得一个EventLoop
    EventLoop *sub_reactor = thread_pool_->nextloop();

    std::shared_ptr<TcpConnection> conn = std::make_shared<TcpConnection>(sub_reactor,  fd, next_conn_id_);
    
    std::function<void(const std::shared_ptr<TcpConnection> &)> cb = std::bind(&TcpServer::HandleClose, this, std::placeholders::_1);
    conn->set_connection_callback(on_connect_);

    //
    conn->set_close_callback(cb);
    conn->set_message_callback(on_message_);
    connectionsMap_[fd] = conn;
    //
    ++next_conn_id_;
    if(next_conn_id_ == 1000){
        next_conn_id_ = 1;
    }
    //
    conn->ConnectionEstablished();
}


inline void TcpServer::HandleClose(const std::shared_ptr<TcpConnection> & conn){
    main_reactor_->RunOneFunc(std::bind(&TcpServer::HandleCloseInLoop, this, conn));
}

inline void TcpServer::HandleCloseInLoop(const std::shared_ptr<TcpConnection> & conn){
    LOG_INFO << "TcpServer::HandleCloseInLoop - Remove connection [id#" <<  conn->id() << "-fd#" << conn->fd() << "]";
    auto it = connectionsMap_.find(conn->fd());
    assert(it != connectionsMap_.end());
    connectionsMap_.erase(it);
    //connectionsMap_.erase(connectionsMap_.find(conn->fd()));

    EventLoop *loop = conn->loop();
    loop->RunOneFunc(std::bind(&TcpConnection::ConnectionDestructor, conn));
}

void TcpServer::set_connection_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn) { on_connect_ = std::move(fn); };
void TcpServer::set_message_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn) { on_message_ = std::move(fn); };

void TcpServer::SetThreadNums(int thread_nums) { thread_pool_->SetThreadNums(thread_nums); }
