/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <sys/wait.h> // For waitpid
#include <iostream>
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality

    int pipefd[2];
    pid_t pid1, pid2;

    // Create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    // Create child to run first command
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(1);
    } else if (pid1 == 0) {
        // Child process for first command
        close(pipefd[0]); // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[1]);
        execvp(cmd1[0], cmd1);
        perror("execvp cmd1");
        exit(1);
    }

    // Create another child to run second command
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(1);
    } else if (pid2 == 0) {
        // Child process for second command
        close(pipefd[1]); // Close unused write end
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to pipe
        close(pipefd[0]);
        execvp(cmd2[0], cmd2);
        perror("execvp cmd2");
        exit(1);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
