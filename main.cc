#include "libs/FTPClient.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>

int getch();
std::string getpass(const char* prompt, bool show_asterisk = true);
void printUsage();

#define FTP_COMMAND_PORT 21
#define VERSION "1.0"

int main(int argc, char*argv[]) {
  std::string host;
  unsigned short port;

  servent* srv = getservbyname("ftp", "tcp");
  if (srv) {
    port = ntohs(srv->s_port);
  } else {
    port = FTP_COMMAND_PORT;
  }

  bool debug = false, passive = false;
  int choice;
  while (1) {
    static struct option long_options[] =
    {
      /* Use flags like so:
      {"verbose",	no_argument,	&verbose_flag, 'V'}*/
      /* Argument styles: no_argument, required_argument, optional_argument */
      {"debug",	no_argument,	0,	'd'},
      {"passive",	no_argument,	0,	'p'},
      {"version", no_argument,	0,	'v'},
      {"help",	no_argument,	0,	'h'},
      
      {0,0,0,0}
    };
  
    int option_index = 0;
  
    /* Argument parameters:
      no_argument: " "
      required_argument: ":"
      optional_argument: "::" */
  
    choice = getopt_long( argc, argv, "dpvh",
          long_options, &option_index);
  
    if (choice == -1) {
      break;
    }
  
    switch(choice) {
      case 'd':
        debug = true;
        break;

      case 'p':
        passive = true;
        break;

      case 'v':
        std::cout << "FTP Client v" << VERSION << std::endl;
        return EXIT_SUCCESS;
  
      case 'h':
        printUsage();
        return EXIT_SUCCESS;
  
      case '?':
        /* getopt_long will have already printed an error */
        return EXIT_FAILURE;
  
      default:
        /* Not sure how to get here... */
        return EXIT_FAILURE;
    }
  }

  if ( optind < argc ) {
      host = argv[optind++];
      if (optind < argc) {
        std::stringstream ss(argv[optind]);
        ss >> port;
      }
  } else {
    std::cerr << "No host name!" << std::endl;
    return EXIT_FAILURE;
  }

  FTPClient ftpClient(host, port);

  ftpClient.debug(debug);
  ftpClient.passive(passive);

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
        password.pop_back();
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

void printUsage() {
  std::cout << "Usage: ftp [OPTION]... HOST [PORT]\n"
            << "Options:\n"
            << std::left
            << std::setw(20) << "\t-d, --debug " << "enable debugging output\n"
            << std::setw(20) << "\t-p, --passive " << "enable passive mode transfer\n"
            << std::setw(20) << "\t-v, --version " << "print program version\n"
            << std::setw(20) << "\t-h, --help " << "show this help"
            << std::endl;
}
