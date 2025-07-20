#ifndef CONNECTOR_H
#define CONNECTOR_H
#include "Util/common.h"
#include "Channel.h"
#include <memory>
#include <functional>

class Connector {
public:
	DISALLOW_COPY_AND_MOVE(Connector);

	Connector(EventLoop* loop,const char* ip, const int port);
	~Connector();

	void Start();
	void StartInLoop();
	void Create();
	bool Connection(const char* ip,const int port);
	void connecting(int socket_fd_);
	void resetChannel();

	void handleWrite();

	void set_connection_callback(std::function<void(int)>const& cb);

private:
	EventLoop* loop_;
	int socket_fd_;
	const char* ip_;
	const int port_;
	std::unique_ptr<Channel> channel_;
	std::function<void(int)> connection_callback;
};

#endif //CONNECTOR_H