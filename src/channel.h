#pragma once

#include <functional>
#include <event.h>

#include "protocol.h"

class Channel
{
public:
    using RecvCallback = std::function<void(Channel&, std::vector<uint8_t>&)>;

    Channel(int fd, RecvCallback callback);
    ~Channel();

    void send(const std::vector<uint8_t>& vec);

    std::vector<uint8_t> read(std::size_t max_size);

private:
    static void timer_callback(int fd, short event, void* channel);
    static void read_callback(int fd, short event, void* channel);
    static void send_callback(int fd, short event, void* channel);

    void _schedule_timer();
    void _schedule_reader();
    void _schedule_sender();

    int _fd;
    bool _connected;
    RecvCallback _callback;
    struct event _timer_event;
    struct event _reader_event;
    struct event _sender_event;
    ReliableProtocol<std::chrono::steady_clock> _protocol;
};
