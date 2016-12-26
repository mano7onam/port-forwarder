//
// Created by mano on 10.12.16.
//

#include "Connection.h"

Connection::Connection(int socket_read, int socket_write, bool flag_connected, int server_socket_flags) {
    this->socket_read = socket_read;
    this->socket_write = socket_write;

    this->flag_closed_read_socket = false;
    this->flag_closed_write_socket = false;

    this->flag_connected = flag_connected;
    this->read_socket_flags = server_socket_flags;

    buf_capacity = DEFAULT_BUFFER_SIZE;
    buf_data_size = 0;

    buf = (char*)malloc(DEFAULT_BUFFER_SIZE + 1);
    buf[DEFAULT_BUFFER_SIZE] = '\0';
}

void Connection::close_read_socket() {
    if (!is_closed_read_socket()) {
        close(socket_read);
        set_closed_read_socket();
    }
}

void Connection::close_write_socket() {
    if (!is_closed_write_socket()) {
        close(socket_write);
        set_closed_write_socket();
    }
}

int Connection::do_receive() {
    if (!flag_connected) {
        fprintf(stderr, "Http connection established\n");

        fcntl(socket_read, F_SETFL, read_socket_flags);
        flag_connected = true;

        return RESULT_CORRECT;
    }

    ssize_t received = recv(socket_read, buf + buf_data_size, buf_capacity - buf_data_size, 0);

    if (-1 == received) {
        perror("recv");

        this->close_read_socket();
        pair->set_closed_write_socket();

        return RESULT_INCORRECT;
    }
    else if (0 == received) {
        fprintf(stderr, "Receive done.\n");

        this->close_read_socket();
        pair->set_closed_write_socket();

        return RESULT_CORRECT;
    }
    else {
        buf_data_size += received;

        return RESULT_CORRECT;
    }
}

int Connection::do_send() {
    ssize_t sent = send(socket_write, buf, buf_data_size, 0);

    if (-1 == sent) {
        perror("sent");

        this->close_write_socket();
        pair->set_closed_read_socket();

        this->close_read_socket();
        pair->set_closed_write_socket();

        return RESULT_INCORRECT;
    }
    else if (0 == sent) {
        fprintf(stderr, "Send done\n");

        this->close_write_socket();
        pair->set_closed_read_socket();

        this->close_read_socket();
        pair->set_closed_write_socket();

        return RESULT_CORRECT;
    }
    else {
        memmove(buf, buf + sent, buf_data_size - sent);
        buf_data_size -= sent;

        return RESULT_CORRECT;
    }
}

void Connection::set_pair(Connection * pair) {
    this->pair = pair;
}

Connection::~Connection() {
    fprintf(stderr, "Connection destructor\n");

    free(buf);

    if (!flag_closed_read_socket && close(socket_read)) {
        perror("close_read");
    }

    if (!flag_closed_write_socket && close(socket_write)) {
        perror("close_write");
    }
}