#include <gtest/gtest.h>

#include "protocol.h"
#include "test/utils.h"
#include "test/fake_clock.h"

using namespace std::chrono_literals;

class ReliableProtocolTest : public testing::Test
{
protected:
    ReliableProtocolTest() : proto(1s) {}
    void SetUp() override
    {
        FakeClock::reset();
    }

    ReliableProtocol<FakeClock> proto;
};


TEST_F(ReliableProtocolTest, EmptyIsUnreadable)
{
    ASSERT_FALSE(proto.is_readable());
}

TEST_F(ReliableProtocolTest, EmptyHasNothingToSend)
{
    ASSERT_FALSE(proto.send_ready_count());
}

TEST_F(ReliableProtocolTest, SendEnqueueMessage)
{
    std::vector<uint8_t> message{1,2,3,4};
    proto.send(message);

    ASSERT_EQ(proto.waiting_send_count(), 1);
    ASSERT_EQ(proto.send_ready_count(), 1);
}

TEST_F(ReliableProtocolTest, SendEnqueuedMessageValue)
{
    proto.send(to_span("abcd"));

    auto expected = std::optional<std::vector<uint8_t>>{{0,0,0,0,'a','b','c','d'}};
    ASSERT_EQ(proto.pop_send_ready(), expected);
}

TEST_F(ReliableProtocolTest, EmptyAfterSend)
{
    proto.send(to_span("abcd"));
    proto.pop_send_ready();

    ASSERT_EQ(proto.pop_send_ready(), std::optional<std::vector<uint8_t>>{});
}

TEST_F(ReliableProtocolTest, SecondSend)
{
    proto.send(to_span("abcd"));
    proto.pop_send_ready();
    proto.feed(std::vector<uint8_t>{0,0,0,0});

    proto.send(to_span("hello"));

    auto expected = std::optional<std::vector<uint8_t>>{{0,0,0,1,'h','e','l','l','o'}};
    ASSERT_EQ(proto.pop_send_ready(), expected);
}

TEST_F(ReliableProtocolTest, UpdateCausesRetransmission)
{
    proto.send(to_span("abcd"));
    auto message = proto.pop_send_ready();
    ASSERT_TRUE(message.has_value());

    FakeClock::advance(2s);
    proto.update();

    ASSERT_EQ(proto.send_ready_count(), 1);
    ASSERT_EQ(proto.pop_send_ready(), message);
}
