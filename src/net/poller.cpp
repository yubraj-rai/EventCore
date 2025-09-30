#include "eventcore/net/poller.h"
#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/select.h>
#endif
#include <unistd.h>
#include <cerrno>
#include <stdexcept>

namespace eventcore {
    namespace net {

        std::unique_ptr<Poller> Poller::create() {
#ifdef __linux__
            return std::make_unique<EpollPoller>();
#else
            return std::make_unique<SelectPoller>();
#endif
        }

#ifdef __linux__

        EpollPoller::EpollPoller() : epfd_(epoll_create1(EPOLL_CLOEXEC)), events_(16) {
            if (epfd_ < 0) throw std::runtime_error("epoll_create1 failed");
        }

        EpollPoller::~EpollPoller() { 
            if (epfd_ >= 0) {
                ::close(epfd_);
            }
        }

        bool EpollPoller::add(int fd, int events, EventCallback cb) {
            struct epoll_event ev;
            ev.data.fd = fd;
            ev.events = 0;

            if (events & kReadable) ev.events |= EPOLLIN;
            if (events & kWritable) ev.events |= EPOLLOUT;
            ev.events |= EPOLLET | EPOLLONESHOT;

            if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
                return false;
            }

            callbacks_[fd] = cb;
            return true;
        }

        bool EpollPoller::modify(int fd, int events) {
            struct epoll_event ev;
            ev.data.fd = fd;
            ev.events = 0;

            if (events & kReadable) ev.events |= EPOLLIN;
            if (events & kWritable) ev.events |= EPOLLOUT;
            ev.events |= EPOLLET;

            return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == 0;
        }

        bool EpollPoller::remove(int fd) {
            if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
                return false;
            }
            callbacks_.erase(fd);
            return true;
        }

        int EpollPoller::poll(int timeout_ms) {
            int numEvents = epoll_wait(epfd_, events_.data(), static_cast<int>(events_.size()), timeout_ms);

            if (numEvents > 0) {
                if (static_cast<size_t>(numEvents) == events_.size()) {
                    events_.resize(events_.size() * 2);
                }

                for (int i = 0; i < numEvents; ++i) {
                    int fd = events_[i].data.fd;
                    int revents = 0;

                    if (events_[i].events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
                        revents |= kReadable;
                    }
                    if (events_[i].events & EPOLLOUT) {
                        revents |= kWritable;
                    }
                    if (events_[i].events & (EPOLLERR | EPOLLHUP)) {
                        revents |= kError;
                    }

                    auto it = callbacks_.find(fd);
                    if (it != callbacks_.end()) {
                        it->second(fd, revents);
                    }
                }
            }

            return numEvents;
        }

#endif // __linux__

        SelectPoller::SelectPoller() : max_fd_(-1) {}

        SelectPoller::~SelectPoller() = default;

        bool SelectPoller::add(int fd, int events, EventCallback cb) {
            FdInfo info;
            info.events = events;
            info.callback = cb;
            fds_[fd] = info;

            if (fd > max_fd_) {
                max_fd_ = fd;
            }

            return true;
        }

        bool SelectPoller::modify(int fd, int events) {
            auto it = fds_.find(fd);
            if (it != fds_.end()) {
                it->second.events = events;
                return true;
            }
            return false;
        }

        bool SelectPoller::remove(int fd) {
            fds_.erase(fd);

            if (fd == max_fd_) {
                max_fd_ = -1;
                for (const auto& pair : fds_) {
                    if (pair.first > max_fd_) {
                        max_fd_ = pair.first;
                    }
                }
            }

            return true;
        }

        int SelectPoller::poll(int timeout_ms) {
            fd_set readfds, writefds, exceptfds;
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_ZERO(&exceptfds);

            for (const auto& pair : fds_) {
                int fd = pair.first;
                int events = pair.second.events;

                if (events & kReadable) FD_SET(fd, &readfds);
                if (events & kWritable) FD_SET(fd, &writefds);
                FD_SET(fd, &exceptfds);
            }

            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;

            int ret = ::select(max_fd_ + 1, &readfds, &writefds, &exceptfds, &tv);

            if (ret > 0) {
                for (const auto& pair : fds_) {
                    int fd = pair.first;
                    int revents = 0;

                    if (FD_ISSET(fd, &readfds)) revents |= kReadable;
                    if (FD_ISSET(fd, &writefds)) revents |= kWritable;
                    if (FD_ISSET(fd, &exceptfds)) revents |= kError;

                    if (revents) {
                        pair.second.callback(fd, revents);
                    }
                }
            }

            return ret;
        }

    } // namespace net
} // namespace eventcore
