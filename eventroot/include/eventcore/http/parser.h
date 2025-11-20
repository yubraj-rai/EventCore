#pragma once
#include "request.h"
#include "../net/buffer.h"

namespace eventcore {
    namespace http {

        class Parser {
            public:
                enum State { kExpectRequestLine, kExpectHeaders, kExpectBody, kComplete };
                Parser();
                bool parse_request(net::Buffer* buffer, Request* request);
                bool is_complete() const { return state_ == kComplete; }
                void reset();

            private:
                bool parse_request_line(const char* begin, const char* end);
                bool parse_headers(net::Buffer* buffer);
                bool parse_body(net::Buffer* buffer);

                State state_;
                Request* request_;
                size_t content_length_;
        };

    } // namespace http
} // namespace eventcore
