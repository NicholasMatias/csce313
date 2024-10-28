#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <termios.h>
#include <cstring>

#include <vector>
#include <string>
#include <ctime>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define WHITE "\033[1;37m"
#define NC "\033[0m"
#define PATH_MAX 500

using namespace std;

/**
 * Return Shell prompt to display before every command as a string
 *
 * Prompt format is: `{MMM DD hh:mm:ss} {user}:{current_working_directory}$`
 */
void print_prompt()
{
        // Use time() to obtain the current system time, and localtime() to parse the raw time data
        time_t currentTime;
        struct tm *timeInfo;
        time(&currentTime);
        timeInfo = localtime(&currentTime);
        char terminalTime[256];

        // Parse date time string from raw time data using strftime()
        // Date-Time format will be in the format (MMM DD hh:mm:ss), with a constant size of 15 characters requiring 16 bytes to output
        strftime(terminalTime, 256, "%b %d %T", timeInfo);

        // Obtain current user and current working directory by calling getenv() and obtaining the "USER" and "PWD" environment variables
        string user = getenv("USER");
        string directory = getenv("PWD");

        // Can use colored #define's to change output color by inserting into output stream
        // MODIFY to output a prompt format that has at least the USER and CURRENT WORKING DIRECTORY
        std::cout << YELLOW << terminalTime << " " << YELLOW << user << ":" << YELLOW << directory << "$" << NC << " ";
}

/**
 * Process shell command line
 *
 * Each command line from the shell is parsed by '|' pipe symbols, so command line must be iterated to execute each command
 * Example: `ls -l / | grep etc`    ::  This command will list (in detailed list form `-l`) the root directory and then pipe into a filter for `etc`
 * When parsed into the Tokenizer, this will split into two separate commands, `ls -l /` and `grep etc`.
 */
void process_commands(Tokenizer &tknr)
{
        // Declare file descriptor variables for storing unnamed pipe fd's
        // Maintain both a FORWARD and BACKWARDS pipe in the parent for command redirection
        int backwards_fds[2];
        int forwards_fds[2];
        vector<pid_t> background_processes;

        // LOOP THROUGH COMMANDS FROM SHELL INPUT
        for (size_t i = 0; i < tknr.commands.size(); ++i)
        {
                auto command = tknr.commands.at(i);
                // Check if the command is 'cd' first
                // This will not have any pipe logic and needs to be done manually since 'cd' is not an executable command
                // Use getenv(), setenv(), and chdir() here to implement this command
                if (command->args[0] == "cd")
                {
                        // There are two tested inputs for 'cd':
                        // (1) User provides "cd -", which directs to the previous directory that was opened
                        // (2) User provides a directory to open
                        if (command->args.size() >= 2)
                        {
                                if (command->args[1] == "-")
                                {
                                        string previous_directory = getenv("OLDPWD");
                                        if (previous_directory.empty() == false)
                                        {
                                                chdir(previous_directory.c_str());
                                                setenv("OLDPWD", getenv("PWD"), 1);
                                                setenv("PWD", previous_directory.c_str(), 1);
                                        }
                                        else
                                        {
                                                cerr << "No previous directory found" << endl;
                                        }
                                }
                                else
                                { // User provides a valid directory to open
                                        string new_directory = command->args[1];
                                        auto old_cwd = getcwd(nullptr, 0);
                                        setenv("OLDPWD", old_cwd, 1);
                                        if (chdir(new_directory.c_str()) == 0)
                                        {
                                                auto new_cwd = getcwd(nullptr, 0);
                                                setenv("PWD", new_cwd, 1);
                                                free(new_cwd);
                                        }
                                        else
                                        {
                                                cerr << "Directory does not exist" << endl;
                                        }
                                        free(old_cwd);
                                }
                        }
                        continue;
                }

                // Initialize backwards pipe to forward pipe
                // memcpy(backwards_fds, forwards_fds, sizeof(forwards_fds));
                // If any other command, set up forward pipe IF the current command is not the last command to be executed
                if (i != tknr.commands.size() - 1)
                {
                        if (pipe(forwards_fds) < 0)
                        {
                                cerr << "pipe creation error" << endl;
                                exit(1);
                        }
                }

                // fork to create child
                pid_t pid = fork();
                if (pid == -1)
                { // error check
                        cerr << "fork error" << endl;
                        exit(1);
                }
                if (pid == 0)
                { // if child, exec to run command
                        if (i != tknr.commands.size() - 1)
                        {                                             // i.e. {current command} | {next command} ...
                                dup2(forwards_fds[1], STDOUT_FILENO); // Redirect STDOUT to forward pipe
                                close(forwards_fds[1]);               // Close respective pipe end
                                close(forwards_fds[0]);
                        }

                        if (i != 0)
                        {                                             // i.e. {first command} | {current command} ...
                                dup2(backwards_fds[0], STDIN_FILENO); // Redirect STDIN to backward pipe
                                close(backwards_fds[0]);              // Close respective pipe end
                                close(backwards_fds[1]);
                        }

                        if (command->hasInput())
                        {                                                                       // i.e. {command} < {input file}
                                int input_file_desc = open(command->in_file.c_str(), O_RDONLY); // Open input file
                                if (input_file_desc < 0)
                                {
                                        cerr << "could not open input file" << endl;
                                        exit(1);
                                }
                                dup2(input_file_desc, STDIN_FILENO); // Redirect STDIN from file
                                close(input_file_desc);              // Close file
                        }

                        if (command->hasOutput())
                        {                                                                                                   // i.e. {command} > {output file}
                                int output_file_desc = open(command->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777); // Open output file
                                if (output_file_desc < 0)
                                {
                                        cerr << "could not open output file" << endl;
                                        exit(1);
                                }
                                dup2(output_file_desc, STDOUT_FILENO); // Redirect STDOUT to file
                                close(output_file_desc);               // Close file
                        }

                        vector<char *> arguments;
                        for (const string &argument : command->args)
                        {
                                arguments.push_back(const_cast<char *>(argument.c_str()));
                        }
                        arguments.push_back(nullptr); // Terminate the args array with nullptr

                        if (execvp(arguments[0], arguments.data()) < 0)
                        { // error check for execution failure
                                cerr << "execvp error" << endl;
                                exit(1);
                        }
                }
                else
                { // if parent, wait for child to finish
                        // Close pipes from parent so pipes receive `EOF`
                        // Pipes will otherwise get stuck indefinitely waiting for parent input/output that will never occur
                        if (i != 0)
                        {
                                close(backwards_fds[0]);
                                close(backwards_fds[1]);
                        }

                        backwards_fds[0] = forwards_fds[0];
                        backwards_fds[1] = forwards_fds[1];

                        // If command is indicated as a background process, set up to ignore child signal to prevent zombie processes
                        if (command->isBackground())
                        {
                                background_processes.push_back(pid);
                                // signal(SIGCHLD, SIG_IGN);
                        }
                        else
                        {
                                int status = 0;
                                waitpid(pid, &status, 0);

                                if (status > 1)
                                { // exit if child didn't exec properly
                                        exit(status);
                                }
                        }
                }
        }
}

int main()
{
        // Use setenv() and getenv() to set an environment variable for the previous PWD from 'PWD'
        auto start_old_cwd = getcwd(nullptr, 0);
        setenv("OLDPWD", start_old_cwd, 1);
        free(start_old_cwd);

        // This is your process loop; Will run infinitely until given defined break case
        for (;;)
        {
                // need to print date/time, username, and absolute path to current dir into 'shell'
                // Might want to use a helper function like this here for code clarity
                print_prompt();

                // get user inputted command
                string input;
                getline(cin, input);

                if (input == "exit" || input == "Exit")
                { // print exit message and break out of infinite loop
                        cout << RED << "Now exiting shell..." << endl
                        << "Goodbye" << NC << endl;
                        break;
                }
                // Might want to catch empty input; Tokenizer will error and not recover on empty input

                // get tokenized commands from user input
                // This will parse your user input for you; Please refer the header files to understand how to use this class
                Tokenizer tknr(input);
                if (tknr.hasError())
                { // continue to next prompt if input had an error
                        continue;
                }

                // Might want to use a helper function like this for processing the shell's command line
                process_commands(tknr);
        }
        while (wait(nullptr) > 0)
                ; // wait for all child processes to finish before exiting shell
        return 0;
}