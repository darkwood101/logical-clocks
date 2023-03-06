#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <vector>

class message {
public:
    message(const uint16_t timestamp_, const uint64_t rank, const uint64_t global_time_);
    ~message();

    // Define function headers here:

private:
    // sending process rank
    const uint16_t rank_;
    // logical clock value
    const uint64_t timestamp_;
    // global system clock value
    const uint64_t global_time_;
    
};

// Constructor w/ parameters
message::message(const uint16_t rank, const uint64_t timestamp, const uint64_t global_time_):
    rank_(rank), timestamp_(timestamp), global_time_(global_time_) {}

// default constructor
message::~message() {
}

#endif
