#pragma once
#include <string>
#include <memory>
#include "HttpResponse.h"

#define CR '\r'
#define LF '\n'

enum HttpResponseParseState {
    kRES_INVALID,

    RES_START,
    RES_PROTOCOL,
    RES_BEFORE_VERSION,
    RES_VERSION,
    RES_BEFORE_STATUS_CODE,
    RES_STATUS_CODE,
    RES_BEFORE_REASON,
    RES_REASON,

    RES_HEADER_KEY,
    RES_HEADER_VALUE,

    RES_WHEN_CR,
    RES_CR_LF,
    RES_CR_LF_CR,

    RES_BODY,
    RES_COMPLETE,
};

class HttpResponseContext {
public:
    HttpResponseContext();
    ~HttpResponseContext();

    bool Parse(const char *begin, int size);
    bool Parse(const std::string &msg);
    bool GetCompleteResponse();
    HttpResponse *response();
    void ResetContextStatus();

private:
    std::unique_ptr<HttpResponse> response_;
    HttpResponseParseState state_;
};
