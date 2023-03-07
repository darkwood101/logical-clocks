#ifndef PROCESS_H
#define PROCESS_H

#include "message.h"

#include <cstdlib>
#include <fstream>
#include <queue>
#include <random>
#include <string>
#include <vector>

class process {
public:
    process(const uint16_t rank, const uint16_t world_size);
    ~process();

    // Prevent copy/move
    process(const process&) = delete;
    process(process&&) = delete;
    process& operator=(const process&) = delete;
    process& operator=(process&&) = delete;

    // Set `clock_speed_` to a randomly generated clock speed
    // @return 0  - OK
    // @return -1 - Error
    int initialize_clock_speed();

    // Open a log file and store the output file stream in `outfile_`.
    // @return 0  - OK
    // @return -1 - Error
    int open_log_file();

    // Connect to all other processes and store socket file descriptors in
    // `other_procs_fd_`. The listening socket (if any) is stored in
    // `listen_fd_`.
    // @return 0  - OK
    // @return -1 - Error
    int open_connections();

    int execute();

    void recv_msg(int send_proc_fd);
    void send_msg(int receive_proc_fd) const;

private:
    // Rank of this process
    const uint16_t rank_;

    // Logical clock value
    uint64_t logical_clock_;

    // Total number of processes
    const uint16_t world_size_;

    // Random number generator for this process
    std::mt19937 rng_;

    // Clock speed for this process. This means that the process performs
    // `clock_speed_` operations per second.
    uint32_t clock_speed_;

    // Output log file for this process
    std::ofstream outfile_;

    // Prefix for all processes' socket paths
    const std::string socket_path_prefix_;

    // Listening socket
    int listen_fd_;

    // File descriptors of other processes
    // `other_procs_fds_.size() == world_size_ - 1`
    // Index in this vector implies nothing about the process rank
    std::vector<int> other_procs_fds_;

    std::queue<message> msg_q_;
};

#endif
