#include "Includes.h"
#include "Connection.h"

long long get_cur_time() {
    auto cur_time = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(cur_time).count();
}

int my_server_socket;

void init_my_server_socket(unsigned short my_port) {
    my_server_socket = socket(PF_INET, SOCK_STREAM, 0);

    int one = 1;
    setsockopt(my_server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (my_server_socket <= 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in my_address;
    my_address.sin_family = PF_INET;
    my_address.sin_addr.s_addr = htonl(INADDR_ANY);
    my_address.sin_port = htons(my_port);

    if (bind(my_server_socket, (struct sockaddr *)&my_address, sizeof(sockaddr_in))) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(my_server_socket, 1024)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

unsigned short server_port = 0;
char server_address[LITTLE_STRING_SIZE];

int do_new_connect_with_server() {
    struct hostent * host_info = gethostbyname(IP_ADDRESS);

    if (NULL == host_info) {
        perror("gethostbyname");
        return RESULT_INCORRECT;
    }

    int server_socket = socket(PF_INET, SOCK_STREAM, 0);

    if (-1 == server_socket) {
        perror("Error while socket()");
        return RESULT_INCORRECT;
    }

    struct sockaddr_in dest_addr;

    dest_addr.sin_family = PF_INET;
    dest_addr.sin_port = htons(server_port);
    memcpy(&dest_addr.sin_addr, host_info->h_addr, host_info->h_length);

    if (connect(server_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) {
        perror("Error while connect()");
        return RESULT_INCORRECT;
    }

    return server_socket;
}

std::vector<Connection*> connections;

int do_accept_connection() {
    struct sockaddr_in client_address;
    int address_size = sizeof(sockaddr_in);

    int client_socket = accept(my_server_socket, (struct sockaddr *)&client_address, (socklen_t *) &address_size);

    if (client_socket <= 0) {
        perror("accept");
        return RESULT_INCORRECT;
    }

    int server_socket = do_new_connect_with_server();

    if (RESULT_INCORRECT != server_socket) {
        Connection * connection1 = new Connection(client_socket, server_socket);
        Connection * connection2 = new Connection(server_socket, client_socket);

        connection1->set_pair(connection2);
        connection2->set_pair(connection1);

        connections.push_back(connection1);
        connections.push_back(connection2);

        return RESULT_CORRECT;
    }
    else {
        return RESULT_INCORRECT;
    }
}

void delete_closed_connections() {
    std::vector<Connection*> rest_connections;

    for (Connection * con : connections) {
        if (con->is_closed()) {
            delete con;
        }
        else {
            rest_connections.push_back(con);
        }
    }

    connections = rest_connections;
}

void init_all(int argc, char * argv[]) {
    unsigned short my_port = 0;
    int opt;
    int cnt_args = 0;

    while ((opt = getopt(argc, argv, "i:a:p:")) != -1) {
        switch (opt) {
            case 'i':
                my_port = (unsigned short)atoi(optarg);
                ++cnt_args;
                break;

            case 'a':
                strncpy(server_address, optarg, strlen(optarg));
                server_address[strlen(optarg)] = '\0';
                ++cnt_args;
                break;

            case 'p':
                server_port = (unsigned short)atoi(optarg);
                ++cnt_args;
                break;

            default:
                fprintf(stderr, "Unknown argument\n");
                exit(EXIT_FAILURE);
        }
    }

    if (3 != cnt_args) {
        fprintf(stderr, "Not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    if (0 == my_port) {
        fprintf(stderr, "Bad port\n");
        exit(EXIT_FAILURE);
    }

    init_my_server_socket(my_port);
}

void signal_handle(int sig) {
    fprintf(stderr, "Exit with code %d\n", sig);

    for (auto con : connections) {
        delete con;
    }

    close(my_server_socket);

    exit(EXIT_SUCCESS);
}

void start_main_loop() {
    bool flag_execute = true;
    for ( ; flag_execute ; ) {
        fd_set fds_read;
        fd_set fds_write;

        FD_ZERO(&fds_read);
        FD_ZERO(&fds_write);

        FD_SET(my_server_socket, &fds_read);
        int max_fd = my_server_socket;

        for (auto con : connections) {
            FD_SET(con->get_read_socket(), &fds_read);
            max_fd = std::max(max_fd, con->get_read_socket());

            if (con->is_buffer_have_data()) {
                FD_SET(con->get_write_socket(), &fds_write);
                max_fd = std::max(max_fd, con->get_write_socket());
            }
        }

        delete_closed_connections();

        int activity = select(max_fd + 1, &fds_read, &fds_write, NULL, NULL);

        if (activity <= 0) {
            perror("select");
            continue;
        }

        if (FD_ISSET(my_server_socket, &fds_read)) {
            fprintf(stderr, "Accept new connection\n");
            do_accept_connection();
        }

        for (auto con : connections) {
            if (!con->is_closed() && FD_ISSET(con->get_read_socket(), &fds_read)) {
                con->do_receive();
            }
            if (!con->is_closed() && FD_ISSET(con->get_write_socket(), &fds_write)) {
                con->do_send();
            }
        }

        delete_closed_connections();
    }
}

int main(int argc, char * argv[]) {
    signal(SIGINT, signal_handle);
    signal(SIGKILL, signal_handle);
    signal(SIGTERM, signal_handle);

    init_all(argc, argv);

    start_main_loop();

    for (auto con : connections) {
        delete con;
    }

    close(my_server_socket);

    return 0;
}