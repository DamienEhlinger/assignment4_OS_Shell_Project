#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <dirent.h>
#include <sys/types.h>
#include "helpers.h"

using namespace std;

/**
 * return the current working directory path as a string
 * if there is an error, throw a runtime_error on failed to get current working directory
 * @author Simon Cao - 03/06/2026
 * @author Damien Ehlinger - 03/08/2026
 * @return the current working directory path as a string
 * @throws runtime_error if there is an error getting the current working directory
 */
string pwd()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
    {
        return string(cwd);
    }
    throw runtime_error("Error getting current working directory");
}

/**
 * add user's command to history
 * if cannot access history.txt file, print an error message
 * @author Simon Cao - 03/06/2026
 * @author Damian Ehlinger - 03/08/2026
 * @param command the command to add to history
 */
void addToHist(string command)
{
    // skip blank or whitespace-only commands
    bool onlyWhitespace = true;
    for (char ch : command)
    {
        if (!isspace(static_cast<unsigned char>(ch)))
        {
            onlyWhitespace = false;
            break;
        }
    }
    if (onlyWhitespace)
        return;

    // create a child process to handle create and write to history
    pid_t child = fork();
    if (child == 0)
    { // CHILD PROCESS

        // open history.txt file
        int fd = open("history.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) cerr << "Error opening history file" << endl;
        else
        {

            // append command to history.txt file content
            write(fd, command.c_str(), command.size());

            // append a new line after each command
            write(fd, "\n", 1);
            close(fd);
        }
        exit(0);
    }
    else if (child > 0)
    { // PARENT PROCESS
        wait(NULL);
    }
    else
        cerr << "Error creating child process" << endl;
}

/**
 * print history of commands
 * if cannot access history, print an error message
 * @author Simon Cao - 03/06/2026
 * @author Damien Ehlinger - 03/08/2026
 */
void printHist()
{
    // create a child process to handle open and read history file
    pid_t child = fork();
    if (child == 0)
    { // CHILD PROCESS

        // open history.txt
        int fd = open("history.txt", O_RDONLY);
        if (fd < 0)
        {
            cerr << "Error opening history file" << endl;
        }
        else
        {
            // read whole history file, then print each command with a number
            string content;
            char buffer[1024];
            ssize_t bytesRead;
            while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
            {
                content.append(buffer, bytesRead);
            }

            istringstream iss(content);
            string line;
            int index = 1;
            while (getline(iss, line))
            {
                if (line.empty())
                    continue;
                cout << index++ << " " << line << endl;
            }
            close(fd);
        }
        exit(0);
    }
    else if (child > 0)
    { // PARENT PROCESS
        wait(NULL);
    }
    else
    {
        cerr << "Error creating child process" << endl;
    }
}

/**
 * clear the terminal
 * @author Simon Cao - 03/06/2026
 */
void clear()
{
    //'\033[2J' clears the screen, '\033[1;1H' moves the cursor to the top-left corner.
    // These are ANSI escape codes
    cout << "\033[2J\033[1;1H";
}

/**
 * function to handle input redirection using '<' operator in the command string
 * @author Damien Ehlinger - 03/08/2026
 */
void inputRedirection(string command)
{
    // step 1: parse command into left side (program/args) and right side (input file)
    size_t pos = command.find('<');
    if (pos == string::npos)
    { // no input redirection found
        cerr << "Error: no input redirection found" << endl;
        return;
    }

    string cmd = trim(command.substr(0, pos));
    string filename = trim(command.substr(pos + 1));

    if (cmd.empty() || filename.empty())
    {
        cerr << "Usage: <command> < <input_file>" << endl;
        return;
    }

    // step 2: split command into tokens for execvp
    vector<string> tokens = tokenize(cmd);
    if (tokens.empty())
    {
        cerr << "Error: command is empty" << endl;
        return;
    }
    vector<char *> args = buildArgs(tokens);

    // step 3: create child process
    pid_t pid = fork();
    if (pid == 0)
    { // CHILD PROCESS
        // step 4: open input file and map it to standard input
        int fd = open(filename.c_str(), O_RDONLY);
        if (fd < 0)
        {
            perror("Error opening input file");
            exit(1);
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("Error redirecting standard input");
            close(fd);
            exit(1);
        }
        close(fd);

        // step 5: run the command; it now reads from the redirected STDIN
        execvp(args[0], args.data());
        perror("Error executing command");
        exit(errno);
    }
    else if (pid > 0)
    { // PARENT PROCESS
        waitpid(pid, nullptr, 0);
    }
    else
    {
        cerr << "Error creating child process" << endl;
    }
}

/**
 * function to handle output redirection using '>' operator in the command string
 * supported both overwrite '>' and append '>>' operators
 * @author Damien Ehlinger - 03/08/2026
 */
void outputRedirection(string command)
{
    bool append = false;
    size_t pos;

    // step 1: parse command into left side (program/args) and right side (output file)
    if ((pos = command.find(">>")) != string::npos) append = true;
    else if ((pos = command.find(">")) != string::npos) append = false;
    else
    {
        cerr << "Error: no output redirection found\n";
        return;
    }

    string cmd = trim(command.substr(0, pos));       // get the command part by taking the substring from the start of the command string to the position of '>' and trimming whitespace from both sides
    string filename = trim(command.substr(pos + (append ? 2 : 1))); // get the filename part by taking the substring from the position after '>' to the end of the command string and trimming whitespace from both sides
    
    // check if command or filename is empty after parsing; if so, print usage message and return
    if (cmd.empty() || filename.empty())
    {
        cerr << "Usage: <command> > <output_file>" << endl;
        return;
    }

    // step 2: split command into tokens for execvp
    vector<string> tokens = tokenize(cmd);
    if (tokens.empty())
    {
        cerr << "Error: command is empty" << endl;
        return;
    }

    // step 3: build args for execvp
    vector<char *> args = buildArgs(tokens);

    // step 3: create child process
    pid_t pid = fork();
    if (pid == 0)
    { // CHILD PROCESS
        // step 4: open output file and map it to standard output
        // O_TRUNC clears old content; O_CREAT creates file if missing
        int fd = -1;
        if (append) fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        else fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        { //   if open fails, print error message and exit with error code
            perror("Error opening output file");
            exit(1);
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        { // dup2 maps fd to STDOUT_FILENO, so that when the command writes to standard output, it goes to the file instead
            perror("Error redirecting standard output");
            close(fd);
            exit(1);
        }
        close(fd);

        // step 5: run the command; it now writes to the redirected STDOUT
        execvp(args[0], args.data());
        perror("Error executing command");
        exit(errno);
    }
    else if (pid > 0)
    { // PARENT PROCESS
        waitpid(pid, nullptr, 0);
    }
    else
    {
        cerr << "Error creating child process" << endl;
    }
}

/**
 * run other commands using execvp
 * @author Simon Cao - 03/06/2026
 */
void runCommand(string command)
{
    string trimmedCommand = trim(command);
    vector<string> tokens = tokenize(trimmedCommand);
    vector<char *> args = buildArgs(tokens);
 
    if (trimmedCommand == "exit" || trimmedCommand.empty())
    {
        exit(0);
    }
    else if (trimmedCommand == "history")
    {
        printHist();
    }
    else if (trimmedCommand == "pwd")
    {
        cout << pwd() << endl;
    }
    else if (trimmedCommand == "clear" || trimmedCommand == "cls")
    {
        clear();
    }
    else if (trimmedCommand.find('<') != string::npos)
    {
        inputRedirection(trimmedCommand);
    }
    else if (trimmedCommand.find('>') != string::npos)
    {
        outputRedirection(trimmedCommand);
    }
    else
    {
        execvp(args[0], args.data());
        //print command name and error message if execvp fails, then exit with the error code
        //convert args[0] from char* to string for printing
        cerr << "Error executing command: " << string(args[0]) << endl;
        perror("Error executing command");
        exit(errno);
    }
    exit(0);
}

/**
 * run a pipeline of commands separated by pipe operator '|'
 * support mutiple pipes in a single command, e.g. "ls -l | grep .cpp | wc -l"
 * @author Simon Cao - 03/09/2026
 * @author Dashiel Padget - 03/07/2026
 * @param subCommands a vector of strings representing the subcommands split by pipe operator
 */
void runPipeLine(vector<string> subCommands)
{
    if (subCommands.empty())
    {
        return;
    }

    int numCommands = subCommands.size();
    vector<int> pipes((numCommands - 1) * 2); // list to store pipes for n - 1 pairs of commands

    // create pipe for each pairs of commands
    for (int i = 0; i < numCommands - 1; ++i)
    {
        int currPipe = pipe(&pipes[i * 2]);
        if (currPipe < 0)
        {
            cerr << "Error creating pipe" << endl;
            return;
        }
    }

    for (int i = 0; i < numCommands; ++i)
    {
        pid_t pid = fork();
        if (pid == 0)
        { // CHILD PROCESS

            // redirect input from previous pipe if not the first command
            if (i > 0)
            {
                if (dup2(pipes[(i - 1) * 2], 0) < 0)
                {
                    perror("Error redirecting standard input");
                    exit(1);
                }
            }

            // redirect output to next pipe if not the last command
            if (i < numCommands - 1)
            {
                if (dup2(pipes[i * 2 + 1], 1) < 0)
                {
                    perror("Error redirecting standard output");
                    exit(1);
                }
            }

            // close all pipes in child
            for (int j = 0; j < pipes.size(); ++j)
            {
                close(pipes[j]);
            }

            runCommand(subCommands[i]); // execute the command;
            exit(0);
        }
        else if (pid < 0)
        {
            cerr << "Error creating child process" << endl;
            return;
        }
    }

    // parent closes all pipes
    for (size_t j = 0; j < pipes.size(); ++j)
    {
        close(pipes[j]);
    }

    // parent waits for all children
    for (int i = 0; i < numCommands; ++i)
    {
        wait(NULL);
    }
}