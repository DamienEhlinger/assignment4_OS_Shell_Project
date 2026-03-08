#include <iostream> 
#include "commands.h"

int main() {
    string command;
    while (true) {
        cout << pwd() << "~$";

        //get user input as command string
        getline(cin, command);

        //add command to history
        addToHist(command);
        if (command == "exit") exit(0);
        if (command == "history") {
            printHist();
        } else if (command == "pwd") {
            cout << pwd() << endl;
        } else if (command == "clear" || command == "cls") {
            clear();
        } else if (command == "ls") {
            ls();
        }else if (command.find("<") != string::npos){
            inputRirection(command);
        }else if (command.find(">") != string::npos){
            outputRirection(command);
        }
        else {
            runCommand(command);
        }
        
    }
}