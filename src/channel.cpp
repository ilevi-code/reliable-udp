#include <unistd.h>

#include "channel.h"

Channel::Channel(int fd, RecvCallback callback) : _fd(fd), _connected(false), _callback(callback)
{
    evtimer_set(&_timer_event, &timer_callback, this);
    event_set(&_reader_event, fd, EV_READ | EV_PERSIST, read_callback, this);
    event_set(&_sender_event, fd, EV_WRITE | EV_PERSIST, send_callback, this);
    _schedule_reader();
}

void Channel::timer_callback(int fd, short event, void* channel)
{
    Channel& self = *reinterpret_cast<Channel*>(channel);
    self._schedule_timer();
    self._schedule_sender();
}

void Channel::read_callback(int fd, short event, void* channel)
{
    Channel& self = *reinterpret_cast<Channel*>(channel);

    auto buffer = self.read(UINT16_MAX);

    self._protocol.feed(std::move(buffer));
    while (true) {
        auto chunk = self._protocol.read_chunk();
        if (!chunk) {
            break;
        }
        self._callback(self, chunk.value());
    }

    self._schedule_sender();
}

void Channel::send_callback(int fd, short event, void* channel)
{
    Channel& self = *reinterpret_cast<Channel*>(channel);

    auto message = self._protocol.pop_send_ready();
    if (!message) {
        event_del(&self._sender_event);
        return;
    }

    // TODO check return value
    write(fd, message.value().data(), message.value().size());
}

Channel::~Channel()
{
    event_del(&_timer_event);
    event_del(&_reader_event);
    event_del(&_sender_event);
}

void Channel::send(const std::vector<uint8_t>& vec)
{
    const std::span data{vec.data(), vec.size()};
    _protocol.send(data);
    _schedule_timer();
    _schedule_sender();
}

std::vector<uint8_t> Channel::read(std::size_t max_size)
{
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    ssize_t nread = -1;
    std::vector<uint8_t> buffer(UINT16_MAX);

    if (!_connected) {
        nread = ::recvfrom(_fd, buffer.data(), buffer.size(), 0, &addr, &addrlen);
        connect(_fd, &addr, addrlen);
        _connected = true;
    } else {
        nread = ::read(_fd, buffer.data(), buffer.size());
    }
    // todo handle errors better
    if (nread == -1) {
        nread = 0;
    }
    buffer.resize(nread);
    return buffer;
}

void Channel::_schedule_timer()
{
    struct timeval delay;

    auto requested_delay_opt = _protocol.update();
    if (!requested_delay_opt) {
        return;
    }

    auto requested_delay = requested_delay_opt.value();
    delay.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(requested_delay).count();
    delay.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(requested_delay).count();
    evtimer_add(&_timer_event, &delay);
}

void Channel::_schedule_reader()
{
    event_add(&_reader_event, nullptr);
}

void Channel::_schedule_sender()
{
    if (_protocol.send_ready_count() != 0) {
        event_add(&_sender_event, nullptr);
    }
}
