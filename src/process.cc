#include "process.h"

#include <iostream>
#include <sched.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

process::process(const uint16_t rank, const uint16_t world_size) :
    rank_(rank), world_size_(world_size), rng_(std::random_device {}()),
    clock_speed_(0), socket_path_prefix_("socket_path_"), listen_fd_(-1),
    other_procs_fds_(world_size_ - 1, -1) {
    if (rank_ >= world_size_) {
        throw std::logic_error("Rank must be smaller than world size");
    }
}

process::~process() {
    // Close the listening socket if needed
    if (listen_fd_ != -1) {
        close(listen_fd_);
    }
    // Gracefully close other sockets
    for (const int fd : other_procs_fds_) {
        if (fd == -1) {
            continue;
        }
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    // Delete the socket so that it can be reused
    unlink((socket_path_prefix_ + std::to_string(rank_)).c_str());
}

int process::initialize_clock_speed() {
    std::uniform_int_distribution<uint32_t> uid(1, 6);
    clock_speed_ = uid(rng_);
    return 0;
}

int process::open_log_file() {
    outfile_.open("data/process" + std::to_string(rank_) + ".log",
                  std::ofstream::out);
    return outfile_.fail();
}

int process::open_connections() {
    // This process listens to all processes with greater rank, so don't open a
    // socket if it has the greatest rank
    if (rank_ != world_size_ - 1) {
        listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (listen_fd_ < 0) {
            outfile_ << "Could not open a socket: " << strerror(errno) << "\n";
            return -1;
        }
        std::string socket_path = socket_path_prefix_ + std::to_string(rank_);
        sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        // Socket path must not exceed the maximum, including null terminator
        if (socket_path.length() + 1 >= sizeof(addr.sun_path)) {
            outfile_ << "Socket path \"" << socket_path
                     << "\" exceeds maximum length\n";
            return -1;
        }
        strcpy(addr.sun_path, socket_path.c_str());
        // Just in case, unlink the socket
        unlink(socket_path.c_str());
        if (bind(listen_fd_,
                 reinterpret_cast<const sockaddr*>(&addr),
                 sizeof(addr)) < 0) {
            outfile_ << "Could not bind the socket on socket path \""
                     << socket_path << "\": " << strerror(errno) << "\n";
            return -1;
        }
        // Accept only as many connections as there are processes with a greater
        // rank
        if (listen(listen_fd_, 64) < 0) {
            outfile_ << "Could not listen on the socket: " << strerror(errno)
                     << "\n";
            return -1;
        }
        socklen_t addr_len = sizeof(addr);
        for (uint16_t i = rank_; i != world_size_ - 1; ++i) {
            other_procs_fds_[i] = accept(listen_fd_,
                                         reinterpret_cast<sockaddr*>(&addr),
                                         &addr_len);
            if (other_procs_fds_[i] < 0) {
                outfile_ << "Could not accept the connection: "
                         << strerror(errno) << "\n";
                return -1;
            }
        }
    }

    // This process connects to all processes with smaller rank
    for (uint16_t i = 0; i != rank_; ++i) {
        other_procs_fds_[i] = socket(AF_UNIX, SOCK_STREAM, 0);
        if (other_procs_fds_[i] < 0) {
            outfile_ << "Could not open a socket: " << strerror(errno) << "\n";
            return -1;
        }
        std::string socket_path = socket_path_prefix_ + std::to_string(i);
        sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        // Socket path must not exceed the maximum, including null terminator
        if (socket_path.length() + 1 >= sizeof(addr.sun_path)) {
            outfile_ << "Socket path \"" << socket_path
                     << "\" exceeds maximum length\n";
            return -1;
        }
        strcpy(addr.sun_path, socket_path.c_str());
        while (connect(other_procs_fds_[i],
                       reinterpret_cast<const sockaddr*>(&addr),
                       sizeof(addr)) < 0) {
            // Due to race conditions, we might fail to connect at first, but
            // we yield the CPU and try again later
            sched_yield();
        }
    }

    return 0;
}

int process::execute() {
    // TODO Cynthia: Implement execute here
    //
    // The following information can be accessed through the `process` class:
    //
    // `clock_speed_` - the number between 1 and 6
    //
    // `outfile_` - output log file for this process, can just do `outfile_ <<
    // "hello"`, for example
    //
    // `other_procs_fds_` - a vector of socket descriptors for other processes,
    // we care about `other_procs_fds_[0]` and `other_procs_fds_[1]`. With 3
    // processes, the size of this vector will always be 3. To relate this to
    // what we wrote in the docs, `process_a_fd` would be `other_procs_fds_[0]`,
    // and `process_b_fd` would be `other_procs_fds_[1]`
    //
    // `rng_` - random number generator. See `process::initialize_clock_speed()`
    // on how you can initialize a uniform distribution for generating random
    // numbers
    //
    // `rank_` - the ID of this process, in range 0-2, inclusive. This might be
    // included in the message? Not sure.
    //
    //
    // Besides writing the loop from the docs, you will also need to do at least
    // 2 other things that I can think of right now:
    //    1. Figure out what goes in a message, and define a message struct,
    //       maybe in a header file like `include/message.h`
    //    2. Add logical clock to the `process` class. This will probably be an
    //       unsigned int, likely `uint64_t`, that you can
    //       put in `process.h` in the `private` section of the class.

    return 0;
}
