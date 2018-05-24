#include "libs/FTPClient.h"
#include <string>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

int getch();
std::string getpass(const char* prompt, bool show_asterisk = true);

int main(int argc, char*argv[]) {
  std::string usage = "Usage: ftp [-d] [host]\n-d   Enables debugging.\n"; 
  if (argc < 2 || argc > 3) {
    std::cout << usage;
    exit(EXIT_FAILURE);
  }

  if (argc == 3 && std::strcmp(argv[1], "-d") != 0) {
    std::cout << usage;
    exit(EXIT_FAILURE);
  }

  if (argc == 2 && std::strcmp(argv[1], "-d") == 0) {
    std::cout << usage;
    exit(EXIT_FAILURE);
  }

  std::string host = argv[2] ? argv[2] : argv[1];
  FTPClient ftpClient(host);

  if (std::strcmp(argv[1], "-d") == 0) {
    ftpClient.debug(true);
  }

  std::string userName, password;
  std::cout << "Username: ";
  std::getline(std::cin, userName); 
  ftpClient.sendUsername(userName);
  password = getpass("Password: ", true); 
  bool resp = ftpClient.sendPassword(password);
  if (resp == false) {
    std::cout << "Login failed.\n";
    return EXIT_FAILURE;
  }

  while (1) {
    char* input = readline("ftp> ");

    // Check for EOF.
    if (!input)
        break;

    // Add input to history.
    add_history(input);

    std::string cmd(input);

    // Free input.
    free(input);

    if (cmd == "help") {
      FTPClient::printHelp();
      continue;
    }

    ftpClient.sendCommand(cmd);
  }
  return 0;
}

// Masking password input: http://www.cplusplus.com/articles/E6vU7k9E/
int getch() {
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

std::string getpass(const char* prompt, bool show_asterisk) {
  using std::cout;
  using std::endl;
  using std::string;

  const char BACKSPACE = 127;
  const char RETN = 10;

  string password;
  unsigned char ch = 0;

  cout << prompt;

  while ((ch=getch()) != RETN) {
    if (ch == BACKSPACE) {
      if (password.length() != 0) {
        if (show_asterisk)
          cout <<"\b \b";
        password.resize(password.length() - 1);
      }
    } else {
      password += ch;
      if (show_asterisk)
        cout << '*';
    }
  }
  cout << endl;
  return password;
}

