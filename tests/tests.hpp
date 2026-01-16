#pragma once

#include <stdint.h>
#include <ostream>
#include <sstream>

// Simple RAII silencer to mute noisy test logs.
class StreamSilencer {
public:
    explicit StreamSilencer(std::ostream& stream)
        : mStream(stream), mOldBuffer(stream.rdbuf(mNullStream.rdbuf())) {}

    ~StreamSilencer() {
        mStream.rdbuf(mOldBuffer);
    }

    StreamSilencer(const StreamSilencer&) = delete;
    StreamSilencer& operator=(const StreamSilencer&) = delete;

private:
    std::ostream& mStream;
    std::ostringstream mNullStream;
    std::streambuf* mOldBuffer;
};

uint32_t getMicros();
uint32_t getMillis();
void waitFor_ms(uint32_t inWait_ms);
