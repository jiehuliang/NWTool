#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include "Util/common.h"
#include "Connector.h"

#include <functional>


class EventLoop;
class TcpClient {
public:
	DISALLOW_COPY_AND_MOVE(TcpClient)
	TcpClient(EventLoop* loop,const char* ip,const int port);
	~TcpClient();

	void Start();

	void Write(const std::string& message);

	void Close();

	void newConnection(int socket_fd);

	void HandleClose(const std::shared_ptr<TcpConnection>& conn);

	void set_connectio_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const &fn);
	void set_message_callback(std::function<void(const std::shared_ptr<TcpConnection> &)> const& fn);

private:
	EventLoop* loop_;
	std::unique_ptr<Connector> Connector_;
	std::shared_ptr<TcpConnection> conn_;

	std::function<void(const std::shared_ptr<TcpConnection>&)> conn_callback;
	std::function<void(const std::shared_ptr<TcpConnection>&)> message_callback;
};

#endif //TCP_CLIENT_H