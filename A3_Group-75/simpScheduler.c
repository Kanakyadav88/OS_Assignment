#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <strings.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/resource.h>  

int ncpu;
int tslice ;
const int COMMAND_SIZE = 500;
#define maxprocess 1000 

typedef struct Process {
    int pid;
    int priority; 
    struct Process* next; 
} process;

typedef struct ProcessInfo {
    pid_t pid;
    int priority;
    double executionTime;
    time_t startTime;
} ProcessInfo;

int shmid;
ProcessInfo* sharedMemory; 

sem_t process_semaphore;

void printCharArray(char** arr, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%c ", arr[i][j]);
        }
        printf("\n");
    }
}

int create_process_and_run(char* command, int priority){
    char command_copy[COMMAND_SIZE];
    strcpy(command_copy, command);
    char* command_vector[100];
    int i = 0;
    command_vector[i] = strtok(command_copy, " ");
    while(command_vector[i] != NULL) {
        command_vector[++i] = strtok(NULL, " ");
    }
    int pid = fork();
    time_t start = time(NULL);
    if(pid < 0) {
        perror("Error in fork");
        return -1; 
    }  else if (pid == 0) {
        if (setpriority(PRIO_PROCESS, 0, priority) == -1) {
            perror("Error in setpriority");
            exit(1);
        }
        if (sem_wait(&process_semaphore) == -1) {
            perror("ERROR in sem_wait");
            exit(1);
        }
        if (execvp(command_vector[0], command_vector) == -1) {
            perror("ERROR in execvp");
            exit(1);
        }
    } else {
        FILE* history = fopen("history.txt", "a");
        if (history == NULL) {
            perror("ERROR in opening history.txt");
            return -1;
        }
        fputs(" Process ID : ", history);
        fprintf(history, "%d", pid);
        waitpid(pid, NULL, 0);
        time_t end = time(NULL);
        double diff = difftime(end, start);
        fputs(" Total Execution Time : ", history);
        fprintf(history, "%lf", diff);
         if (fclose(history) != 0) {
            perror("ERROR in closing history.txt");
        int sharedMemoryIndex =0;
        if (sharedMemoryIndex >= 0 && sharedMemoryIndex < maxprocess) {
            sharedMemory[sharedMemoryIndex].pid = pid;
            sharedMemory[sharedMemoryIndex].priority = priority;
            sharedMemory[sharedMemoryIndex].startTime = time(NULL);
        } else {
            perror("Shared memory index out of bounds");
        }
    }
    return 1;
    }
    return 1;
}

int launch (char *command, int priority) {
    int status;
    status = create_process_and_run(command, priority);
    return status;
}

typedef struct Node {
    int data;
    struct Node *next;
    char executable_name[256];  
}node;

typedef struct LinkedList {
    struct Node *head;
}readyqueue;

readyqueue  readyq = { .head = NULL };
int sizechecker(readyqueue *readyq) {
    node *current  = readyq->head ;
    int size = 0 ;
    while(current!=NULL){
        current = current->next ;
        size ++ ;
    }
    if(size==maxprocess){
        return 0  ;
        printf("Ready queue is full cant add ") ;
    }
    else{
        return 1 ;
    }
}

void add(readyqueue *readyq, int pid, const char *name, int priority) {
    node *mover = (node *)malloc(sizeof(node));
    mover->data = pid;
    strncpy(mover->executable_name, name, sizeof(mover->executable_name));
    mover->next = NULL;
    if (readyq->head == NULL) {
        readyq->head = mover;
        return;
    }

    struct Node *current = readyq->head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = mover;
}


void remover(readyqueue *readyq ) {
    if (sizechecker(readyq)!=0)
    {
    node *running = readyq->head;
    readyq->head = running->next;
    free(running); 
    }
}

int checker(const char* command) {
    int counter = 0;

    if (strcmp(command, "") == 0) return 0;
    else {
        char* token = strtok((char*)command, " ");
        while (token != NULL) {
            counter++;
            token = strtok(NULL, " ");
        }
    }
    if (counter > 2 ) return 1; 
    else  return 0;
}

void read_user_input(char* command) {
    if (fgets(command, COMMAND_SIZE, stdin) == NULL) {
        perror("Error reading user input");
    }
    command[strlen(command) - 1] = '\0';  
}

void pidprinter(int counter , readyqueue *readyq){
    node *dataprinter  = readyq->head  ;
    int i = 0 ;
    while(i!=counter)
    {
        printf("%d\n",dataprinter->data) ;
        printf("%s\n",dataprinter->executable_name) ;
        dataprinter  = dataprinter->next ;
        i++ ;
    }
}
void printer(readyqueue *readyq) {
    node *mover = readyq->head;
    int counter = 0;

    while (mover != NULL) {
        counter++;
        mover = mover->next;
    }

    printf("counter %d\n", counter);
    pidprinter(counter  , readyq) ;
}
void schedular(const char *executable, const char *command) {
    if (strcmp(command, "submit exit") == 0) {
        printf("Exiting the scheduler.\n");
        exit(0);
    } else {
        pid_t child_pid = fork();

        if (child_pid == 0) {
            execl(executable, executable, NULL);
            perror("exec");
            exit(1);
        } else if (child_pid < 0) {
            perror("fork");
        } else {
            int status;
            waitpid(child_pid, &status, 0);
        }
    }
}

void round_robin_schedule(int ncpu, int tslice, char* command) {
    int i = 0;
    time_t start_times[ncpu];
    pid_t c_pid[ncpu];
    node* currentmover = readyq.head;
    while (i < ncpu) {
        if (currentmover == NULL) {
            break;
        }

        int pid = currentmover->data;
        remover(&readyq);

        c_pid[i] = fork();

        if (c_pid[i] < 0) {
            perror("Fork failed");
            return;
        } else if (c_pid[i] == 0) {
            schedular(currentmover->executable_name, command);
            perror("Error in schedular");
            exit(1); 
        }
        else {
            start_times[i] = time(NULL);
        }

        i++;
        printf("\n");
        printf("Number of processes running: %d\n", i);

        if (i >= ncpu || currentmover->next == NULL) {

            sleep(tslice);

        }
    }
    for (int j = 0; j < i; j++) {
        if (kill(c_pid[j], SIGSTOP) == -1) {
            perror("Error stopping child process");
        }
        if (sem_post(&process_semaphore) == -1) {
            perror("Error in sem_post");
        }
        int status;
        if (waitpid(c_pid[j], &status, 0) == -1) {
            perror("Error waiting for child process");
        }
        time_t end_time = time(NULL);
        double execution_time = difftime(end_time, start_times[j]);
        double waiting_time = difftime(start_times[j], start_times[0]);
        printf("Job %d: %s (PID: %d) - Execution Time: %lf seconds - Waiting Time: %lf seconds\n", j + 1, currentmover->executable_name, c_pid[j], execution_time, waiting_time);
    }
    while (i > 0) {
        if (waitpid(c_pid[i - 1], NULL, 0) == -1) {
            perror("Error waiting for child process");
        }
        i--;
    }
}



volatile int scheduling_started = 0;  

void shell_loop(char* command, int ncpu, int tslice) {
    int status;
    int submitchecker = 0;
    int priority = 1; 
    scheduling_started = 1;
    if (strncmp(command, "submit", 6) == 0) {
        const char* priority_str = command + 7;
        int priority = atoi(priority_str);

        if (priority >= 1 && priority <= 4) {
            
        } else {
            printf("Invalid priority value: %d. Priority must be between 1 and 4.\n", priority);
        }
    }

    do {    
        read_user_input(command);
        printf("CocoShell:~$ ");
        int flag = 0;
        if (checker(command) == 1) {
            printf("The command has parameters.\n");
            printf("Try again\n\n");
            printf("Welcome:~$ ");
        }
        else
        {
            const char* calls[] = {
                "read", "write", "sleep", "usleep",
                "nanosleep", "select", "poll", "accept",
                "wait", "waitpid", "pthread_mutex_lock", "pthread_cond_wait",
                "recv", "send", "open", "fopen"
            };

            for (int i = 0; i < sizeof(calls) / sizeof(calls[0]); i++) {
                char* executable = command + 7;
                if (strstr(executable, calls[i]) != NULL) {
                    printf("Error Blocking sys call found (%s).\n", calls[i]);
                }
            }

            // exit schedler 
            if (strcmp(command, "submit exit") == 0) {
                printf("Exiting the program.\n");
                break;
            }

            
            if (strncmp(command, "submit", 6) == 0) {
                char* executable = command + 7;
                int pid = fork();
                printf("line 375 : %d yup\n",pid);
                if (pid < 0) {
                    perror("Fork failed");
                } 
                else if (pid == 0){
                    printf("line 380");
                    printf("line 381 : %d ", sizechecker(&readyq));
                    if (sizechecker(&readyq) == 1) {
                        add(&readyq, pid, executable, priority);
                        printf("process is added ")  ;
                    }
                    else {
                        printf("The ready queue is full. Cannot add more items.\n");
                        printer(&readyq);
                    }
                    submitchecker = 1;
                    kill(getpid(), SIGUSR1);
                } 
                else{
                    printf("line 393");
                    int status;
                    if (waitpid(pid, &status, 0) == -1) {
                    perror("Error waiting for child process");
                    }
                }

                if (sizechecker(&readyq) > 0 && (scheduling_started || submitchecker)) {
                    round_robin_schedule(ncpu, tslice, command);
                } else {
                    printf("No jobs have been submitted yet. Please submit jobs before scheduling.\n");
                    scheduling_started = 0;
                }
            }

            FILE* history = fopen("history.txt", "a");
            if (history == NULL) {
                perror("Error opening history.txt");
                return;
            }
            char hist[10] = "history";
            char letter;
            if (!strcmp(command, hist)) {
                printf("\n");
                letter = fgetc(history);
                while (letter != EOF) {
                    printf("%c", letter);
                    letter = fgetc(history);
                }
            }
            fputs(command, history);
            if (fputs("\n", history) == EOF) {
                perror("Error writing to history.txt");
            }
            if (fclose(history) == EOF) {
                perror("Error closing history.txt");
                return;
            }
            status = launch(command, priority);
            history = fopen("history.txt", "a");
            if (history == NULL) {
                perror("Error opening history.txt");
                return;
            }
            if (fputs("\n", history) == EOF) {
                perror("Error writing to history.txt");
            }
            if (fclose(history) == EOF) {
                perror("Error closing history.txt");
                return;
            }
        }
    }while (status);
}
void my_handler(int signum) {
    FILE* history = fopen("history.txt", "r");
    if (history == NULL) {
        perror("Error opening history.txt");
        return;  
    }
    char hist[10] = "history";
    char letter;
    printf("\n");
    letter = fgetc(history);
    while(letter != EOF) {
        printf("%c", letter);
        letter = fgetc(history);
    
        if (fclose(history) != 0) {
            perror("Error closing history.txt");
            return;  
        }
    }
    exit(0);
}

int main() {
    // shared memory creation
    shmid = shmget(IPC_PRIVATE, maxprocess * sizeof(ProcessInfo), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Error creating shared memory");
        return 1;
    }
        // shared memory attach
    sharedMemory = (ProcessInfo*)shmat(shmid, (void*)0, 0);
    if (sharedMemory == (void*)-1) {
        perror("Error attaching to shared memory");
        return 1;
    }


    while(1)    
    {
    
        printf("Enter NCPU:  ") ;
        scanf("%d",&ncpu) ;
        if (ncpu < 1) {
            perror("Invalid NCPU");
            return 1;  
        }
        else {
            printf("Enter TSLICE: ") ;    
            scanf("%d",&tslice) ;
            if(tslice<0){
                perror("TSLICE cannot be negative");
                return 1; 
            }
            else {
                struct sigaction sig;
                sig.sa_handler = my_handler;
                sigaction(SIGINT, &sig, NULL);
                if (sem_init(&process_semaphore, 0, ncpu) == -1) {
                    perror("Error in sem_init");
                    return 1;
                }
                char command[COMMAND_SIZE];
                FILE* history = fopen("history.txt", "w"); 
                if (history == NULL) {
                    perror("ERROR in opening history.txt");
                    return 1;
                }
                if (fclose(history) != 0) {
                    perror("ERROR in closing history.txt");
                }
                shell_loop(command, ncpu, tslice);

                if (sem_destroy(&process_semaphore) == -1) {
                    perror("Error in sem_destroy");
                }
             // shared memory detached and removal
                shmdt(sharedMemory);
                shmctl(shmid, IPC_RMID, NULL);
                
            }
        }
    }return 0;
}

