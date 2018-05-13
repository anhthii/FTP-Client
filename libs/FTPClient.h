#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include "Socket.h"
#include <string>
#include <iostream>
#include <thread>
#include <functional>
#include <memory>
#include <vector>

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
  GET,
  CD,
  LCD,
  DELE,
  MDELE,
  MKDIR,
  RMDIR,
  PWD,
  EXIT
};

class FTPClient: public ConnectSocket {
  std::string send(const std::string& command, const std::string& argument = "", bool printResponse = true);
  unsigned short getResponseCode(const std::string& responseMessage);
  FTPCommand getFTPCommand(const std::string& str);
  std::shared_ptr<HostSocket> openPort();
  Mode _mode;

  public:
    bool static isValidCommand();
    void static createDataChannel(std::function<void(const std::string&)> fn); 
    FTPClient(const std::string& host, int port = FTP_OPEN_PORT);
    void sendUsername(const std::string& username);
    bool sendPassword(const std::string& password);
    bool sendCommand(const std::string& command);
    void expandGlob(const std::string& file, std::vector<std::string>& files);
};

#endif

