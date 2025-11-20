#pragma once
#include "../core/noncopyable.h"
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#ifdef __linux__
#include <sys/epoll.h>
#endif

namespace eventcore {
    namespace net {

        class Poller : public NonCopyable {
            public:
                enum Events { kNone = 0, kReadable = 1, kWritable = 2, kError = 4 };
                using EventCallback = std::function<void(int fd, int events)>;

                virtual ~Poller() = default;
                virtual bool add(int fd, int events, EventCallback cb) = 0;
                virtual bool modify(int fd, int events) = 0;
                virtual bool remove(int fd) = 0;
                virtual int poll(int timeout_ms) = 0;
                static std::unique_ptr<Poller> create();
        };

#ifdef __linux__
        class EpollPoller : public Poller {
            public:
                EpollPoller();
                ~EpollPoller();
                bool add(int fd, int events, EventCallback cb) override;
                bool modify(int fd, int events) override;
                bool remove(int fd) override;
                int poll(int timeout_ms) override;

            private:
                int epfd_;
                std::vector<struct epoll_event> events_;
                std::unordered_map<int, EventCallback> callbacks_;
        };
#endif

        class SelectPoller : public Poller {
            public:
                SelectPoller();
                ~SelectPoller();
                bool add(int fd, int events, EventCallback cb) override;
                bool modify(int fd, int events) override;
                bool remove(int fd) override;
                int poll(int timeout_ms) override;

            private:
                struct FdInfo { int events; EventCallback callback; };
                std::unordered_map<int, FdInfo> fds_;
                int max_fd_ = -1;
        };

    } // namespace net
} // namespace eventcore
