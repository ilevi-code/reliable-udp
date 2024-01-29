#include <gtest/gtest.h>

#include "protocol.h"
#include "test/utils.h"
#include "test/fake_clock.h"

using namespace std::chrono_literals;

class ReliableConversationTest : public testing::Test
{
protected:
    void SetUp() override
    {
        FakeClock::reset();
    }

    void pass_message_and_ack()
    {
        receiver.feed(sender.pop_send_ready().value());
        sender.feed(receiver.pop_send_ready().value());
    }

    ReliableProtocol<FakeClock> sender;
    ReliableProtocol<FakeClock> receiver;
};

TEST_F(ReliableConversationTest, SingleMessage)
{
    sender.send(to_span("abcd"));
    // pass the message
    receiver.feed(sender.pop_send_ready().value());
    // pass the ack
    sender.feed(receiver.pop_send_ready().value());

    ASSERT_EQ(to_string(receiver.read_chunk().value()), "abcd");
}

TEST_F(ReliableConversationTest, MultipleMessages)
{
    const char* messages[] = {"hello", "world", "zinc is a simple-stupid protocol"};

    for (auto message: messages) {
        sender.send(to_span(message));
    }

    for (auto _ : messages) {
        pass_message_and_ack();
    }

    for (auto message: messages) {
        auto received = receiver.read_chunk();
        ASSERT_EQ(to_string(received.value()), message);
    }

    ASSERT_EQ(sender.send_ready_count(), 0);
    ASSERT_EQ(receiver.send_ready_count(), 0);
}

TEST_F(ReliableConversationTest, IdleAfterSendAndAck)
{
    sender.send(to_span("abcd"));
    pass_message_and_ack();
    receiver.read_chunk();

    FakeClock::advance(2s);

    ASSERT_EQ(sender.send_ready_count(), 0);
    ASSERT_EQ(receiver.send_ready_count(), 0);
}

TEST_F(ReliableConversationTest, WithDrop)
{
    sender.send(to_span("abcd"));

    // drop the message
    sender.pop_send_ready();

    FakeClock::advance(2s);
    sender.update();
    pass_message_and_ack();

    ASSERT_EQ(to_string(receiver.read_chunk().value()), "abcd");

    sender.update();
    receiver.update();

    ASSERT_EQ(sender.send_ready_count(), 0);
    ASSERT_EQ(receiver.send_ready_count(), 0);
}

TEST_F(ReliableConversationTest, IdleAfterDropAndPass)
{
    sender.send(to_span("abcd"));

    // drop the message
    sender.pop_send_ready();

    FakeClock::advance(2s);
    sender.update();
    pass_message_and_ack();
    receiver.read_chunk().value();

    FakeClock::advance(2s);
    sender.update();
    receiver.update();

    ASSERT_EQ(sender.send_ready_count(), 0);
    ASSERT_EQ(receiver.send_ready_count(), 0);
}
