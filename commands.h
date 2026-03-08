#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cctype>
#include <dirent.h>
#include <sys/types.h>
#include <sstream>

using namespace std;

/**
 * return the current working directory path as a string
 * if there is an error, throw a runtime_error on failed to get current working directory
 * @return the current working directory path as a string
 * @throws runtime_error if there is an error getting the current working directory
 */
string pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return string(cwd);
    }
    throw runtime_error("Error getting current working directory");
}

/**
 * add user's command to history 
 * if cannot access history.txt file, print an error message
 * @param command the command to add to history
 */
void addToHist(string command) {

    //create a child process to handle create and write to history
    pid_t child = fork();
    if (child == 0 ) { //CHILD PROCESS  

        //open history.txt file
        int fd = open("history.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) cerr << "Error opening history file" << endl;
        else {

            //append command to history.txt file content
            write(fd, command.c_str(), command.size());

            //append a new line after each command
            write(fd, "\n", 1);
            close(fd);
        }
        exit(0);
    } else if (child > 0) { //PARENT PROCESS
        wait(NULL);
    } else cerr << "Error creating child process" << endl; 
}

/**
 * print history of commands
 * if cannot access history, print an error message
 */
void printHist() {
    //create a child process to handle open and read history file
    pid_t child = fork();
    if (child == 0) { //CHILD PROCESS

            //open history.txt
            int fd = open("history.txt", O_RDONLY);
        if (fd < 0) {
            cerr << "Error opening history file" << endl;
        } else {

            //read history.txt in chunk of 1024 bytes and after each read, terminate the string by adding '\0'
            //to prevent printing garbage
            char buffer[1024];
            ssize_t bytesRead;
            while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytesRead] = '\0';
                cout << buffer;
            }
            close(fd);
        }
        exit(0);
    } else if (child > 0) { //PARENT PROCESS
        wait(NULL);
    } else {
        cerr << "Error creating child process" << endl;
    }
}

/**
 * clear the terminal 
 */
void clear() {
    //'\033[2J' clears the screen, '\033[1;1H' moves the cursor to the top-left corner.
    //These are ANSI escape codes
    cout << "\033[2J\033[1;1H";
}

/**
 * run other commands using excvp
 */
void runCommand(string command) {

    //split command string into a list of string of character 
    vector<string> tokens;
    string token;
    istringstream iss(command);
    while (iss >> token) tokens.push_back(token);
    if (tokens.empty()) return; 

    //convert list of string of character to list of char* for execvp
    char *args[100];
    for (size_t i = 0; i < tokens.size(); ++i) {
        args[i] = (char*) tokens[i].c_str();
    }
    //add a nullptr at the end of args to indicate the end of arguments for execvp
    args[tokens.size()] = nullptr;

    //create a child process to handle user command
    pid_t pid = fork();
    if (pid == 0) { //CHILD PROCESS
        if (execvp(args[0], args) < 0) {

            //print error message if execvp fails and exit with error code
            perror("Error executing command");
            exit(errno);
        }
    } else if(pid > 0) { //PARENT PROCESS
        wait(NULL);
    } else cerr << "Error creating child process" << endl;
    
}

void ls(vector<string> args = {}) {

    pid_t pid = fork();
    if (pid == 0) {
        vector<char*> cargs;
        cargs.push_back((char*)"ls");
        for (auto &arg : args)
            cargs.push_back((char*)arg.c_str());
        cargs.push_back(nullptr);
        execvp("ls", cargs.data());
        perror("execvp failed");
        exit(1);
    }
    else if (pid > 0) {
        wait(nullptr);
    }
    else {
        perror("fork failed");
    }
}

void runProgram(string command) {
    // split by '|'
    vector<string> parts;
    {
        string part;
        stringstream s(command);
        while (std::getline(s, part, '|')) {
            // trim leading/trailing spaces
            auto l = part.find_first_not_of(" \t\n\r");
            if (l == string::npos) part = "";
            else {
                auto r = part.find_last_not_of(" \t\n\r");
                part = part.substr(l, r - l + 1);
            }
            if (!part.empty()) parts.push_back(part);
        }
    }

    if (parts.empty()) return;

    size_t n = parts.size();

    // tokenize each part
    vector<vector<string>> tokens(n);
    for (size_t i = 0; i < n; ++i) {
        stringstream ss(parts[i]);
        string t;
        while (ss >> t) tokens[i].push_back(t);
    }

    // prepare args pointers (they point into tokens strings, which stay alive)
    vector<vector<char*>> args(n);
    for (size_t i = 0; i < n; ++i) {
        for (auto &s : tokens[i]) args[i].push_back((char*)s.c_str());
        args[i].push_back(nullptr);
    }

    // create pipes: we need n-1 pipes, each with 2 fds
    vector<int> pipefds;
    pipefds.resize((n > 0 ? n - 1 : 0) * 2);
    for (size_t i = 0; i + 1 < n; ++i) {
        if (pipe(&pipefds[i * 2]) < 0) {
            perror("pipe");
            return;
        }
    }

    vector<pid_t> pids;
    pids.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // close fds
            for (int fd : pipefds) close(fd);
            return;
        }

        if (pid == 0) {
            // child
            // if not first, set stdin to read end of previous pipe
            if (i > 0) {
                int read_fd = pipefds[(i - 1) * 2];
                if (dup2(read_fd, STDIN_FILENO) < 0) { perror("dup2"); exit(1); }
            }
            // if not last, set stdout to write end of this pipe
            if (i + 1 < n) {
                int write_fd = pipefds[i * 2 + 1];
                if (dup2(write_fd, STDOUT_FILENO) < 0) { perror("dup2"); exit(1); }
            }

            // close all pipe fds in child
            for (int fd : pipefds) close(fd);

            // exec
            execvp(args[i][0], args[i].data());
            perror("execvp failed");
            exit(1);
        }

        // parent
        pids.push_back(pid);
    }

    // parent closes all pipe fds
    for (int fd : pipefds) close(fd);

    // wait for all children
    for (pid_t pid : pids) waitpid(pid, NULL, 0);
}
