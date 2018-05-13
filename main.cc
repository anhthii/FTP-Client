#include "libs/FTPClient.h"
#include <string>
#include <iostream>

int main(int argc, char*argv[]) {
  (void) argc;

  FTPClient ftpClient("127.0.0.1");
  std::string userName, password;
  std::cout << "Username: ";
  //std::getline(std::cin, userName); 
  ftpClient.sendUsername("teddybear");
  std::cout << "Password: ";
  //std::getline(std::cin, password);
  bool resp = ftpClient.sendPassword("anaconda_456");
  if (resp == false) {
    std::cout << "Login failed.\n";
    return EXIT_FAILURE;
  }

  while (1) {
    std::string cmd;
    std::cout << "ftp> ";
    std::getline(std::cin, cmd);
   /*  if (!FTPClient::isValidCommand(cmd)) {
     std::cout << "Invalid command.\n";
      continue;
    } */
    ftpClient.sendCommand(cmd);
  }
  return 0;
}
