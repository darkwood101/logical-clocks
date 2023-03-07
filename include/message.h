#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>

class message {
public:
    message();
    message(const uint16_t rank, const uint64_t timestamp);

    // sending process rank
    uint16_t rank_;
    // logical clock value of the sending process
    uint64_t timestamp_;
};


#endif
