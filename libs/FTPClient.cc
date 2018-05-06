#include "FTPClient.h"
#include <sstream>

#define FTP_OPEN_PORT 21
#define MAX_FTP_ARGUMENT_SIZE_ALLOWED 500

using std::string;
using std::cout;

FTPClient::FTPClient(string const& host, int PORT = 49152) : ConnectSocket(host, FTP_OPEN_PORT) {
  _mode = ACTIVE;
  _PORT = PORT;
  cout << "Simple FTP client\n" << "";
  cout << "Connection established, waiting for welcome message...\n";
  string rcvMsg = receiveMessage();
  cout << rcvMsg;
}

unsigned short FTPClient::getResponseCode(string const& responseMessage) {
  std::stringstream ss(responseMessage);
  unsigned short responseCode;
  ss >> responseCode;
  return responseCode; 
}

FTPCommand FTPClient::getFTPCommand(string const& str) {
  if (str == "ls") return LS;
  if (str == "put") return PUT;
}

std::string FTPClient::send(string const& command, string const& argument) {
  if (argument.size() > MAX_FTP_ARGUMENT_SIZE_ALLOWED) {
    cout << "Invalid FTP command argument\n";
    exit(1);
  }
  sendMessage(command + " " + argument + "\r\n");
  string rcvMsg = receiveMessage();
  cout << rcvMsg;
  return rcvMsg;
}

void FTPClient::sendUsername(string const& username) {
  send("USER", username);
}

bool FTPClient::sendPassword(string const& password) {
  string responseMessage = send("PASS", password);
  auto responseCode = getResponseCode(responseMessage);

  if (responseCode != FTPResponseCode::LOGGED_ON) {
    return false;
  }
  return true;
}

bool FTPClient::sendCommand(string const& command) {
  string cmdStr, param;
  std::stringstream ss(command);
  ss >> cmdStr >> param;

  if (_mode == ACTIVE) {
    std::stringstream ss;
    ss << "EPRT |1|127.0.0.1|" << _PORT << "|\r\n";
    sendMessage(ss.str());
    // clear file descriptor that containing non-use reponse message after sending port command
    receiveMessage(); 
  }

  auto cmd = getFTPCommand(cmdStr);
  switch (cmd) {
    case LS: {
      std::thread t([this] {
        HostSocket host(_PORT);
        DataSocket ds = host.accept();
        string data = ds.receiveMessage();
        cout << data;
      });

      if (param.empty()) {
        sendMessage("LIST\r\n");
        string rcvMsg = receiveMessage();
        cout << rcvMsg; 
      } else {
        send("LIST", param);
      }

      t.join();
      string closeMsg = receiveMessage();
      cout << closeMsg;
  }
    break;

  case PUT: {

  }
    break;

  default:
    return false;
  }

  _PORT++; // increase port by one for later command request
  return true;
}


void FTPClient::createDataChannel(std::function<void(std::string const&)> fn) { // not implemented yet
  
}
