#include <iostream>
#include "Http/HttpServer.h"
#include "Http/HttpRequest.h"
#include "Http/HttpResponse.h"
#include "Event/EventLoop.h"
#include "HooLog/HooLog.h"
#include <string>
#include <memory>

const std::string html = " <font color=\"red\">This is html!</font> ";
void HttpResponseCallback(const HttpRequest &request, HttpResponse *response)
{
    if (request.method() != "GET") {
        response->SetStatusCode(HttpResponse::HttpStatusCode::k400BadRequest);
        response->SetStatusMessage("Bad Request");
        response->SetCloseConnection(true);
    }

    {
        std::string url = request.url();
        if(url == "/"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody(html);
            response->SetContentType("text/html");
        }else if(url == "/hello"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
            response->SetBody("hello world\n");
            response->SetContentType("text/plain");
        }else if(url == "/favicon.ico"){
            response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
        }else{
            response->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
            response->SetStatusMessage("Not Found");
            response->SetBody("Sorry Not Found\n");
            response->SetCloseConnection(true);
        }
    }
    return;
}

std::unique_ptr<AsyncLogger> asynclog;
void AsyncOutputFunc(const char *data, int len)
{
    asynclog->AppendNonCache(data, len);
}

void AsyncFlushFunc() {
    asynclog->Flush();
}

int main(int argc, char *argv[]){
    int port;
    if (argc <= 1)
    {
        port = 1238;
    }else if (argc == 2){
        port = atoi(argv[1]);
    }else{
        printf("error");
        exit(0);
    }
    // �����׶���ʱ�������첽��־
    setLogLevel(loglevel::DEBUG);

    std::shared_ptr<AsyncLogger> asyncLogger = std::make_shared<AsyncLogger>();
    setOutputFunc(std::bind(&AsyncLogger::AppendNonCache, asyncLogger, std::placeholders::_1, std::placeholders::_2));
    setFlushFunc(std::bind(&AsyncLogger::Flush, asyncLogger));
    asyncLogger->Start();

    int size = std::thread::hardware_concurrency() - 1;
    EventLoop *loop = new EventLoop();
    HttpServer *server = new HttpServer(loop, "127.0.0.1", port, true);
    server->SetHttpCallback(HttpResponseCallback);
    server->SetThreadNums(size);
    server->start();
    
    delete loop;
    delete server;
    return 0;
}