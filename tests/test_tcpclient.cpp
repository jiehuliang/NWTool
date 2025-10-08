#include "NetWork/Connector.h"
#include "Event/EventLoop.h"
#include "Thread/EventLoopThreadPool.h"
#include "NetWork/TcpConnection.h"
#include "NetWork/TcpClient.h"
#include "Util/Buffer.h"
#include "Util/CurrentThread.h"
#include "HooLog/HooLog.h"
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
    // ��ȡ�������ӵ�Ip��ַ��port�˿�
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

    setLogLevel(loglevel::DEBUG);

    std::shared_ptr<AsyncLogger> asyncLogger = std::make_shared<AsyncLogger>();
    setOutputFunc(std::bind(&AsyncLogger::AppendNonCache, asyncLogger, std::placeholders::_1, std::placeholders::_2));
    setFlushFunc(std::bind(&AsyncLogger::Flush, asyncLogger));
    asyncLogger->Start();

    int size = std::thread::hardware_concurrency() - 1;
    std::unique_ptr<EventLoopThreadPool> event_loop_thread_pool = std::unique_ptr<EventLoopThreadPool>(new EventLoopThreadPool(nullptr));
    event_loop_thread_pool->SetThreadNums(size);
    event_loop_thread_pool->start();

    EchoClient* client = new EchoClient(event_loop_thread_pool->nextloop(), "127.0.0.1", port);

    // 2. �����̣߳������� stdin��Ȼ�����Ϣ�׵� loop �̷߳���
    std::thread keyboard([&] {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "quit") break;
            // ��֤�̰߳�ȫ��������Ͷ�ݵ� IO �߳�
            loop->RunOneFunc([=] {
                client->write(line + "\n");  // EchoClient ���еķ��ͽӿ�
                });
        }
        client->close();
        // �û����� quit -> �����˳�
        loop->RunOneFunc([&] { loop->Stop(); });
        });

    client->start();

    keyboard.join();
    delete loop;
    delete client;
    return 0;
}