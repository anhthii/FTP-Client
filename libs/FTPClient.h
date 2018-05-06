#ifndef _FTPCLIENT
#define _FTPCLIENT
#include "Socket.h"
#include <string>
#include <iostream>
#include <thread>
#include <functional>

enum FTPResponseCode {
  LOGGED_ON = 230,
  SUCCESSFULLY_TRANSFERRED = 226,
};

enum Mode {
  ACTIVE,
  PASSIVE,
};

enum FTPCommand {
  LS,
  PUT,
};

class FTPClient: public ConnectSocket {
  std::string send(std::string const& command, std::string const& argument);
  unsigned short getResponseCode(std::string const& responseMessage);
  FTPCommand getFTPCommand(std::string const& str);
  int _PORT;
  Mode _mode;

  public:
    bool static isValidCommand();
    void static createDataChannel(std::function<void(std::string const&)> fn); 
    FTPClient(std::string const& host, int _PORT);
    void sendUsername(std::string const& username);
    bool sendPassword(std::string const& password);
    bool sendCommand(std::string const& command);
};

#endif
