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
  std::string send(const std::string& command, const std::string& argument);
  unsigned short getResponseCode(const std::string& responseMessage);
  FTPCommand getFTPCommand(const std::string& str);
  int _PORT;
  Mode _mode;

  public:
    bool static isValidCommand();
    void static createDataChannel(std::function<void(const std::string&)> fn); 
    FTPClient(const std::string& host, int _PORT);
    void sendUsername(const std::string& username);
    bool sendPassword(const std::string& password);
    bool sendCommand(const std::string& command);
};

#endif
