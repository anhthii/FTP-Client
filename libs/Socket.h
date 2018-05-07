#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <string>

class BaseSocket {
  int socketFd; // socket file descriptor

  protected:
    static const int invalidSocketFd = -1;
    BaseSocket(int socketFd);
    int getSocketFd() const { return socketFd; }

  public:
    void close();
    ~BaseSocket();
};

class DataSocket : public BaseSocket {
  public:
    const static int MAX_BUFF_SIZE = 1024; 
    DataSocket(int socketFd) : BaseSocket(socketFd) {}
    std::string receiveMessage();
    void sendMessage(const std::string& msg);
};

class HostSocket : public BaseSocket {
  struct sockaddr_in _host_addr;

  public:
    HostSocket(unsigned int port);
    DataSocket accept();
};

class ConnectSocket : public DataSocket {
  struct sockaddr_in _host_addr;
  struct hostent *_host;

  public:
    ConnectSocket(const std::string& host, unsigned int port);
};

namespace ErrorLog {
  void error(const std::string& msg);
  void BaseSocketError(const std::string& msg);
  void DataSocketError(const std::string& msg);
  void HostSocketError(const std::string& msg);
  void ConnectSocketError(const std::string& msg);
}

#endif /* SOCKET_H */
