#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <arpa/inet.h>    //close
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <vector>

#define BUFFER_SIZE (100000000)
#define PORT 4444
#define INTERVAL 1000000
#define MAX_CONNECT 100
#define MAX_CNT0 10

using namespace std;

typedef long long ll;

vector<ll> bits_receiveds;
vector<ll> tnows;
vector<ll> tavgs;
vector<ll> times;
vector<int> csokets;
vector<struct sockaddr_in> addresses;
vector<int> cnt0;

void do_print_speed_test(int sig) {       
    bool was_active = false;
    for (int i = 0; i < csokets.size(); ++i) {
        if (-1 == csokets[i]) {
            continue;
        }

        ++times[i];
        was_active = true;
        tnows[i] = (bits_receiveds[i] * 8ll) / 10000ll;
        tavgs[i] = (ll)(tavgs[i] * ((times[i] - 1) / (double)times[i]) + tnows[i] / (double)times[i]);
        fprintf(stderr, "%s:%d - Now: %lld.%lld(Mb/s), Avg: %lld.%lld(Mb/s)\n", 
            inet_ntoa(addresses[i].sin_addr), ntohs(addresses[i].sin_port),
            tnows[i] / 100ll, tnows[i] % 100ll, tavgs[i] / 100ll, tavgs[i] % 100ll);

        if (0 == bits_receiveds[i]) {
            ++cnt0[i];
        }
        else {
            cnt0[i] = 0;
        }

        if (cnt0[i] > MAX_CNT0) {
            fprintf(stderr, "Connections closed (timeout)\n");
            close(csokets[i]);
            csokets[i] = -1;
            fprintf(stderr, "After close (timeout)\n");
        }

        bits_receiveds[i] = 0;
    }  
    if (was_active) {
        fprintf(stderr, "\n");
    }
    
    alarm(1);
}

int main(int argc, char *argv[])
{      
    int master_socket = 0;
    int csocket_fd = 0;
    struct sockaddr_in addr;
    struct sockaddr_in s_addr;
    socklen_t addr_size = 0;
    unsigned short port = 0;    

    void *buf = malloc(BUFFER_SIZE + 1);
    char *message = NULL;
    int size = 0;        

    bzero(&addr, sizeof(struct sockaddr_in));
    bzero(&s_addr, sizeof(struct sockaddr_in));
    addr_size = sizeof(struct sockaddr_in);    

    port = PORT;
    s_addr.sin_family = PF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    master_socket = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    bind(master_socket, (struct sockaddr *) &s_addr, sizeof(struct sockaddr_in));
    listen(master_socket, SOMAXCONN);       
    fprintf(stderr, "Waiting connections...\n"); 

    signal(SIGALRM, do_print_speed_test);
    alarm(1);
    while (true) 
    {
        fd_set readfds; // set of socket descriptions
        int max_sd;

        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (int i = 0; i < csokets.size(); ++i) {
            int sd = csokets[i];

            if (-1 != sd) {
                FD_SET(sd, &readfds);   
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
            continue;

        if (FD_ISSET(master_socket, &readfds)) {
            int new_socket = accept(master_socket, (struct sockaddr *)&addr, &addr_size);
            fprintf(stderr, "New connection, socket fd is %d, ip is : %s, port : %d\n", 
                new_socket, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
            
            bool was_added = false;

            for (int i = 0; i < csokets.size(); ++i) {
                if (-1 != csokets[i]) {
                    continue;
                }

                csokets[i] = new_socket;
                addresses[i] = addr;
                bits_receiveds[i] = 0;
                tnows[i] = 0;
                tavgs[i] = 0;
                times[i] = 0;
                cnt0[i] = 0;

                was_added = true;
                break;
            }

            if (!was_added) {
                csokets.push_back(new_socket);
                addresses.push_back(addr);
                bits_receiveds.push_back(0);
                tnows.push_back(0);
                tavgs.push_back(0);
                times.push_back(0);
                cnt0.push_back(0);
            }
        }

        for (int i = 0; i < csokets.size(); ++i){
            int sd = csokets[i];
            if (-1 == sd) {
                continue;
            }
            
            if (FD_ISSET(sd, &readfds)) {
                int size_read = 0;
                if ((size_read = recv(sd, buf, BUFFER_SIZE, 0)) <= 0){
                    getpeername(sd, (struct sockaddr*)&addr, &addr_size);
                    printf("Host disconnected, ip: %s, port: %d\n", 
                        inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                    close(sd);
                    csokets[i] = -1;
                }
                else {     
                    //send(sd, buf, size_read, 0);
                    //fprintf(stderr, "Sent: %ld\n", send(sd, buf, size_read, 0));           
                    bits_receiveds[i] += (ll)size_read;
                }
            }            
        }
    }         

    close(master_socket);
    free(buf);       
    return EXIT_SUCCESS;
}
