#include <cstdlib> //exit, atoi
#include <cstdio> //perror
#include <netdb.h> //gethostbyname
#include <cstring> //memcpy
#include <sys/socket.h> //sockaddr_in, AF_INET, socket, bind, listen, connect
#include <poll.h> //poll
#include <unistd.h> //read, write, close
#include <arpa/inet.h> //htonl, htons
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <fstream>

#define LITTLE_STRING_SIZE 4096
#define DEFAULT_BUFFER_SIZE (4096)
#define DEFAULT_PORT 80

#define RESULT_CORRECT 1
#define RESULT_INCORRECT -1

std::ofstream out_to_file("output.txt");

struct Buffer {
    char * buf;
    size_t start;
    size_t end;
    size_t size;
    bool is_correct;

    Buffer(size_t size_buf) {
        buf = (char*)malloc(size_buf);
        if (NULL == buf) {
            perror("new buf");
            is_correct = false;
            exit(EXIT_FAILURE);
        }
        start = 0;
        end = 0;
        size = size_buf;
        is_correct = true;
    }

    int resize(size_t new_size_buf) {
        char * result = (char *)realloc(buf, new_size_buf);
        if (NULL == result) {
            perror("realloc");
            free(buf);
            exit(EXIT_FAILURE);
        }
        buf = result;
        size = new_size_buf;
    }

    // todo rename to 'push_back_data'
    void add_data_to_end(const char * from, size_t size_data) {
        while (end + size_data > size) {
            resize(size * 2);
        }
        memcpy(buf + end, from, size_data);
        end += size_data;
    }

    // todo rename to 'push_back_symbol'
    void add_symbol_to_end(char c) {
        while (end + 1 > size) {
            resize(size * 2);
        }
        buf[end++] = c;
    }

    ~Buffer() {
        fprintf(stderr, "Destructor buffer!!!!\n");
        fprintf(stderr, "Buf: %d\n", buf);
        fprintf(stderr, "Size: %ld\n", size);
        if (is_correct) {
            free(buf);
            perror("free");
        }
        else {
            fprintf(stderr, "Destructor buffer not correct\n");
        }
        fprintf(stderr, "Done\n");
    }
};

struct Client {
    int my_socket;
    int server_socket;
    bool is_correct_my_socket;
    bool is_correct_server_socket;
    Buffer * buffer_in;
    Buffer * buffer_out;
    bool is_closed;
    bool is_closed_correct;
    struct sockaddr_in dest_addr;
    long long last_time_activity;

    Client(int my_socket, size_t size_buf) {
        this->my_socket = my_socket;
        this->server_socket = -1;
        is_correct_my_socket = true;
        is_correct_server_socket = true;
        buffer_in = new Buffer(size_buf);
        buffer_out = new Buffer(size_buf);
        is_closed = false;
    }

    void set_closed_correct() {
        is_closed = true;
        is_closed_correct = true;
    }

    void set_closed_incorrect() {
        is_closed = true;
        is_closed_correct = false;
    }

    ~Client() {
        fprintf(stderr, "Destructor client!!!!\n");
        if (-1 != my_socket && is_correct_my_socket) {
            close(my_socket);
        }
        if (-1 != server_socket && is_correct_server_socket) {
            close(server_socket);
        }
        delete buffer_in;
        delete buffer_out;
    }
};

int server_socket;
std::vector<Client*> clients;

void init_server_socket(unsigned short server_port) {
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == server_socket) {
        perror("Error while creating serverSocket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = PF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(server_port);

    if (-1 == (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)))) {
        perror("Error while binding");
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(server_socket, 1024)) {
        perror("Error while listen()");
        exit(EXIT_FAILURE);
    }
}

void accept_incoming_connection() {
    struct sockaddr_in client_address;
    int address_size = sizeof(sockaddr_in);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_address,
                               (socklen_t *)&address_size);

    if (client_socket <= 0) {
        perror("Error while accept()");
        exit(EXIT_FAILURE);
    }

    Client * new_client = new Client(client_socket, DEFAULT_BUFFER_SIZE);
    clients.push_back(new_client);
}

int create_tcp_connection_to_request(int i) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == server_socket) {
        perror("Error while socket()");
        return RESULT_INCORRECT;
    }

    struct sockaddr_in dest_addr = clients[i]->dest_addr;
    if (-1 == connect(server_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) {
        perror("Error while connect()");
        return RESULT_INCORRECT;
    }
    clients[i]->server_socket = server_socket;

    return RESULT_CORRECT;
}

void delete_finished_clients() {
    //fprintf(stderr, "\nClients size before clean: %ld\n", clients.size());
    std::vector<Client*> rest_clients;
    for (int i = 0; i < clients.size(); ++i) {
        if (clients[i]->is_closed) {
            //fprintf(stderr, "%d - delete\n", i);
            delete clients[i];
        }
        else {
            //fprintf(stderr, "%d - rest, buf: %d, %d\n", i, clients[i]->buffer_in->buf, clients[i]->buffer_out->buf);
            rest_clients.push_back(clients[i]);
        }
    }
    clients = rest_clients;
    //fprintf(stderr, "Clients size after clean: %ld\n", clients.size());
}

void receive_request_from_client(int i) {

}

void send_answer_to_client(int i) {

}

void receive_server_response(int i) {

}

void send_request_to_server(int i) {

}

int main(int argc, char *argv[]) {
    // todo GET_OPT!!!

    if (2 != argc) {
        perror("Wrong number of arguments");
        exit(EXIT_FAILURE);
    }

    unsigned short server_port = (unsigned short)atoi(argv[1]);
    if (0 == server_port) {
        perror("Wrong port for listening");
        exit(EXIT_FAILURE);
    }
    init_server_socket(server_port);

    bool flag_execute = true;
    for ( ; flag_execute ; ) {
        fd_set fds_read;
        fd_set fds_write;
        FD_ZERO(&fds_read);
        FD_ZERO(&fds_write);
        int max_fd = 0;

        FD_SET(server_socket, &fds_read);
        max_fd = server_socket;

        for (auto client : clients) {
            FD_SET(client->my_socket, &fds_read);
            FD_SET(client->my_socket, &fds_write);
            FD_SET(client->server_socket, &fds_read);
            FD_SET(client->server_socket, &fds_write);
            max_fd = std::max(max_fd, client->my_socket);
            max_fd = std::max(max_fd, client->server_socket);
        }

        int activity = select(max_fd + 1, &fds_read, &fds_write, NULL, NULL);
        //fprintf(stderr, "Activity: %d\n", activity);
        if (-1 == activity) {
            perror("Error while select()");
            continue;
        }
        else if (0 == activity) {
            perror("poll() returned 0");
            continue;
        }

        if (FD_ISSET(server_socket, &fds_read)) {
            fprintf(stderr, "Have incoming client connection\n");
            accept_incoming_connection();
        }

        for (int i = 0; i < clients.size(); ++i) {
            if (FD_ISSET(clients[i]->my_socket, &fds_read)) {
                fprintf(stderr, "Have data from client %d\n", i);
                receive_request_from_client(i);
            }
            if (clients[i]->is_closed) {
                continue;
            }
            if (-1 != clients[i]->server_socket && FD_ISSET(clients[i]->server_socket, &fds_write)) {
                send_request_to_server(i);
            }
        }

        delete_finished_clients();

        for (int i = 0; i < clients.size(); ++i) {
            if (FD_ISSET(clients[i]->server_socket, &fds_read)) {
                fprintf(stderr, "Have data from server, (id):%d\n", i);
                receive_server_response(i);
            }
            if (clients[i]->is_closed) {
                continue;
            }
            if (FD_ISSET(clients[i]->my_socket, &fds_write)) {
                send_answer_to_client(i);
            }
        }

        delete_finished_clients();
    }

    // close clients
    for (auto client : clients) {
        delete client;
    }

    close(server_socket);

    return 0;
}