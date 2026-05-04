#pragma once

#include<string>
#include<utility>
#include <map>



class HttpResponse{
    public:
        enum HttpStatusCode
        {
            kUnkonwn = 1,
            k100Continue = 100,
            k200K = 200,
            k400BadRequest = 400,
            k401UnAuth = 401,
            k403Forbidden = 403,
            k404NotFound = 404,
            k500internalServerError = 500
        };
        HttpResponse();
        HttpResponse(bool close_connection);
        ~HttpResponse();

        void SetProtocol(const std::string &protocol);
        const std::string &protocol() const;

        void SetVersion(const std::string &version);
        const std::string &version() const;

        void SetStatusCode(HttpStatusCode status_code); // 设置回应码
        int statusCode() const;
        void SetStatusMessage(const std::string &status_message);
        const std::string &statusMessage() const;
        void SetCloseConnection(bool close_connection);

        void SetContentType(const std::string &content_type); 
        void AddHeader(const std::string &key, const std::string &value); // 设置回应头
        std::string GetHeader(const std::string &key) const;

        void SetBody(const std::string &body);
        const std::string &body() const;

        std::string message(); // 将信息加入到buffer中。

        bool IsCloseConnection();

    private:
        // static const std::string server_name_;

        std::string protocol_;
        std::string version_;

        std::map<std::string, std::string> headers_;

        HttpStatusCode status_code_;
        std::string status_message_;
        std::string body_;
        bool close_connection_;
};