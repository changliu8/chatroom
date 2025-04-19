#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <stdarg.h>

#define SERVER_PORT 1111 // Server Listening Port
#define BUF_SIZE 1024    // buffer size
#define MAX_CLNT 256     // MAX CLIENT

void handle_clnt(int clnt_sock);
void send_msg(const std::string &msg, int clnt_sock);
int output(const char *arg, ...);
int error_output(const char *arg, ...);
void error_handling(const std::string &message);

int clnt_cnt = 0;
std::mutex mtx;
// Store client_name and corresponding socket.
std::unordered_map<std::string, int> clnt_socks;

int main(int argc, const char **argv, const char **envp)
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    // create a socket
    //   AF_INET: IPv4
    //   SOCK_STREAM: TCP
    //   IPPROTO_TCP: TCP
    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == -1)
    {
        error_handling("socket() failed!");
    }

    //   allocate memory for serv_addr using 0.
    memset(&serv_addr, 0, sizeof(serv_addr));
    //   set IPv4
    serv_addr.sin_family = AF_INET;
    //   set IP addgress
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //   set port number
    serv_addr.sin_port = htons(SERVER_PORT);

    //   bind the server socket to the given address
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handling("bind() failed!");
    }
    printf("the server is running on port %d\n", SERVER_PORT);
    // listing the socket until client connects.
    if (listen(serv_sock, MAX_CLNT) == -1)
    {
        error_handling("listen() error!");
    }

    while (1)
    { // continusly listening the port
        clnt_addr_size = sizeof(clnt_addr);
        // accepts pause the process until client connects in
        // returns client socket descriptor, -1 means failed
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
        {
            error_handling("accept() failed!");
        }

        // increase the total client number
        mtx.lock();
        clnt_cnt++;
        mtx.unlock();

        // generate a thread to the given user
        std::thread th(handle_clnt, clnt_sock);
        th.detach();

        output("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

void handle_clnt(int clnt_sock)
{
    char msg[BUF_SIZE];
    int flag = 0;

    // propogate user to all clients in the chat room
    char tell_name[13] = "#new client:";
    while (recv(clnt_sock, msg, sizeof(msg), 0) != 0)
    {
        // check if this is the first message ever in the chatroom
        if (std::strlen(msg) > std::strlen(tell_name))
        {
            // whether it is new client
            char pre_name[13];
            std::strncpy(pre_name, msg, 12);
            pre_name[12] = '\0';
            if (std::strcmp(pre_name, tell_name) == 0)
            {
                // declare client name
                char name[20];
                std::strcpy(name, msg + 12);
                if (clnt_socks.find(name) == clnt_socks.end())
                {
                    output("the name of socket %d: %s\n", clnt_sock, name);
                    clnt_socks[name] = clnt_sock;
                }
                else
                {
                    // whether the name is redundent
                    std::string error_msg = std::string(name) + " exists already. Please quit and enter with another name!";
                    send(clnt_sock, error_msg.c_str(), error_msg.length() + 1, 0);
                    mtx.lock();
                    clnt_cnt--;
                    mtx.unlock();
                    flag = 1;
                }
            }
        }

        if (flag == 0)
            send_msg(std::string(msg), clnt_sock);
    }
    if (flag == 0)
    {
        // client close connection, remove from the list.
        std::string leave_msg;
        std::string name;
        mtx.lock();
        for (auto it = clnt_socks.begin(); it != clnt_socks.end(); ++it)
        {
            if (it->second == clnt_sock)
            {
                name = it->first;
                clnt_socks.erase(it->first);
            }
        }
        clnt_cnt--;
        mtx.unlock();
        leave_msg = "client " + name + " leaves the chat room";
        send_msg(leave_msg, clnt_sock);
        output("client %s leaves the chat room\n", name.c_str());
        close(clnt_sock);
    }
    else
    {
        close(clnt_sock);
    }
}

void send_msg(const std::string &msg, int clnt_sock)
{
    mtx.lock();
    // if it starts with @, it means dm
    // otherwise it sends to all users
    std::string pre = "@";
    int first_space = msg.find_first_of(" ");
    if (msg.compare(first_space + 1, 1, pre) == 0)
    {
        // dm
        int space = msg.find_first_of(" ", first_space + 1);
        std::string receive_name = msg.substr(first_space + 2, space - first_space - 2);
        std::string send_name = msg.substr(1, first_space - 2);
        if (clnt_socks.find(receive_name) == clnt_socks.end())
        {
            // if user is not exist
            std::string error_msg = "[error] there is no client named " + receive_name;
            send(clnt_socks[send_name], error_msg.c_str(), error_msg.length() + 1, 0);
        }
        else
        {
            send(clnt_socks[receive_name], msg.c_str(), msg.length() + 1, 0);
            // maybe I can remove this part;
            send(clnt_socks[send_name], msg.c_str(), msg.length() + 1, 0);
        }
    }
    else
    {
        // propogate to all users
        for (auto it = clnt_socks.begin(); it != clnt_socks.end(); it++)
        {
            if (it->second != clnt_sock)
            {
                send(it->second, msg.c_str(), msg.length() + 1, 0);
            }
        }
    }
    mtx.unlock();
}

int output(const char *arg, ...)
{
    int res;
    va_list ap;
    va_start(ap, arg);
    res = vfprintf(stdout, arg, ap);
    va_end(ap);
    return res;
}

int error_output(const char *arg, ...)
{
    int res;
    va_list ap;
    va_start(ap, arg);
    res = vfprintf(stderr, arg, ap);
    va_end(ap);
    return res;
}

void error_handling(const std::string &message)
{
    std::cerr << message << std::endl;
    exit(1);
}