#include "message.h"

// Constructor w/ parameters
message::message(const uint16_t rank, const uint64_t timestamp) :
    rank_(rank), timestamp_(timestamp) {
}

message::message() {
}
