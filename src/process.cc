#include "process.h"
#include "message.h"

#include <iostream>
#include <sched.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>


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

// Function to handle poll-receive message logic
// Write into log who it received the message from
bool process::recv_msg(int send_process_fd) {

    pollfd fd;
    memset(&fd, 0, sizeof(pollfd));
    fd.fd = send_process_fd; // waiting for this file desc
    fd.events |= POLLIN; // waiting for this event

    while (poll(&fd, 1, 0) != 0) {
        message msg();
        size_t total_recvd = 0;
        ssize_t recvd = 0;

        // Receive message into msg buffer until # of bytes received is correct
        while (total_recvd != sizeof(msg)) {
            // *** not sure if i did the process_a_fd stuff correctly here
            recvd = recv(send_process_fd, ((char*) &msg) + total_recvd, sizeof(msg) - total_recvd, 0);
            if (recvd <= 0) {
                // Throw ERROR!!!
            }
            total_recvd += recvd;
        }
        // *** need to update message queue here?
        // print message received here (?)
    }
    
}

// Function to handle send message
bool process::send_msg(uint16_t rank, uint16_t timestamp, int receive_proc_fd) {  
    // prepare msg…
    message msg(rank, timestamp);
    
    size_t total_sent = 0;
    ssize_t sent = 0;

    while (total_sent != sizeof(msg)) {
        sent = send(receive_proc_fd, ((char*) &msg) + total_sent, sizeof(msg) - total_sent, 0);
        if (sent <= 0) {
            // ERROR!
            // exit();
        }
        total_sent += sent;
    }
}

int process::execute() {
    int process_a_fd = other_procs_fds_[0];
    int process_b_fd = other_procs_fds_[1];
    int sleep_length = clock_speed_; // (clock speed)
    // define message queue
    //std::queue<message> queue;

    while (true) {
        // poll + receive from process a
        if (recv_msg(process_a_fd)) {
            // print receive?
            // update message queue?
            // update_logical_clock(); - IMPLEMENT
        } 
        // poll + receive from process b
        else if (recv_msg(process_b_fd)) {
            // print something here?
            // update_logical_clock(); - IMPLEMENT
        }
        else {
			// generate random number from 1 to 10
            std::uniform_int_distribution<uint32_t> uid(1, 10);
            int num = uid(rng_);

			// external events
			if (num == 1) {
                // send: 
                uint16_t timestamp = 0; // COMPUTE TIMESTAMP HERE
				send_msg(rank_, timestamp, process_a_fd);
                // update_logical_clock(); - IMPLEMENT
			} 
            else if (num == 2) {
				uint16_t timestamp = 0; // COMPUTE TIMESTAMP HERE
				send_msg(rank_, timestamp, process_b_fd);
                // update_logical_clock(); - IMPLEMENT
			} 
            else if (num == 3) {
				uint16_t timestamp = 0; // COMPUTE TIMESTAMP HERE
				send_msg(rank_, timestamp, process_a_fd);
                send_msg(rank_, timestamp, process_b_fd);

                //update_logical_clock();
			} 
            else {
				// internal event (add one to value of clock, probably)

				// update_logical_clock();
			}
		}
		sleep(0.1);
	}


    return 0;
}
