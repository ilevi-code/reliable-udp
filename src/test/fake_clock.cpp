#include "test/fake_clock.h"

FakeClock::time_point FakeClock::now() {
    return timestamp;
}

void FakeClock::reset()
{
    timestamp = time_point{FakeClock::duration::zero()};
}

void FakeClock::advance(duration diff)
{
    timestamp += diff;
}

FakeClock::time_point FakeClock::timestamp;


