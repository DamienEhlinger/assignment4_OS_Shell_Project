#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>
#include <cstring>

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
    

    //Damien's code portion.

}