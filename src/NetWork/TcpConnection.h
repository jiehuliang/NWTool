#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include "Util/common.h"

#include <functional>
#include <memory>
#include <string>
#include "Timer/TimeStamp.h"

class Buffer;
class HttpContext;
class SessionBase;
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    enum ConnectionState
    {
        Invalid = 1,
        Connected,
        Disconected
    };

    DISALLOW_COPY_AND_MOVE(TcpConnection);

    
    TcpConnection(EventLoop *loop, int connfd, int connid);
    ~TcpConnection();

    // ��ʼ��TcpConnection
    void ConnectionEstablished();

    // ����TcpConection
    void ConnectionDestructor();

    // ��������ʱ���ûص�����
    void set_connection_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn);
     // �ر�ʱ�Ļص�����
    void set_close_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn);   
    // ���ܵ���Ϣ�Ļص�����                                  
    void set_message_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn);

    void set_session(SessionBase* session);

    // �趨send buf
    Buffer *read_buf();
    Buffer *send_buf();

    void Read(); // ������
    void Write(); // д����
    void Send(const std::string &msg); // �����Ϣ
    void Send(const char *msg, int len); // �����Ϣ
    void Send(const char *msg);


    void HandleMessage(); // �����յ���Ϣʱ�����лص�
    void HandleWrite(); //д����

    // ��TcpConnection����ر�����ʱ�����лص����ͷ���Ӧ��socket.
    void HandleClose(); 
    ConnectionState state() const;
    EventLoop *loop() const;
    int fd() const;
    int id() const;
    HttpContext *context() const;
    SessionBase* session() const;

    TimeStamp timestamp() const;
    void UpdateTimeStamp(TimeStamp now);


private:
    // �����Ӱ󶨵�Socket
    int connfd_;
    // Ϊ�����ӷ���һ��id���������debug
    int connid_;

    // ����״̬
    ConnectionState state_;

    EventLoop *loop_;

    std::unique_ptr<Channel> channel_;
    std::unique_ptr<Buffer> read_buf_;
    std::unique_ptr<Buffer> send_buf_;

    std::function<void(const std::shared_ptr<TcpConnection> &)> on_close_;
    std::function<void(const std::shared_ptr<TcpConnection> &)> on_message_;
    std::function<void(const std::shared_ptr<TcpConnection> &)> on_connect_;

    void ReadNonBlocking();
    void WriteNonBlocking();

    std::unique_ptr<HttpContext> context_;

    //ҵ��Session����������Connection��ͬ
    SessionBase* session_;

    // ��ҪƵ����ֵ��ʹ����ͨ��Ա������
    TimeStamp timestamp_;
};


class SessionBase {
public:
    SessionBase();
    virtual ~SessionBase();
};

#endif // TCPCONNECTION_H