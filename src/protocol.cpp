#include <arpa/inet.h>

#include "protocol.h"

BaseProtocol::BaseProtocol() : _tx{{}, {}, 0}, _rx{{}, {}, 0}
{
}

std::vector<uint8_t> u32_to_vector(uint32_t num)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&num);
    return std::vector<uint8_t>(ptr, ptr + sizeof(num));
}

uint32_t vector_to_u32(const std::vector<uint8_t>& data)
{
    return *reinterpret_cast<const uint32_t*>(data.data());
}

size_t BaseProtocol::send_ready_count() const
{
    return _tx.data_queue.size();
}

size_t BaseProtocol::waiting_send_count() const
{
    return _tx.messages.size();
}

std::optional<std::vector<uint8_t>> BaseProtocol::pop_send_ready()
{
    if (_tx.data_queue.size() > 0) {
        auto message = std::move(_tx.data_queue.front());
        _tx.data_queue.pop_front();
        return message;
    }
    return {};
}

std::optional<std::vector<uint8_t>> BaseProtocol::read_chunk()
{
    if (_rx.data_queue.size() > 0) {
        auto message = std::move(_rx.data_queue.front());
        _rx.data_queue.pop_front();
        return message;
    }
    return {};
}

bool BaseProtocol::is_readable() const
{
    return _rx.data_queue.size() > 0;
}

void BaseProtocol::feed(std::vector<uint8_t>&& data)
{
    if (data.size() < 4) {
        // junk?
        return;
    }

    uint32_t id = htonl(*reinterpret_cast<uint32_t*>(data.data()));
    if (data.size() == 4) {
        handle_ack(id);
    } else {
        handle_message(id, {data.data() + sizeof(id), data.size() - sizeof(id)});
    }
}

void BaseProtocol::handle_ack(uint32_t id)
{
    if (id != _tx.messages.begin()->first) {
        return;
    }
    _tx.messages.erase(id);
    queue_current_message_for_send();
}

void BaseProtocol::handle_message(uint32_t id, std::span<uint8_t> data)
{
    std::vector<uint8_t> _data{data.begin(), data.end()};
    _rx.messages.emplace(id, _data);

    id = htonl(id);
    uint8_t* id_ptr = reinterpret_cast<uint8_t*>(&id);
    _tx.data_queue.push_back(std::vector<uint8_t>(id_ptr, id_ptr + sizeof(id)));

    update_read_ready();
}

void BaseProtocol::update_read_ready()
{
    while (true) {
        auto it = _rx.messages.find(_rx.next_id);
        if (it == _rx.messages.end()) {
            break;
        }

        auto data = std::move(it->second);
        _rx.messages.erase(it);
        _rx.data_queue.push_back(std::move(data));

        _rx.next_id++;
    }
}

void BaseProtocol::send(std::span<const uint8_t> data)
{
    _tx.messages.emplace(_tx.next_id, std::vector<uint8_t>(data.begin(), data.end()));
    _tx.next_id++;
    if (_tx.messages.size() == 1) {
        queue_current_message_for_send();
    }
}

void BaseProtocol::queue_current_message_for_send()
{
    if (_tx.messages.empty()) {
        return;
    }

    auto id = _tx.messages.begin()->first;
    std::vector<uint8_t> sendable = u32_to_vector(htonl(id));

    // append message data to ack
    auto& data = _tx.messages.begin()->second;
    sendable.reserve(sendable.size() + distance(data.begin(),data.end()));
    sendable.insert(sendable.end(),data.begin(),data.end());

    _tx.data_queue.push_back(std::move(sendable));
}

void BaseProtocol::do_update()
{
    queue_current_message_for_send();
}
