//
// Created by mano on 10.12.16.
//

#ifndef PORTFORWARDERREDEAWROFTROP_CONNECTION_H
#define PORTFORWARDERREDEAWROFTROP_CONNECTION_H

#include "Buffer.h"

class Connection {
    int socket_read;
    int socket_write;

    bool flag_closed_write_socket;
    bool flag_closed_read_socket;

    bool flag_connected;
    int read_socket_flags;

    char * buf;
    size_t buf_data_size;
    size_t buf_capacity;

    Connection * pair;

public:
    Connection(int socket_read, int socket_write, bool flag_connected, int server_socket_flags);

    bool is_buffer_have_data() { return buf_data_size > 0; }

    bool buffer_have_empty_space() { return buf_capacity - buf_data_size > 0; }

    int get_read_socket() { return socket_read; }

    int get_write_socket() { return socket_write; }

    bool is_closed_read_socket() { return flag_closed_read_socket; }

    bool is_closed_write_socket() { return flag_closed_write_socket; }

    void set_closed_read_socket() { flag_closed_read_socket = true; }

    void set_closed_write_socket() { flag_closed_write_socket = true; }

    void close_all() { close_read_socket(); close_write_socket(); }

    bool can_to_delete() { return flag_closed_read_socket && flag_closed_write_socket; }

    void close_read_socket();

    void close_write_socket();

    int do_receive();

    int do_send();

    void set_pair(Connection * pair);

    Connection * get_pair() { return pair; }

    ~Connection();
};


#endif //PORTFORWARDERREDEAWROFTROP_CONNECTION_H