#include "HttpResponseContext.h"
#include <memory>
#include <string>
#include <cstring>
#include <cstdlib>

HttpResponseContext::HttpResponseContext() : state_(HttpResponseParseState::RES_START) {
    response_ = std::unique_ptr<HttpResponse>(new HttpResponse());
}

HttpResponseContext::~HttpResponseContext() {};

bool HttpResponseContext::GetCompleteResponse() {
    return state_ == HttpResponseParseState::RES_COMPLETE;
}

void HttpResponseContext::ResetContextStatus() {
    state_ = HttpResponseParseState::RES_START;
}

bool HttpResponseContext::Parse(const std::string &msg) {
    return Parse(msg.data(), static_cast<int>(msg.size()));
}

bool HttpResponseContext::Parse(const char *begin, int size) {
    char *start = const_cast<char *>(begin);
    char *end = start;
    char *colon = end;
    int status_code = 0;

    while (state_ != HttpResponseParseState::kRES_INVALID
        && state_ != HttpResponseParseState::RES_COMPLETE
        && end - begin <= size) {

        char ch = *end;

        switch (state_) {
            case HttpResponseParseState::RES_START: {
                if (ch == CR || ch == LF || isblank(ch)) {
                } else if (isupper(ch)) {
                    state_ = HttpResponseParseState::RES_PROTOCOL;
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }
            case HttpResponseParseState::RES_PROTOCOL: {
                if (ch == '/') {
                    response_->SetProtocol(std::string(start, end));
                    start = end + 1;
                    state_ = HttpResponseParseState::RES_BEFORE_VERSION;
                } else if (isupper(ch)) {
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }
            case HttpResponseParseState::RES_BEFORE_VERSION: {
                if (isdigit(ch)) {
                    state_ = HttpResponseParseState::RES_VERSION;
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }
            case HttpResponseParseState::RES_VERSION: {
                if (isblank(ch)) {
                    response_->SetVersion(std::string(start, end));
                    start = end + 1;
                    state_ = HttpResponseParseState::RES_BEFORE_STATUS_CODE;
                } else if (isdigit(ch) || ch == '.') {
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }
            case HttpResponseParseState::RES_BEFORE_STATUS_CODE: {
                if (isblank(ch)) {
                } else if (isdigit(ch)) {
                    state_ = HttpResponseParseState::RES_STATUS_CODE;
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }
            case HttpResponseParseState::RES_STATUS_CODE: {
                if (isblank(ch)) {
                    status_code = atoi(std::string(start, end).c_str());
                    response_->SetStatusCode(
                        static_cast<HttpResponse::HttpStatusCode>(status_code));
                    start = end + 1;
                    state_ = HttpResponseParseState::RES_BEFORE_REASON;
                } else if (isdigit(ch)) {
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }
            case HttpResponseParseState::RES_BEFORE_REASON: {
                if (isblank(ch)) {
                } else {
                    state_ = HttpResponseParseState::RES_REASON;
                }
                break;
            }
            case HttpResponseParseState::RES_REASON: {
                if (ch == CR) {
                    response_->SetStatusMessage(std::string(start, end));
                    start = end + 1;
                    state_ = HttpResponseParseState::RES_WHEN_CR;
                }
                break;
            }

            case HttpResponseParseState::RES_HEADER_KEY: {
                if (ch == ':') {
                    colon = end;
                    state_ = HttpResponseParseState::RES_HEADER_VALUE;
                }
                break;
            }
            case HttpResponseParseState::RES_HEADER_VALUE: {
                if (isblank(ch)) {
                } else if (ch == CR) {
                    response_->AddHeader(std::string(start, colon), std::string(colon + 2, end));
                    start = end + 1;
                    state_ = HttpResponseParseState::RES_WHEN_CR;
                }
                break;
            }

            case HttpResponseParseState::RES_WHEN_CR: {
                if (ch == LF) {
                    start = end + 1;
                    state_ = HttpResponseParseState::RES_CR_LF;
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }
            case HttpResponseParseState::RES_CR_LF: {
                if (ch == CR) {
                    state_ = HttpResponseParseState::RES_CR_LF_CR;
                } else if (isblank(ch)) {
                    state_ = HttpResponseParseState::kRES_INVALID;
                } else {
                    state_ = HttpResponseParseState::RES_HEADER_KEY;
                }
                break;
            }
            case HttpResponseParseState::RES_CR_LF_CR: {
                if (ch == LF) {
                    if (end - begin < size) {
                        state_ = HttpResponseParseState::RES_BODY;
                    } else {
                        state_ = HttpResponseParseState::RES_COMPLETE;
                    }
                    start = end + 1;
                } else {
                    state_ = HttpResponseParseState::kRES_INVALID;
                }
                break;
            }

            case HttpResponseParseState::RES_BODY: {
                int bodylength = size - (end - begin);
                response_->SetBody(std::string(start, start + bodylength));
                state_ = HttpResponseParseState::RES_COMPLETE;
                break;
            }

            default:
                state_ = HttpResponseParseState::kRES_INVALID;
                break;
        }

        end++;
    }

    return state_ == HttpResponseParseState::RES_COMPLETE;
}

HttpResponse *HttpResponseContext::response() {
    return response_.get();
}
