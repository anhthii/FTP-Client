#include "FTPClient.h"
#include <string>
#include <iostream>

int main(int argc, char*argv[]) {
  // temporary code for development purpose
  if(argv[1][0] == '\0') {
    std::cout << "Port is not specified\n";
    std::cout << "Usage: ftp <PORT>\n";
    exit(1);
  }
  int PORT = std::stoi(argv[1]); // port for data channel
  //

  FTPClient ftpClient("127.0.0.1", PORT);
  std::string userName, password;
  std::cout << "Username: ";
  std::cin >> userName; 
  ftpClient.sendUsername(userName);
  std::cout << "Password: ";
  std::cin >> password;
  bool resp = ftpClient.sendPassword(password);
  if (resp == false) {
    std::cout << "Login failed.\n";
    exit(1);
  }

  while(1) {
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
