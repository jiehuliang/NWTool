#include "NetWork/Connector.h"
#include "Event/EventLoop.h"
#include "NetWork/TcpConnection.h"
#include "NetWork/TcpClient.h"
#include "Util/Buffer.h"
#include "Util/CurrentThread.h"
#include <iostream>
#include <functional>
#include <arpa/inet.h>
#include <vector>

class EchoClient {
public:
    EchoClient(EventLoop* loop, const char* ip, const int port);
    ~EchoClient();

    void start();
    void write(const std::string& message);
    void close();
    void onConnection(const std::shared_ptr<TcpConnection>& conn);
    void onMessage(const std::shared_ptr<TcpConnection>& conn);

private:
    TcpClient client_;
};

void EchoClient::start() {
    client_.Start();
}

void EchoClient::write(const std::string& message) {
    client_.Write(message);
}

void EchoClient::close() {
    client_.Close();
}

EchoClient::EchoClient(EventLoop* loop, const char* ip, const int port) : client_(loop, ip, port) {
    client_.set_connectio_callback(std::bind(&EchoClient::onConnection, this, std::placeholders::_1));
    client_.set_message_callback(std::bind(&EchoClient::onMessage, this, std::placeholders::_1));
};
EchoClient::~EchoClient() {};

void EchoClient::onConnection(const std::shared_ptr<TcpConnection>& conn) {
    // 获取接收连接的Ip地址和port端口
    int clnt_fd = conn->fd();
    struct sockaddr_in peeraddr;
    socklen_t peer_addrlength = sizeof(peeraddr);
    getpeername(clnt_fd, (struct sockaddr*)&peeraddr, &peer_addrlength);

    std::cout << "ThreadId:" << CurrentThread::tid()
        << " EchoServer::OnNewConnection : new connection "
        << "[fd#" << clnt_fd << "]"
        << " from " << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port)
        << std::endl;
};

void EchoClient::onMessage(const std::shared_ptr<TcpConnection>& conn) {
    // std::cout << CurrentThread::tid() << " EchoServer::onMessage" << std::endl;
    if (conn->state() == TcpConnection::ConnectionState::Connected)
    {
        std::cout << "ThreadId:" << CurrentThread::tid() << " Message from clent " << conn->read_buf()->RetrieveAllAsString() << std::endl;
        //conn->Send(conn->read_buf()->beginread(), conn->read_buf()->readablebytes());
        //conn->HandleClose();
    }
}

int main(int argc, char* argv[]) {
    int port;
    if (argc <= 1)
    {
        port = 1234;
    }
    else if (argc == 2) { 
        port = atoi(argv[1]);
    }
    else {
        printf("error");
        exit(0);
    }
    EventLoop* loop = new EventLoop();
    EchoClient* client = new EchoClient(loop, "127.0.0.1", port);

    // 2. 键盘线程：阻塞读 stdin，然后把消息抛到 loop 线程发送
    std::thread keyboard([&] {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "quit") break;
            // 保证线程安全：把任务投递到 IO 线程
            loop->RunOneFunc([=] {
                client->write(line + "\n");  // EchoClient 已有的发送接口
                });
        }
        client->close();
        // 用户输入 quit -> 优雅退出
        loop->RunOneFunc([&] { loop->Stop(); });
        });

    client->start();

    keyboard.join();
    delete loop;
    delete client;
    return 0;
}