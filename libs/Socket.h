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
#include <memory>
#include <fcntl.h>

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
    const static int MAX_BUFF_SIZE = 512; 
    DataSocket(int socketFd) : BaseSocket(socketFd) {}
    void clearFd(); // clear file descriptor
    void sendMessage(const std::string& msg);
    std::string receiveMessage();
    std::string receiveData();
    bool sendFile(const std::string& file);
    bool receiveFile(const std::string& file);
};

class HostSocket : public BaseSocket {
  sockaddr_in _host_addr;
  unsigned short _port;
  uint32_t _addr;

  public:
    HostSocket(sockaddr_in myAddr, unsigned short port = 0);
    const unsigned short& getPort() { return _port; }
    const uint32_t& getAddr() {return _addr; }
    std::shared_ptr<DataSocket> accept();
};

class ConnectSocket : public DataSocket {
  sockaddr_in _host_addr;
  hostent* _host;

  protected:
    sockaddr_in _myAddr;
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
