#include <event.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include <functional>
#include <iostream>
#include <algorithm>

#include "protocol.h"

class Channel
{
public:
    using RecvCallback = std::function<void(Channel&, std::vector<uint8_t>&)>;

    static void timer_callback(int fd, short event, void* channel)
    {
        Channel& self = *reinterpret_cast<Channel*>(channel);
        self._schedule_timer();
        self._schedule_sender();
    }

    static void read_callback(int fd, short event, void* channel)
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

    static void send_callback(int fd, short event, void* channel)
    {
        Channel& self = *reinterpret_cast<Channel*>(channel);

        auto message = self._protocol.pop_send_ready();
        if (!message) {
            event_del(&self._sender_event);
            return;
        }

        write(fd, message.value().data(), message.value().size());
    }

    Channel(int fd, RecvCallback callback) : _fd(fd), _connected(false), _callback(callback)
    {
        evtimer_set(&_timer_event, &timer_callback, this);
        event_set(&_reader_event, fd, EV_READ | EV_PERSIST, read_callback, this);
        event_set(&_sender_event, fd, EV_WRITE | EV_PERSIST, send_callback, this);
        _schedule_reader();
    }

    ~Channel()
    {
        event_del(&_timer_event);
        event_del(&_reader_event);
    }

    void send(const std::vector<uint8_t>& vec)
    {
        const std::span data{vec.data(), vec.size()};
        _protocol.send(data);
        // TODO might cuase double addition?
        _schedule_timer();
        _schedule_sender();
    }

    std::vector<uint8_t> read(std::size_t max_size)
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

private:
    void _schedule_timer()
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

    void _schedule_reader() { event_add(&_reader_event, nullptr); }

    void _schedule_sender()
    {
        if (_protocol.send_ready_count() != 0) {
            event_add(&_sender_event, nullptr);
        }
    }

    int _fd;
    bool _connected;
    RecvCallback _callback;
    struct event _timer_event;
    struct event _reader_event;
    struct event _sender_event;
    ReliableProtocol<std::chrono::steady_clock> _protocol;
};

int get_sock(uint16_t port)
{
    int sock = -1;
    struct event timer_event, udp_event;
    struct sockaddr_in sin;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&sin, sizeof(sin))) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    return sock;
}

struct VecPreview
{
    const std::vector<uint8_t>& vec;
};

std::ostream& operator<<(std::ostream& stream, const VecPreview& preview)
{
    std::string_view partial(reinterpret_cast<const char*>(preview.vec.data()), std::min(preview.vec.size(), std::size_t(10)));

    stream << '"' << partial << '"';
    if (preview.vec.size() > partial.size())
    {
        std::cout << "... (truncated)";
    }
    return stream;
}

int main()
{
    int sock = get_sock(1337);

    event_init();

    Channel c(sock, [](Channel& channel, std::vector<uint8_t>& data) {
        std::cout << "got " << data.size() << " bytes: " << VecPreview{data} << std::endl;
        channel.send(data);
    });

    event_dispatch();
    return 0;
}
