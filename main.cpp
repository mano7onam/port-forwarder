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

int my_socket;
std::vector<Client*> clients;

void init_server_socket(unsigned short my_port) {
    my_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == my_socket) {
        perror("Error while creating serverSocket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in my_address;
    my_address.sin_family = PF_INET;
    my_address.sin_addr.s_addr = htonl(INADDR_ANY);
    my_address.sin_port = htons(my_port);

    fprintf(stderr, "%d\n", my_port);
    if (-1 == (bind(my_socket, (struct sockaddr *)&my_address, sizeof(my_address)))) {
        perror("Error while binding");
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(my_socket, 1024)) {
        perror("Error while listen()");
        exit(EXIT_FAILURE);
    }
}

unsigned short server_port = 0;
char server_address[LITTLE_STRING_SIZE];

int create_tcp_connection_to_request(Client * client) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == server_socket) {
        perror("Error while socket()");
        return RESULT_INCORRECT;
    }

    fprintf(stderr, "server_address: %s\n", server_address);
    struct hostent * host_info = gethostbyname(server_address);
    if (NULL == host_info) {
        perror("gethostbyname");
        return RESULT_INCORRECT;
    }
    struct sockaddr_in dest_addr;
    bzero(&dest_addr, sizeof(struct sockaddr_in));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = server_port;
    memcpy(&dest_addr.sin_addr, host_info->h_addr, host_info->h_length);

    fprintf(stderr, "Before connect\n");
    if (-1 == connect(server_socket, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in))) {
        perror("Error while connect()");
        return RESULT_INCORRECT;
    }
    fprintf(stderr, "After connect\n");
    client->server_socket = server_socket;

    return RESULT_CORRECT;
}

void accept_incoming_connection() {
    struct sockaddr_in client_address;
    int address_size = sizeof(sockaddr_in);
    int client_socket = accept(my_socket, (struct sockaddr *)&client_address,
                               (socklen_t *)&address_size);

    if (client_socket <= 0) {
        perror("Error while accept()");
        exit(EXIT_FAILURE);
    }

    Client * new_client = new Client(client_socket, DEFAULT_BUFFER_SIZE);
    if (RESULT_CORRECT == create_tcp_connection_to_request(new_client)) {
        clients.push_back(new_client);
    }
    else {
        new_client->set_closed_incorrect();
        delete new_client;
    }
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
    Buffer * client_buffer_in = clients[i]->buffer_in;
    ssize_t received = recv(clients[i]->my_socket, client_buffer_in->buf + client_buffer_in->end,
                            client_buffer_in->size - client_buffer_in->end, 0);
    fprintf(stderr, "%ld\n", received);
    switch (received) {
        case -1:
            perror("Error while read()");
            clients[i]->is_correct_my_socket = false;
            clients[i]->set_closed_incorrect();
            break;
        case 0:
            fprintf(stderr, "Close client\n");
            clients[i]->set_closed_correct();
            break;
        default:
            client_buffer_in->end += received;
            if (client_buffer_in->end == client_buffer_in->size) {
                client_buffer_in->resize(client_buffer_in->size * 2);
            }

    }
}

void send_answer_to_client(int i) {
    if (!clients[i]->is_closed && clients[i]->buffer_out->end > clients[i]->buffer_out->start) {
        Buffer * client_buffer_out = clients[i]->buffer_out;
        ssize_t sent = send(clients[i]->my_socket, client_buffer_out->buf + client_buffer_out->start,
                            (size_t)(client_buffer_out->end - client_buffer_out->start), 0);
        switch (sent) {
            case -1:
                perror("Error while send to client");
                clients[i]->is_correct_my_socket = false;
                clients[i]->set_closed_incorrect();
                break;
            case 0:
                clients[i]->set_closed_correct();
                break;
            default:
                client_buffer_out->start += sent;
        }
    }
}

void receive_server_response(int i) {
    Buffer * client_buffer_out = clients[i]->buffer_out;
    ssize_t received = recv(clients[i]->server_socket, client_buffer_out->buf + client_buffer_out->end,
                            (size_t)(client_buffer_out->size - client_buffer_out->end), 0);
    switch (received) {
        case -1:
            perror("recv(from server)");
            clients[i]->is_correct_server_socket = false;
            clients[i]->set_closed_incorrect();
            break;
        case 0:
            fprintf(stderr, "Close connection with server\n");
            if (0 != close(clients[i]->server_socket)) {
                perror("close");
                clients[i]->set_closed_incorrect();
                break;
            }
            clients[i]->server_socket = -1;
            break;
        default:
            client_buffer_out->end += received;
            if (client_buffer_out->end == client_buffer_out->size) {
                if (-1 == client_buffer_out->resize(client_buffer_out->size * 2)) {
                    clients[i]->set_closed_incorrect();
                    break;
                }
            }
            client_buffer_out->buf[client_buffer_out->end] = '\0';
            fprintf(stderr, "\n\nReceived from server:\n%s\n\n", client_buffer_out->buf);
            out_to_file << client_buffer_out->buf << std::endl << std::endl;
    }
}

void send_request_to_server(int i) {
    if (clients[i]->buffer_in->end > clients[i]->buffer_in->start) {
        fprintf(stderr, "\nHave data to send to server (i):%d (fd):%d\n", i, clients[i]->server_socket);

        Buffer * client_buffer_in = clients[i]->buffer_in;
        ssize_t sent = send(clients[i]->server_socket, client_buffer_in->buf,
                            (size_t)(client_buffer_in->end - client_buffer_in->start), 0);
        fprintf(stderr, "Sent to server: %ld, %ld\n", sent, client_buffer_in->end - client_buffer_in->start);

        switch (sent) {
            case -1:
                perror("Error while send to server");
                clients[i]->is_correct_server_socket = false;
                clients[i]->set_closed_incorrect();
                break;
            case 0:
                if (0 != close(clients[i]->server_socket)) {
                    perror("close");
                    clients[i]->set_closed_incorrect();
                }
                clients[i]->server_socket = -1;
                break;
            default:
                client_buffer_in->start += sent;
        }
    }
}

int main(int argc, char *argv[]) {
    unsigned short my_port = 0;
    int opt;
    int cnt = 0;
    while ((opt = getopt(argc, argv, "i:a:p:")) != -1) {
        switch (opt) {
            case 'i':
                my_port = (unsigned short)atoi(optarg);
                ++cnt;
                break;
            case 'a':
                strncpy(server_address, optarg, strlen(optarg));
                server_address[strlen(optarg)] = '\0';
                ++cnt;
                break;
            case 'p':
                server_port = (unsigned short)atoi(optarg);
                ++cnt;
                break;
            default:
                fprintf(stderr, "Unknown argument\n");
                exit(EXIT_FAILURE);
        }
    }

    if (3 != cnt) {
        fprintf(stderr, "Not enough arguments\n");
        exit(EXIT_FAILURE);
    }
    if (0 == my_port) {
        perror("Wrong port for listening");
        exit(EXIT_FAILURE);
    }
    init_server_socket(my_port);

    bool flag_execute = true;
    for ( ; flag_execute ; ) {
        fd_set fds_read;
        fd_set fds_write;
        FD_ZERO(&fds_read);
        FD_ZERO(&fds_write);
        int max_fd = 0;

        FD_SET(my_socket, &fds_read);
        max_fd = my_socket;

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

        if (FD_ISSET(my_socket, &fds_read)) {
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

    close(my_socket);

    return 0;
}