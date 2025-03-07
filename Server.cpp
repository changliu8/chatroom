#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unordered_map>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888 // listening port
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
using namespace std;

void handle_client(int client_sock);
void send_msg(const string &msg);
void output(const char *arg, ...);
int error_output(const char *arg, ...);
void error_handling(const std::string &message);

int client_count = 0;
mutex mtx;

unordered_map<string, int> client_socks;

int main(int argc, const char **argv, const char **envp)
{
    int server_sock, client_sock;
    /*sockaddr_in {
    sa_family_t sin_family (address family),
    in_port_t sin_port (port number),
    struct in_addr sin_addr (IP address),
    char sin_zero[8] (padding to make the struct the same size as sockaddr)
    }*/

    /*
    struct in_addr{
        In_addr_t, s_addr; // 32-bit IP address
    }
    */
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;

    // create socket, socket(domain, type, protocol)
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    // check if the socket is created successfully
    if (server_sock == -1)
    {
        error_handling("socket() error");
    }
    // bind socket to corresponding address
    // use 0 to padding serv_addr, which is sockaddr_in
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    // htonl represent accepte host byte order to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // listening any ip address
    // htons represent convert bit to network byte order
    server_addr.sin_port = htons(SERVER_PORT); // set a port number.

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        error_handling("bind() failed");
    }
    printf("The server is running on port %d\n", SERVER_PORT);
    if (listen(server_sock, MAX_CLIENTS) == -1)
    {
        error_handling("listen() error!");
    }
    while (1)
    {
    }
}

void handle_client(int client_sock)
{
    char msg[BUF_SIZE];
    int flag = 0;

    // first time
    char tell_name[13] = "#new client:";
    // recv(socket, message, len of message, flag(0 : regular data, MSG_PEEK and MSG_OOB))
    while (recv(client_sock, msg, sizeof(msg), 0) != 0)
    {
        if (strlen(msg) > strlen(tell_name))
        {
            char pre_name[13];
            strncpy(pre_name, msg, 12);
            pre_name[12] = '\0';
            if (strcmp(pre_name, tell_name) == 0)
            {
                char name[20];
                strcpy(name, msg + 12);
                if (client_socks.find(name) == client_socks.end())
                {
                    output("the name of socket %d: %s\n", client_sock, name);
                    client_socks[name] = client_sock;
                }
                else
                {
                    string error_msg = std::string(name) + " exists already. Please quit and enter with another name!";
                    send(client_sock, error_msg.c_str(), error_msg.length() + 1, 0);
                    mtx.lock();
                    client_count--;
                    mtx.unlock();
                    flag = 1;
                }
            }
        }
    }
}
