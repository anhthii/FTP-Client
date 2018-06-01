#include "FTPClient.h"
#include <sstream>
#include <limits>
#include <vector>
#include <libgen.h>
#include <wordexp.h>
#include <sys/stat.h>

#define MAX_FTP_ARGUMENT_SIZE_ALLOWED 500
#define MAX_PATH_SIZE 256

using std::string;
using std::cout;

FTPClient::FTPClient(const std::string& host, int port) : ConnectSocket(host, port) {
  _debug = false;
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
  return INVALID_COMMAND;
}

unsigned short FTPClient::send(const std::string& command, const std::string& argument, bool printResponse) {
  if (argument.size() > MAX_FTP_ARGUMENT_SIZE_ALLOWED) {
    cout << "Invalid FTP command argument\n";
    exit(1);
  }
  if (_debug == true) {
    std::cout << "---> " << command << " " << argument << "\r\n";
  }

  sendMessage(command + " " + argument + "\r\n");
  std::string rcvMsg = receiveMessage();
  if (printResponse == true) {
    cout << rcvMsg;
  }
  return getResponseCode(rcvMsg);
}

void FTPClient::sendUsername(const std::string& username) {
  send("USER", username);
}

bool FTPClient::sendPassword(const std::string& password) {
  auto responseCode = send("PASS", password);
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
  if (_debug) {
    std::cout << "---> " << ss.str();
    std::cout << receiveMessage();
  } else {
    // clear file descriptor that containing non-use reponse message after sending port command
    DataSocket::clearFd();
  }
  return host;
}

std::unique_ptr<ConnectSocket> FTPClient::initPassive() {
      sendMessage("PASV\r\n");
      if (_debug) {
        std::cout << "---> PASV" << std::endl;
      }
      std::string rcvMsg = receiveMessage();
      if (_debug) {
        std::cout << rcvMsg;
      }
      auto responseCode = getResponseCode(rcvMsg);
      if (responseCode != FTPResponseCode::ENTERING_PASSIVE_MODE) {
        std::cout << "Passive mode refused." << std::endl;
        return nullptr;
      }
      std::istringstream ss(rcvMsg);

      ss >> responseCode;
      while (!std::isdigit(ss.peek())) {
        ss.get();
      }
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
  std::shared_ptr<DataSocket> ds;
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
      line.pop_back(); // remove \r
      files.push_back(line);
    }
  }
  ds->close();

  std::cout << receiveMessage();
}

void FTPClient::createDataChannel(const std::string& command, const std::string& param, std::function<void (std::shared_ptr<DataSocket>)> callback) {
  std::unique_ptr<HostSocket> host;
  std::shared_ptr<DataSocket> ds;
  if (_mode == ACTIVE) {
    host = openPort();
  } else {
    ds = initPassive();
    if (!ds) return;
  }

  auto responseCode = send(command, param);
  // ignore if responsecode != 250
  if (responseCode == FTPResponseCode::OPENING_DATA_CHANNEL) {
    if (_mode == ACTIVE) {
      ds = host->accept();
    }

    callback(ds);

    std::string rcvMsg = receiveMessage();
    cout << rcvMsg;
  }
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
    createDataChannel("LIST", param, [&param](std::shared_ptr<DataSocket> ds) {
      std::string data = ds->receiveData();
      cout << data;
    });
  }
    break;

  case PUT: {
    std::string pathToFile = param;
    char* s = strdup(param.c_str());
    char* filename = basename(s); // get filename from path

    if (access(pathToFile.c_str(), R_OK) != -1) {
      // we are only allowed to send filename not the whole path
      createDataChannel("STOR", filename, [&](std::shared_ptr<DataSocket> ds) {
        if (!ds->sendFile(pathToFile)) {
          std::cerr << "Error sending file!\n";
        }
        ds->close();
      });

    } else {
      std::cout << param << ": File not found\n";
    }
  }
    break;

  case MPUT: {
    std::vector<std::string> files;
    std::istringstream iss(param);
    
    wordexp_t result;
    switch (wordexp(param.c_str(), &result, 0)) {
      case 0:
        break;
      case WRDE_NOSPACE:
        std::cerr << "Error allocating memory to expand " << param << "!\n";
        wordfree(&result);
        return false;

      default:
        std::cerr << "Error expanding " << param << "!\n";
        return false;
    }

    for (std::size_t i = 0; i < result.we_wordc; ++i) {
      struct stat sb;
      if (stat(result.we_wordv[i], &sb) == -1) {
        continue;
      }
      if (S_ISREG(sb.st_mode)) {
        files.push_back(result.we_wordv[i]);
      }
    }
    wordfree(&result);

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
    std::string pathToFile = param;
    char* s = strdup(param.c_str());
    char* filename = basename(s); // get filename from path
    createDataChannel("RETR", pathToFile, [&](std::shared_ptr<DataSocket> ds) {
      if (!ds->receiveFile(filename)) {
        std::cerr << "Error receiving file!\n";
      }
      ds->close();
    });
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
    passive(_mode == ACTIVE);
    if (_mode == ACTIVE) {
      std::cout << "Passive mode off." << std::endl;
    } else {
      std::cout << "Passive mode on." << std::endl;
    }
  }
    break;

  case INVALID_COMMAND:
  default:
    cout << "Invalid command." << std::endl;
    return false;
  }

  return true;
}

void FTPClient::printHelp(){
	string cmd_desc = "";
  cmd_desc += "passive                      : switch to passive mode.\n";
	cmd_desc += "put     [sourcepath]filename : upload file(relative or absolute address) to the server.\n";
	cmd_desc += "get     [sourcepath]filename : download file(relative or absolute address) from the server.\n" ;
  cmd_desc += "delete  [sourcepath]filename : delete file(relative or absolute address) from the server.\n" ;
  cmd_desc += "mput    [file1 file2...]     : upload multiple files to the server.\n";
	cmd_desc += "mget    [file1 file2...]     : download multiple files from the server.\n" ;
  cmd_desc += "mdelete [file1 file2...]     : delete file from the server.\n" ;
	cmd_desc += "ls|dir  [sourcepath]         : list all files in sourcepath folder\n" ;
	cmd_desc += "cd      [destination]        : change working directory of the server to destination.\n" ;
  cmd_desc += "lcd     [destination]        : change local working directory.\n" ;
  cmd_desc += "mkdir   [sourcepath]         : make directory on the server.\n" ;
  cmd_desc += "rmdir   [sourcepath]         : remove directory on the server.\n" ;
	cmd_desc += "pwd                          : display working directory of the server.\n" ;
	cmd_desc += "quit|exit                    : quit from ftp session and return to Unix prompt.\n";
	std::cout << cmd_desc;
}

