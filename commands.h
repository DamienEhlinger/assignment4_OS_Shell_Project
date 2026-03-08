#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <algorithm>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cctype>
#include <dirent.h>
#include <sys/types.h>
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

    size_t pos = command.find('|');

    string left = command.substr(0, pos);
    string right = command.substr(pos + 1);

    // tokenize left command
    stringstream ss1(left);
    vector<string> tokens1;
    string temp;

    while (ss1 >> temp)
        tokens1.push_back(temp);

    // tokenize right command
    stringstream ss2(right);
    vector<string> tokens2;

    while (ss2 >> temp)
        tokens2.push_back(temp);

    // convert to char* arrays
    vector<char*> args1;
    vector<char*> args2;

    for (auto &t : tokens1)
        args1.push_back((char*)t.c_str());
    args1.push_back(nullptr);

    for (auto &t : tokens2)
        args2.push_back((char*)t.c_str());
    args2.push_back(nullptr);

    int fd[2];
    pipe(fd);

    pid_t pid1 = fork();

    if (pid1 == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        execvp(args1[0], args1.data());
        perror("execvp failed");
        exit(1);
    }

    waitpid(pid1, NULL, 0);

    pid_t pid2 = fork();

    if (pid2 == 0) {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        execvp(args2[0], args2.data());
        perror("execvp failed");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid2, NULL, 0);
}


void inputRirection(string command){

}

void outputRirection(string command) {

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

    size_t pos = command.find('|');

    string left = command.substr(0, pos);
    string right = command.substr(pos + 1);

    // tokenize left command
    stringstream ss1(left);
    vector<string> tokens1;
    string temp;

    while (ss1 >> temp)
        tokens1.push_back(temp);

    // tokenize right command
    stringstream ss2(right);
    vector<string> tokens2;

    while (ss2 >> temp)
        tokens2.push_back(temp);

    // convert to char* arrays
    vector<char*> args1;
    vector<char*> args2;

    for (auto &t : tokens1)
        args1.push_back((char*)t.c_str());
    args1.push_back(nullptr);

    for (auto &t : tokens2)
        args2.push_back((char*)t.c_str());
    args2.push_back(nullptr);

    int fd[2];
    pipe(fd);

    pid_t pid1 = fork();

    if (pid1 == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        execvp(args1[0], args1.data());
        perror("execvp failed");
        exit(1);
    }

    waitpid(pid1, NULL, 0);

    pid_t pid2 = fork();

    if (pid2 == 0) {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        execvp(args2[0], args2.data());
        perror("execvp failed");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid2, NULL, 0);
}


void inputRirection(string command){

}

void outputRirection(string command) {

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