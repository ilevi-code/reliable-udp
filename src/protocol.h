#pragma once

#include <stdint.h>
#include <vector>
#include <deque>
#include <optional>
#include <map>
#include <span>
#include <chrono>

/**
 * Simple state machine, for managing retransmission and ack system
 */
class BaseProtocol
{
public:
    BaseProtocol();

    size_t send_ready_count() const;
    size_t waiting_send_count() const;
    /**
     * Get a message that is ready to be sent
     */
    std::optional<std::vector<uint8_t>> pop_send_ready();

    std::optional<std::vector<uint8_t>> read_chunk();
    bool is_readable() const;

    /**
     * Update state machine with incoming network data
     */
    void feed(std::vector<uint8_t>&& data);

    /**
     * Queue data to be sent
     */
    void send(std::span<const uint8_t> data);

protected:
    void do_update();

private:
    void handle_ack(uint32_t id);
    void handle_message(uint32_t id, std::span<uint8_t> data);
    void update_read_ready();

    void queue_current_message_for_send();

    struct MessageContext {
        std::deque<std::vector<uint8_t>> data_queue;
        std::map<uint32_t, std::vector<uint8_t>> messages;
        uint32_t next_id;
    };
    // data_queue -> messages to sent, prefixed with id
    MessageContext _tx;
    // data_queue -> messages ready to be read
    MessageContext _rx;
};

template<typename Clock = std::chrono::steady_clock>
class ReliableProtocol : public BaseProtocol
{
public:
    using Duration = typename Clock::duration;
    using TimePoint = typename Clock::time_point;

    ReliableProtocol() : BaseProtocol() {}

    /**
     * Notify that some time has passed.
     * @return the duration until next expected call to update().
     * @note If last requested duration hasn't passed, nothing is done.
     *       An updated duration will returned.
     */
    Duration update()
    {
        using namespace std::chrono_literals;
        static constexpr auto INTERVAL = 1s;
        auto now = Clock::now();

        if (_next_update < now)
        {
            do_update();
            _next_update = now + INTERVAL;
            return INTERVAL;
        }
        return _next_update - now;

    }

private:
    TimePoint _next_update;
};
