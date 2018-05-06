#include "Socket.h"
// ============================= Base Socket ===========================================
BaseSocket::BaseSocket(int socketFd) : socketFd(socketFd) {
  if (socketFd == invalidSocketFd) {
    ErrorLog::BaseSocketError("Constructor, Error on initializing socket file descriptor");
  }
}

BaseSocket::~BaseSocket() {
  close();
}

void BaseSocket::close() {
  if (::close(socketFd) < 0) {
    ErrorLog::BaseSocketError("Error on closing socket file descriptor");
  }
}

// ============================= Data Socket ===========================================
std::string DataSocket::receiveMessage() {
  char recvMsg[MAX_BUFF_SIZE];
  ::memset(recvMsg, 0, MAX_BUFF_SIZE);
  std::size_t n;
  n = ::read(getSocketFd(), recvMsg, MAX_BUFF_SIZE);
  if (n < 0)
    ErrorLog::DataSocketError("error occuring when receiving data");
  return std::string(recvMsg);
}

void DataSocket::sendMessage(std::string const& msg) {
  char const* buffer = msg.c_str();
  std::size_t size = msg.size();
  std::size_t dataWritten = 0;

  while(dataWritten < size) {
    std::size_t put = ::write(getSocketFd(), buffer + dataWritten, size - dataWritten);

    if (put == static_cast<std::size_t>(-1)) {
      switch(errno) {
        case EPIPE:
        {
            // fatal error
          ErrorLog::DataSocketError("error occuring when sending data");
        }
        case EAGAIN:
        {
            // temporary error, retry writing data
            continue;
        }
      }
    }

    dataWritten += put;
  }  
}

// ============================= Host Socket ===========================================
HostSocket::HostSocket(unsigned int port) : BaseSocket(::socket(AF_INET, SOCK_STREAM, 0)) {
  const int MAX_CONNECTIONS_ALLOWED = 1; 
  ::memset((char*) &_host_addr, 0, sizeof(_host_addr));
  _host_addr.sin_family = AF_INET;
  _host_addr.sin_addr.s_addr = INADDR_ANY;
  _host_addr.sin_port = htons(port);

  if (::bind(getSocketFd(), (struct sockaddr *) &_host_addr, sizeof(_host_addr)) != 0) {  
    ErrorLog::HostSocketError("Error on binding socket");
  }

  if (::listen(getSocketFd(), MAX_CONNECTIONS_ALLOWED) != 0) {
    ErrorLog::HostSocketError("Error on listening");
  }
}

DataSocket HostSocket::accept() {
  struct sockaddr_storage serverStorage;
  socklen_t addr_size = sizeof serverStorage;
  int newSocket = ::accept(getSocketFd(), (struct sockaddr*)&serverStorage, &addr_size);
  if (newSocket == -1) {
    ErrorLog::HostSocketError("Error on accepting");
  }
  return DataSocket(newSocket);
}

// ============================= Connect Socket ===========================================
ConnectSocket::ConnectSocket(std::string const& host, unsigned int port) : DataSocket(::socket(AF_INET, SOCK_STREAM, 0)) {
  _host = ::gethostbyname(host.c_str());
  if (_host == NULL) {
    errno = ECONNREFUSED;
    ErrorLog::ConnectSocketError("No such host");
  }

  ::memset((char*) &_host_addr, 0, sizeof(_host_addr));
  _host_addr.sin_family = AF_INET;
  ::memcpy((char*) &_host_addr.sin_addr.s_addr, (char*) _host->h_addr , _host->h_length);
  _host_addr.sin_port = htons(port);

  if (::connect(getSocketFd(), (struct sockaddr*) &_host_addr, sizeof(_host_addr)) != 0) {
    ErrorLog::ConnectSocketError("Can't connect to host");
  }
}

// ============================= Log ===========================================
using namespace ErrorLog;

void ErrorLog::error(std::string const& msg) {
  std::cout << msg << ": " << std::strerror(errno) << '\n';
  exit(EXIT_FAILURE);
}

void ErrorLog::BaseSocketError(std::string const& msg) {
  error("[BaseSocket] " + msg);
}

void ErrorLog::DataSocketError(std::string const& msg) {
  error("[DataSocket] " + msg);
}

void ErrorLog::HostSocketError(std::string const& msg) {
   error("[HostSocket] " + msg);
}

void ErrorLog::ConnectSocketError(std::string const& msg) {
   error("[ConnectSocket] " + msg);
}
