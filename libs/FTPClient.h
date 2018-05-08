#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include "Socket.h"
#include <string>
#include <iostream>
#include <thread>
#include <functional>

#define FTP_OPEN_PORT 21

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
  Mode _mode;

  public:
    bool static isValidCommand();
    void static createDataChannel(std::function<void(const std::string&)> fn); 
    FTPClient(const std::string& host, int port = FTP_OPEN_PORT);
    void sendUsername(const std::string& username);
    bool sendPassword(const std::string& password);
    bool sendCommand(const std::string& command);
};

#endif
