#include "FTPClient.h"
#include <sstream>
#include <vector>
#define MAX_FTP_ARGUMENT_SIZE_ALLOWED 500
#define MAX_PATH_SIZE 256

using std::string;
using std::cout;

FTPClient::FTPClient(const std::string& host, int port /* = FTP_OPEN_PORT */) : ConnectSocket(host, port) {
  _mode = ACTIVE;
  cout << "Simple FTP client\n" << "";
  cout << "Connection established, waiting for welcome message...\n";
  std::string rcvMsg = receiveMessage();
  cout << rcvMsg;
}

unsigned short FTPClient::getResponseCode(const std::string& responseMessage) {
  std::stringstream ss(responseMessage);
  unsigned short responseCode;
  ss >> responseCode;
  return responseCode; 
}

FTPCommand FTPClient::getFTPCommand(const std::string& str) {
  if (str == "ls" || str == "dir") return LS;
  if (str == "put") return PUT;
  if (str == "get") return GET;
  if (str == "cd") return CD;
  if (str == "lcd") return LCD;
  if (str == "delete") return DELE;
  if (str == "mdelete") return MDELE;
  if (str == "mkdir") return MKDIR;
  if (str == "rmdir") return RMDIR;
  if (str == "pwd") return PWD;
  if (str == "exit" || str == "quit") return EXIT;
}

std::string FTPClient::send(const std::string& command, const std::string& argument, bool printResponse) {
  if (argument.size() > MAX_FTP_ARGUMENT_SIZE_ALLOWED) {
    cout << "Invalid FTP command argument\n";
    exit(1);
  }
  std::cout << command << " " << argument << "\r\n";
  sendMessage(command + " " + argument + "\r\n");
  std::string rcvMsg = receiveMessage();
  if (printResponse == true) {
    cout << rcvMsg;
  }
  return rcvMsg;
}

void FTPClient::sendUsername(const std::string& username) {
  send("USER", username);
}

bool FTPClient::sendPassword(const std::string& password) {
  std::string responseMessage = send("PASS", password);
  auto responseCode = getResponseCode(responseMessage);

  if (responseCode != FTPResponseCode::LOGGED_ON) {
    return false;
  }
  return true;
}

std::shared_ptr<HostSocket> FTPClient::openPort() {
  auto host = std::make_shared<HostSocket>(_myAddr);
  const char* addr = (char*)&host->getAddr();
  const char* port = (char*)&host->getPort();
  std::stringstream ss;
  ss << "PORT " 
      << ((int)addr[0] & 0xff) << ","
      << ((int)addr[1] & 0xff) << ","
      << ((int)addr[2] & 0xff) << ","
      << ((int)addr[3] & 0xff) << ","
      << ((int)port[0] & 0xff) << ","
      << ((int)port[1] & 0xff)
      << "\r\n";
  // std::cout << ss.str();
  sendMessage(ss.str());
  // clear file descriptor that containing non-use reponse message after sending port command
  DataSocket::clearFd();
  return host;
}

void FTPClient::expandGlob(const std::string& file, std::vector<std::string>& files) {
  auto host = openPort();
  std::thread t([&] {
    DataSocket ds = host->accept();
    std::string data = ds.receiveMessage();
    if (data.empty()) {
      cout << "No such file: " << file << "\n";
    } else {
      std::stringstream ss(data);
      std::string line;
      while (std::getline(ss, line)) {
        files.push_back(line);
      }
    }
  });
  send("NLST", file, false);
  t.join();
  
  std::cout << receiveMessage();
}

bool FTPClient::sendCommand(const std::string& command) {
  std::string cmdStr, param;
  std::stringstream ss(command);
  ss >> cmdStr >> std::ws;
  std::getline(ss, param);
  
  auto cmd = getFTPCommand(cmdStr);
  switch (cmd) {
  case LS: {
    auto host = openPort();
    std::thread t([&] {
      DataSocket ds = host->accept();
      std::string data = ds.receiveMessage();
      cout << data;
    });

    if (param.empty()) {
      sendMessage("LIST\r\n");
      std::string rcvMsg = receiveMessage();
      cout << rcvMsg; 
    } else {
      send("LIST", param);
    }

    t.join();
    std::string closeMsg = receiveMessage();
    cout << closeMsg;
  }
    break;

  case PUT: {
    /* sendMessage("TYPE I\r\n"); // switch to binary mode */
    /* std::string rcvMsg = receiveMessage(); */
    /* cout << rcvMsg; */ 
    auto host = openPort();
    std::thread t([&] {
      DataSocket ds = host->accept();
      if (!ds.sendFile(param)) {
        std::cerr << "Error sending file!\n";
      }
        
    });
    send("STOR", param);
    // check for error before join
    t.join();
    std::string rcvMsg = receiveMessage();
    cout << rcvMsg;
  }
    break;

  case GET: {
    auto host = openPort();
    std::thread t([&] {
      DataSocket ds = host->accept();
      if (!ds.receiveFile(param)) {
        std::cerr << "Error receiving file!\n";
      }
        
    });
    send("RETR", param);
    // check for error before join
    t.join();
    std::string rcvMsg = receiveMessage();
    cout << rcvMsg;
  }

  case CD: {
    send("CWD", param);
  }
    break;

  case LCD: {
    std::string path = !param.empty() ? param : ".";
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

  case MDELE: {
    // splitting std::string by space
    std::vector<std::string> files;
    std::istringstream iss(param);
    for (std::string s; iss >> s; ) {
      expandGlob(s, files);
    }
    for(auto& file: files) {
      std::string yesOrNo;
      cout << "mdelete " << file << "?";
      std::getline(std::cin, yesOrNo);
      if (yesOrNo.front() == 'n' || yesOrNo.front() == 'N') {
        // ignore
      } else {
        send("DELE", file, true);
      }
    }
  }

    break;

  case MKDIR: {
    send("MKD", param);
  }
    break;

  case RMDIR: {
    send("RMD", param);
  }
    break;

  case PWD: {
    send("PWD");
  }
    break;

  case EXIT: {
    send("QUIT");
    exit(0);
  }
    break;

  default:
    return false;
  }

  return true;
}

void FTPClient::createDataChannel(std::function<void(const std::string&)> fn) { // not implemented yet
  
}

