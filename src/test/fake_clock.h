#include <chrono>

class FakeClock
{
public:
    using duration = typename std::chrono::duration<int>;
    using time_point = typename std::chrono::time_point<FakeClock>;

    static time_point now();
    static void reset();
    static void advance(duration diff);

    static time_point timestamp;
};

