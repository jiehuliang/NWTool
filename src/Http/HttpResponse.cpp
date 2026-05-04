#include "HttpResponse.h"
#include <string>

HttpResponse::HttpResponse() :
    status_code_(HttpStatusCode::kUnkonwn), close_connection_(false){};

HttpResponse::HttpResponse(bool close_connection) :
    status_code_(HttpStatusCode::kUnkonwn), close_connection_(close_connection){};

HttpResponse::~HttpResponse(){};

void HttpResponse::SetProtocol(const std::string &protocol){
    protocol_ = protocol;
}

const std::string &HttpResponse::protocol() const{
    return protocol_;
}

void HttpResponse::SetVersion(const std::string &version){
    version_ = version;
}

const std::string &HttpResponse::version() const{
    return version_;
}

void HttpResponse::SetStatusCode(HttpStatusCode status_code){
    status_code_ = status_code;
}

void HttpResponse::SetStatusMessage(const std::string& status_message){
    status_message_ = std::move(status_message);
}

int HttpResponse::statusCode() const{
    return static_cast<int>(status_code_);
}

const std::string &HttpResponse::statusMessage() const{
    return status_message_;
}

void HttpResponse::SetCloseConnection(bool close_connection){
    close_connection_ = close_connection;
}

void HttpResponse::SetContentType(const std::string &content_type){
    AddHeader("Content-Type", content_type);
}

void HttpResponse::AddHeader(const std::string &key, const std::string &value){
    headers_[key] = value;
}

std::string HttpResponse::GetHeader(const std::string &key) const{
    auto it = headers_.find(key);
    return it == headers_.end() ? std::string() : it->second;
}

void HttpResponse::SetBody(const std::string &body){
    body_ = std::move(body);
}

const std::string &HttpResponse::body() const{
    return body_;
}

bool HttpResponse::IsCloseConnection(){
    return close_connection_;
}

std::string HttpResponse::message(){
    std::string message;
    message += ("HTTP/1.1 " +
                std::to_string(status_code_) + " " +
                status_message_ + "\r\n"
    );
    if(close_connection_){
        message += ("Connection: close\r\n");
    }else{
        message += ("Content-Length: " + std::to_string(body_.size()) + "\r\n");
        message += ("Connection: Keep-Alive\r\n");
    }

    for (const auto&header : headers_){
        message += (header.first + ": " + header.second + "\r\n");
    }

    message += "\r\n";
    message += body_;

    return message;
}
