#include "Connector.h"

#include "HooLog/HooLog.h"
#include "Event/EventLoop.h"

#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <error.h>
#include <unistd.h>

Connector::Connector(EventLoop* loop,const char* ip,const int port):loop_(loop),socket_fd_(-1),ip_(ip),port_(port) {

}

Connector::~Connector() {

}

void Connector::Start() {
	loop_->RunOneFunc(std::bind(&Connector::StartInLoop,this));
}

void Connector::StartInLoop() {
	Create();
	Connection(ip_, port_);
}

void Connector::Create() {
	assert(socket_fd_ == -1);
	socket_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (socket_fd_ == -1) {
		LOG_ERROR << "Failed to create socket";
	}
}

bool Connector::Connection(const char* ip, const int port) {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	LOG_INFO << "connect socketfd:" << socket_fd_ << ",ip:" << ip << ",port:" << port;
	int ret = connect(socket_fd_, (struct sockaddr*)&addr, sizeof(addr));
	int error = ret == 0 ? 0 : errno;
	switch (error) {
	case 0:
	case EINPROGRESS:
	case EINTR:
	case EISCONN:
		connecting(socket_fd_);
	case EAGAIN:
	case EADDRINUSE:
	case EADDRNOTAVAIL:
	case ECONNREFUSED:
	case ENETUNREACH:
		//retry
		break;
	case EACCES:
	case EPERM:
	case EAFNOSUPPORT:
	case EALREADY:
	case EBADF:
	case EFAULT:
	case ENOTSOCK:
		LOG_ERROR << "connect error in Connector::startInLoop " << error;
		::close(socket_fd_);
		break;
	default:
		LOG_ERROR << "Unexpected error in Connector::startInLoop " << error;
		::close(socket_fd_);
		break;
	}

	return true;
}

void Connector::connecting(int socket_fd_) {
	channel_.reset(new Channel(socket_fd_,loop_));
	channel_->set_write_callback(std::bind(&Connector::handleWrite, this));
	channel_->EnableWrite();
}

void Connector::resetChannel()
{
	channel_.reset();
}

void Connector::handleWrite() {
	channel_->disableAll();
	loop_->DeleteChannel(channel_.get());
	loop_->RunOneFunc(std::bind(&Connector::resetChannel, this));
	int optval = -1;
	socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
	if (::getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) == 0 && optval == 0) {
		connection_callback(socket_fd_);
	}
	else {
		LOG_ERROR << "Unexpected error in Connector::handleWrite,optval: " << optval ;
	}

}

void Connector::set_connection_callback(std::function<void(int)>const & cb) {
	connection_callback = cb;
}

