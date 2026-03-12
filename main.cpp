/**
 * Group Members: Simon Cao, Damien Ehlinger, Dashiel Padget
 * Date: 03/09/2026
 */
#include <iostream> 
#include "commands.h"

int main() {
    string command;
    while (true) {
        cout << pwd() << "~$";

        //get user input as command string
        getline(cin, command);
        command = trim(command);

        if (command.empty()) {
            continue; // skip empty commands
        }

        //add command to history
        addToHist(command);
        if (command == "exit") exit(0);
        else if (command == "clear" || command == "cls") {
            clear();
        } else {
           vector<string> subCommands = splitCommandbyPipe(command);
           runPipeLine(subCommands);
        }
        
    }
}