#include "FTPClient.h"
#include <sstream>

#define MAX_FTP_ARGUMENT_SIZE_ALLOWED 500
#define MAX_PATH_SIZE 512

using std::string;
using std::cout;

FTPClient::FTPClient(const std::string& host, int port /* = FTP_OPEN_PORT */) : ConnectSocket(host, port) {
  _mode = ACTIVE;
  cout << "Simple FTP client\n" << "";
  cout << "Connection established, waiting for welcome message...\n";
  string rcvMsg = receiveMessage();
  cout << rcvMsg;
}

unsigned short FTPClient::getResponseCode(const std::string& responseMessage) {
  std::stringstream ss(responseMessage);
  unsigned short responseCode;
  ss >> responseCode;
  return responseCode; 
}

FTPCommand FTPClient::getFTPCommand(const std::string& str) {
  if (str == "ls") return LS;
  if (str == "put") return PUT;
  if (str == "cd") return CD;
  if (str == "lcd") return LCD;
  if (str == "delete") return DELE;
}

std::string FTPClient::send(const std::string& command, const std::string& argument) {
  if (argument.size() > MAX_FTP_ARGUMENT_SIZE_ALLOWED) {
    cout << "Invalid FTP command argument\n";
    exit(1);
  }
  //std::cout << command << " " << argument << "\r\n";
  sendMessage(command + " " + argument + "\r\n");
  string rcvMsg = receiveMessage();
  cout << rcvMsg;
  return rcvMsg;
}

void FTPClient::sendUsername(const std::string& username) {
  send("USER", username);
}

bool FTPClient::sendPassword(const std::string& password) {
  string responseMessage = send("PASS", password);
  auto responseCode = getResponseCode(responseMessage);

  if (responseCode != FTPResponseCode::LOGGED_ON) {
    return false;
  }
  return true;
}

bool FTPClient::sendCommand(const std::string& command) {
  string cmdStr, param;
  std::stringstream ss(command);
  ss >> cmdStr >> param;

  HostSocket host(_myAddr);
  const char* addr = (char*)&host.getAddr();
  const char* port = (char*)&host.getPort();

  auto cmd = getFTPCommand(cmdStr);
  if (_mode == ACTIVE &&
    (cmd == LS || cmd == PUT)
  ) {
    std::stringstream ss;
    ss << "PORT " 
       << ((int)addr[0] & 0xff) << ","
       << ((int)addr[1] & 0xff) << ","
       << ((int)addr[2] & 0xff) << ","
       << ((int)addr[3] & 0xff) << ","
       << ((int)port[0] & 0xff) << ","
       << ((int)port[1] & 0xff)
       << "\r\n";
    std::cout << ss.str();
    sendMessage(ss.str());
    // clear file descriptor that containing non-use reponse message after sending port command
    receiveMessage(); 
  }

  switch (cmd) {
  case LS: {
      std::thread t([&] {
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
      sendMessage("TYPE I\r\n"); // switch to binary mode
      string rcvMsg = receiveMessage();
      cout << rcvMsg; 
      std::thread t([&] {
        DataSocket ds = host.accept();
        if (!ds.sendFile(param)) {
          std::cerr << "Error sending file!\n";
        }

      });
      send("STOR", param);
      t.join();
      rcvMsg = receiveMessage();
      cout << rcvMsg;
  }
    break;
  
  case CD: {
    send("CWD", param);
  }
    break;

  case LCD: {
    string path = !param.empty() ? param : ".";
    if (chdir(path.c_str()) == 0) {
      cout << "Local directory now: ";
      char currPath[MAX_PATH_SIZE];
      getcwd(currPath, MAX_PATH_SIZE);
      cout << currPath << "\n";
    } else {
      ErrorLog::error("local: " + path);
    }
  }
    break;
  
  case DELE: {
    send("DELE", param);
  }
    break;

  default:
    return false;
  }

  return true;
}


void FTPClient::createDataChannel(std::function<void(const std::string&)> fn) { // not implemented yet
  
}
