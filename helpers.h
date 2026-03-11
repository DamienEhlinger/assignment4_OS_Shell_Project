#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>

using namespace std;

/**
 * Tokenize a command string into a vector of strings, splitting by whitespace
 * @author Simon Cao - 03/09/2026
 * @param command the command string to tokenize
 * @return a vector of strings representing the tokens in the command
 */
vector<string> tokenize(const string &command)
{
    bool inQuotes = false; // flag to track if we are inside quotes
    vector<string> tokens;
    string currentToken;
    for (size_t i = 0; i < command.length(); ++i)
    {
        char c = command[i];
        if (c == '"')
            inQuotes = !inQuotes;
        else if (isspace(c) && !inQuotes)
        {
            if (!currentToken.empty())
            {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        }
        else
        {
            currentToken += c;
        }
    }
    if (!currentToken.empty())
    {
        tokens.push_back(currentToken);
    }
    return tokens;
}

/**
 * Trim leading and trailing whitespace from a string
 * @author Damien Ehlinger - 03/08/2026
 * @param s the string to trim
 * @return the trimmed string
 */
string trim(const string &s)
{
    size_t start = s.find_first_not_of(" \t"); // find the first non-whitespace character in the string; if there is no non-whitespace character, return an empty string; otherwise, return the substring from the first non-whitespace character to the end of the string, effectively trimming leading whitespace from the input string
    if (start == string::npos)
        return string("");                   // find the first non-whitespace character in the string; if there is no non-whitespace character, return an empty string; otherwise, return the substring from the first non-whitespace character to the end of the string, effectively trimming leading whitespace from the input string
    size_t end = s.find_last_not_of(" \t");  // find the last non-whitespace character in the string; if there is no non-whitespace character, return an empty string; otherwise, return the substring from the first non-whitespace character to the last non-whitespace character, effectively trimming leading and trailing whitespace from the input string
    return s.substr(start, end - start + 1); // return the trimmed string
}

/**
 * Build a list of parameters for execvp from tokens
 * Works by converting each string of token into a char* then added to a vector.
 * @author Simon Cao - 03/09/2026
 * @param command the command string to parse
 * @return a vector of char* representing the command and its arguments
 */
vector<char *> buildArgs(const vector<string>& tokens)
{
    vector<char *> args;
    int i = 0;
    for (const string &token : tokens)
    {
        args.push_back((char *)token.c_str());
    }
    args.push_back(nullptr);
    return args;
}

/**
 * Split pipe command into subcommands string
 * @author Simon Cao - 03/09/2026
 * @param command the command string to split
 * @return a vector of strings representing the subcommands split by pipe operator
 */
vector<string> splitCommandbyPipe(const string &command)
{
    vector<string> subCommands;
    stringstream ss(command);
    string part;
    while (getline(ss, part, '|'))
    {
        subCommands.push_back(part);
    }
    return subCommands;
}