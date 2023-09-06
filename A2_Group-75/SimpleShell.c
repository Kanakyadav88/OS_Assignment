#include "fib.h"

// Shell runs the infinite loop and reads tha users input to execute.
void shell_loop(){
    int status;
    do{
        printf("iiitd@possum:~$ ");
        char* command =read_user_input();
        status = launch(command);

    }
    while(status);

}
// Launch method accepts the user input (command name along with arguments to it)
// It will create a new process that would execute the user command and return execution status.
int launch(char *command){
    int status;
    status = create_process_and_run(command);
    return status;
}

// creating child processes

int create_process_and_run(char* command){
    int status = fork();         // Fork() is a system call used for creating a new process
    if(status<0){
        printf("Something bad happened");
    }
    else if( status==0){
        printf("I am the child process\n");
    }
    else{
        printf("I am the parent shell\n ");
    }
}