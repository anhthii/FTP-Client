#include "FTPClient.h"
#include <sstream>
#include <limits>
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
  if (str == "mput") return MPUT;
  if (str == "get") return GET;
  if (str == "mget") return MGET;
  if (str == "cd") return CD;
  if (str == "lcd") return LCD;
  if (str == "delete") return DELE;
  if (str == "mdelete") return MDELE;
  if (str == "mkdir") return MKDIR;
  if (str == "rmdir") return RMDIR;
  if (str == "pwd") return PWD;
  if (str == "exit" || str == "quit") return EXIT;
  if (str == "passive") return PASV;
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

std::unique_ptr<HostSocket> FTPClient::openPort() {
  auto host = std::make_unique<HostSocket>(_myAddr);
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

std::unique_ptr<ConnectSocket> FTPClient::initPassive() {
      sendMessage("PASV\r\n"); // todo handle error
      std::istringstream ss(receiveMessage());

      // todo write another parser
      ss.ignore(std::numeric_limits<std::streamsize>::max(), '(');
      char comma;
      unsigned int addr1, addr2, addr3, addr4;
      unsigned short port1, port2;
      ss >> addr1 >> comma
         >> addr2 >> comma
         >> addr3 >> comma
         >> addr4 >> comma
         >> port1 >> comma
         >> port2;

      std::string addr = 
        std::to_string(addr1) + '.' +
        std::to_string(addr2) + '.' +
        std::to_string(addr3) + '.' +
        std::to_string(addr4);
      return std::make_unique<ConnectSocket>(addr, port1 << 8 | port2);
}

void FTPClient::expandGlob(const std::string& file, std::vector<std::string>& files) {
  std::unique_ptr<HostSocket> host;
  std::unique_ptr<DataSocket> ds;
  if (_mode == ACTIVE) {
    host = openPort();
  } else {
    ds = initPassive();
  }

  send("NLST", file, false);

  if (_mode == ACTIVE) {
    ds = host->accept();
  }

  std::string data = ds->receiveData();
  if (data.empty()) {
    cout << "No such file: " << file << "\n";
  } else {
    std::stringstream ss(data);
    std::string line;
    while (std::getline(ss, line)) {
      line.erase(line.length() - 1);
      files.push_back(line);
    }
  }
  ds->close();

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
    // todo switch to ascii mode

    std::unique_ptr<HostSocket> host;
    std::unique_ptr<DataSocket> ds;
    if (_mode == ACTIVE) {
      host = openPort();
    } else {
      ds = initPassive();
    }

    if (param.empty()) {
      sendMessage("LIST\r\n");
      std::string rcvMsg = receiveMessage();
      cout << rcvMsg; 
    } else {
      send("LIST", param);
    }

    if (_mode == ACTIVE) {
      ds = host->accept();
    }

    std::string data = ds->receiveData();
    cout << data;
    
    std::string closeMsg = receiveMessage();
    cout << closeMsg;
  }
    break;

  case PUT: {
    std::unique_ptr<HostSocket> host;
    std::unique_ptr<DataSocket> ds;
    if (_mode == ACTIVE) {
      host = openPort();
    } else {
      ds = initPassive();
    }
        
    send("STOR", param);

    if (_mode == ACTIVE) {
      ds = host->accept();
    }

    if (!ds->sendFile(param)) {
      std::cerr << "Error sending file!\n";
    }
    ds->close();

    std::string rcvMsg = receiveMessage();
    cout << rcvMsg;
  }
    break;

  case MPUT: {
    std::vector<std::string> files;
    std::istringstream iss(param);
    for (std::string s; iss >> s; ) {
      files.push_back(s);
    }
    for(auto& file: files) {
      std::string yesOrNo;
      std::cout << "mput " << file << "?";
      std::getline(std::cin, yesOrNo);
      if (yesOrNo.front() == 'n' || yesOrNo.front() == 'N') {
        // ignore
      } else {
        sendCommand("put " +  file);
      }
    }
  }
    break;

  case GET: {
    std::unique_ptr<HostSocket> host;
    std::unique_ptr<DataSocket> ds;
    if (_mode == ACTIVE) {
      host = openPort();
    } else {
      ds = initPassive();
    }

    send("RETR", param);

    if (_mode == ACTIVE) {
      ds = host->accept();
    }

    if (!ds->receiveFile(param)) {
      std::cerr << "Error receiving file!\n";
    }
    ds->close();

    std::string rcvMsg = receiveMessage();
    cout << rcvMsg;
  }
    break;

  case MGET: {
    std::vector<std::string> files;
    std::istringstream iss(param);
    for (std::string s; iss >> s; ) {
      expandGlob(s, files); // todo handle ascii type
    }
    for(auto& file: files) {
      std::string yesOrNo;
      std::cout << "mget " << file << "?";
      std::getline(std::cin, yesOrNo);
      if (yesOrNo.front() == 'n' || yesOrNo.front() == 'N') {
        // ignore
      } else {
        sendCommand("get " + file);
      }
    }
  }
    break;

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
    break;

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
        send("DELE", file);
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

  case PASV: {
    if (_mode == ACTIVE) {
      _mode = PASSIVE;
      std::cout << "Passive mode on." << std::endl;
    } else {
      _mode = ACTIVE;
      std::cout << "Passive mode off." << std::endl;
    }
  }

  default:
    return false;
  }

  return true;
}

