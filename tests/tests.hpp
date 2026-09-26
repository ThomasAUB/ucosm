#pragma once

#include <stdint.h>
#include <ostream>
#include <streambuf>

// Discards everything written to it. It holds no state, so several
// scheduler threads may write through it concurrently. A std::ostringstream
// could not: its buffer is reallocated as it grows, and two threads growing
// it at once free and read the same block (heap-use-after-free).
class NullBuffer : public std::streambuf {
protected:
    int_type overflow(int_type c) override {
        return traits_type::not_eof(c);
    }
    std::streamsize xsputn(const char*, std::streamsize inCount) override {
        return inCount;
    }
};

// Simple RAII silencer to mute noisy test logs.
class StreamSilencer {
public:
    explicit StreamSilencer(std::ostream& stream)
        : mStream(stream), mOldBuffer(stream.rdbuf(&mNullBuffer)) {}

    ~StreamSilencer() {
        mStream.rdbuf(mOldBuffer);
    }

    StreamSilencer(const StreamSilencer&) = delete;
    StreamSilencer& operator=(const StreamSilencer&) = delete;

private:
    std::ostream& mStream;
    NullBuffer mNullBuffer;
    std::streambuf* mOldBuffer;
};

uint32_t getMicros();
uint32_t getMillis();
void waitFor_ms(uint32_t inWait_ms);
