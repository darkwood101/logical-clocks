#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <vector>

class message {
public:
    message(const uint16_t timestamp_, const uint16_t rank);
    ~message();

    // Prevent copy/move
    message(const message&) = delete;
    message(message&&) = delete;
    message& operator=(const message&) = delete;
    message& operator=(message&&) = delete;

    // Define function headers here:

private:
    // sending process rank
    const uint16_t rank_;
    // logical clock value
    const uint16_t timestamp_;
    
};

// Constructor w/ parameters
message::message(const uint16_t rank, const uint16_t timestamp):
    rank_(rank), timestamp_(timestamp) {}

// default constructor
message::~message() {
}

#endif
