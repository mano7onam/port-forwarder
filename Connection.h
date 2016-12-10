//
// Created by mano on 10.12.16.
//

#ifndef PORTFORWARDERREDEAWROFTROP_CONNECTION_H
#define PORTFORWARDERREDEAWROFTROP_CONNECTION_H

#include "Buffer.h"

class Connection {
    int socket_read;
    int socket_write;

    bool flag_closed;

    Buffer * buffer;

    Connection * pair;

public:
    Connection(int socket_read, int socket_write);

    int get_read_socket();

    int get_write_socket();

    void set_close();

    bool is_closed();

    int do_receive();

    int do_send();

    bool is_buffer_have_data();

    void set_pair(Connection * pair);

    ~Connection();
};


#endif //PORTFORWARDERREDEAWROFTROP_CONNECTION_H
