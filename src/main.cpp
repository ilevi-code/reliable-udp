#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include <iostream>
#include <algorithm>

#include "channel.h"

int get_sock(uint16_t port)
{
    int sock = -1;
    struct event timer_event, udp_event;
    struct sockaddr_in sin;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

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
