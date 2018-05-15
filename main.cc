#include "libs/FTPClient.h"
#include <string>
#include <iostream>

int main(int argc, char*argv[]) {
  if (argc < 2) {
    std::cout << "Host is not specifed." << std::endl;
    std::cout << "Usage: ftp <host>" << std::endl;
    exit(EXIT_FAILURE);
  }
  FTPClient ftpClient(argv[1]);
  std::string userName, password;
  std::cout << "Username: ";
  std::getline(std::cin, userName); 
  ftpClient.sendUsername(userName);
  std::cout << "Password: ";
  std::getline(std::cin, password);
  bool resp = ftpClient.sendPassword(password);
  if (resp == false) {
    std::cout << "Login failed.\n";
    return EXIT_FAILURE;
  }

  while (1) {
    std::string cmd;
    std::cout << "ftp> ";
    std::getline(std::cin, cmd);
    if (cmd == "help") {
      FTPClient::printHelp();
      continue;
    }

    ftpClient.sendCommand(cmd);
  }
  return 0;
}
