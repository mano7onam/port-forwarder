//
// Created by mano on 10.12.16.
//

#include "Buffer.h"

Buffer::Buffer(size_t size) {
    this->size = size;

    start = 0;
    end = 0;

    buf = (char*) malloc(size);

    if (NULL == buf) {
        perror("malloc");
        is_correct = false;
        exit(EXIT_FAILURE);
    }
}

char * Buffer::get_end() {
    return buf + end;
}

char * Buffer::get_start() {
    return buf + start;
}

size_t Buffer::get_data_size() {
    return end - start;
}

bool Buffer::is_have_data() {
    return start != end;
}

size_t Buffer::get_empty_space_size() {
    return size - end;
}

int Buffer::do_resize(size_t new_size) {
    char * new_buf = (char *)realloc(buf, new_size);

    if (NULL == new_buf) {
        perror("realloc");
        free(buf);
        exit(EXIT_FAILURE);
    }

    buf = new_buf;
    size = new_size;

    return RESULT_CORRECT;
}

int Buffer::do_move_end(ssize_t received) {
    end += received;

    if (end == size) {
        return do_resize(size * 2);
    }

    return RESULT_CORRECT;
}

void Buffer::do_move_start(ssize_t sent) {
    start += sent;
}

Buffer::~Buffer() {
    fprintf(stderr, "Destructor buffer\n");
    fprintf(stderr, "Size: %ld\n", size);

    if (is_correct) {
        free(buf);
        perror("free");
    }
    else {
        fprintf(stderr, "Destructor buffer not correct\n");
    }

    fprintf(stderr, "Done buffer destructor\n");
}
