#include "process.h"

#include <climits>
#include <cstring>
#include <iostream>
#include <limits>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

template <typename T>
int str_to_unsigned(char const* str, T& res) {
    char* endptr;
    errno = 0;
    unsigned long long num = strtoull(str, &endptr, 10);
    if ((errno == ERANGE && num == ULLONG_MAX) || endptr == str ||
        *endptr != '\0' || num > std::numeric_limits<T>::max()) {
        return -1;
    }
    res = static_cast<T>(num);
    return 0;
}

static void usage(char const* prog) {
    std::cerr << "usage: " << prog
              << " [-h] <world size>\n"
                 "\n"
                 "Start the distributed system model with <world size> number "
                 "of processes.\n"
                 "\n"
                 "Options:\n"
                 "\t-h\t\t Display this message and exit.\n";
}

static void murder_all_children(const std::vector<pid_t>& children) {
    for (const pid_t child : children) {
        if (child == -1) {
            continue;
        }
        kill(child, SIGKILL);
    }
}

static int execute(const uint16_t rank, const uint16_t world_size) {
    int status;

    process p(rank, world_size);
    status = p.open_log_file();
    if (status != 0) {
        return status;
    }
    status = p.initialize_clock_speed();
    if (status != 0) {
        return status;
    }
    status = p.open_connections();
    if (status != 0) {
        return status;
    }

    // LATER: uncomment this!!!
    
    // status = p.execute();
    // if (status != 0) {
    //     return status;
    // }

    return 0;
}

int main(int argc, char** argv) {
    // Argument parsing
    if (argc != 2 && argc != 3) {
        std::cerr << "Wrong number of arguments\n";
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 3) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[2], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            std::cerr << "Invalid arguments\n";
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (strcmp(argv[1], "-h") == 0) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }
    uint16_t world_size;
    if (str_to_unsigned(argv[1], world_size) != 0) {
        std::cerr << "Invalid world size " << argv[1] << "\n";
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Open directory for logs if it doesn't already exist
    struct stat stat_buf;
    if (stat("data/", &stat_buf) == -1) {
        if (mkdir("data/", S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
            perror("Could not create a directory for logs");
            return EXIT_FAILURE;
        }
    }

    std::vector<pid_t> children(world_size, -1);
    // Fork all the children, and have them execute
    for (uint16_t i = 0; i != world_size; ++i) {
        pid_t p = fork();
        if (p < 0) {
            // If something goes wrong, kill all children
            perror("Could not fork");
            murder_all_children(children);
            return EXIT_FAILURE;
        } else if (p == 0) {
            // In the child
            if (execute(i, world_size) == 0) {
                return EXIT_SUCCESS;
            } else {
                return EXIT_FAILURE;
            }
        } else {
            // In the parent
            children[i] = p;
        }
    }

    for (uint16_t num_children = 0; num_children != world_size;
         ++num_children) {
        // If anything goes wrong, kill all children and exit
        int wstatus;
        pid_t p = wait(&wstatus);
        if (p < 0) {
            perror("Could not call wait");
            murder_all_children(children);
            return EXIT_FAILURE;
        } else if (!WIFEXITED(wstatus)) {
            std::cerr << "Child " << p << " terminated abnormally, status " << wstatus << "\n";
            murder_all_children(children);
            return EXIT_FAILURE;
        } else if (WEXITSTATUS(wstatus) != EXIT_SUCCESS) {
            std::cerr << "Child " << p << " exited with status " << WEXITSTATUS(wstatus) << "\n";
            murder_all_children(children);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
