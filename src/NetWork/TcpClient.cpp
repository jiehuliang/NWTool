#include "TcpClient.h"
#include "Event/EventLoop.h"
#include "TcpConnection.h"
#include "HooLog/HooLog.h"

TcpClient::TcpClient(EventLoop* loop, const char* ip, const int port):loop_(loop) {
	Connector_ = std::unique_ptr<Connector>(new Connector(loop_,ip,port));

	Connector_->set_connection_callback(std::bind(&TcpClient::newConnection,this,std::placeholders::_1));
}

TcpClient::~TcpClient() {}

void TcpClient::Start() {
	Connector_->Start();
	loop_->Loop();
}

void TcpClient::Write(const std::string& message) {
	conn_->Send(message);
}

void TcpClient::Close() {
	conn_->HandleClose();
}

void TcpClient::newConnection(int socket_fd) {
	conn_ = std::make_shared<TcpConnection>(loop_,socket_fd,0);
	conn_->set_close_callback(std::bind(&TcpClient::HandleClose,this,std::placeholders::_1));
	conn_->set_connection_callback(conn_callback);
	conn_->set_message_callback(message_callback);

	conn_->ConnectionEstablished();

}

void TcpClient::HandleClose(const std::shared_ptr<TcpConnection>& conn) {
	LOG_INFO << "TcpClient::HandleClose - Remove connection [id#" << conn->id() << "-fd#" << conn->fd() << "]";
	loop_->RunOneFunc(std::bind(&TcpConnection::ConnectionDestructor,conn));
	conn_.reset();
}

void TcpClient::set_connectio_callback(std::function<void(const std::shared_ptr<TcpConnection>&)> const& fn) {
	conn_callback = fn;
}

void TcpClient::set_message_callback(std::function<void(const std::shared_ptr<TcpConnection>&)> const& fn) {
	message_callback = fn;
}



