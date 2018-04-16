
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_SIZE 1024
#define RUN_BACKGROUND -1

struct node {
    char data[MAX_SIZE];
    int pid;
    struct node *next;
};

struct node *head = NULL;
struct node *current = NULL;

//delete a link with given key
struct node* delete(int pid) {

    //start from the first link
    struct node* current = head;
    struct node* placeHolder = head;
    struct node* previous = NULL;

    //if list is empty
    if(head == NULL) {
        return NULL;
    }

    //navigate through list
    while(current->pid != pid) {

        //if it is last node
        if(current->next == NULL) {
            return NULL;
        } else {
            //store reference to current link
            previous = current;
            //move to next link
            current = current->next;
        }
    }

    //found a match, update the link
    if(current == head) {
        //change first to point to next link
        free(placeHolder);
        head = head->next;
    } else {
        //bypass the current link
        previous->next = current->next;
    }

    return current;
}

//display the list
void printList() {
    int stat;
    struct node *ptr = head;

    //start from the beginning
    while(ptr != NULL) {
        pid_t return_pid = waitpid(ptr->pid, &stat, WNOHANG);
        if (return_pid == 0) {
            printf("%d %s \n", ptr->pid, ptr->data);
            ptr = ptr->next;
        }else{
           ptr = delete(ptr->pid);
        }
    }

}

//delete list.
void deleteCommandList(){
    struct node *ptr = head;

    //start from the beginning
    while(ptr != NULL) {
           ptr=delete(ptr->pid);
        }

}

//insert link at the first location
void insertCommand(int key, char *string) {
    //create a link
    struct node *link = (struct node*) malloc(sizeof(struct node));
    link->pid = key;
    strcpy(link->data,string);

    //point it to old first node
    link->next = head;

    //point first to new first node
    head = link;
}

char** dissectInput(char *string){
    char **command = malloc(8 * sizeof(char *));
    char *token;
    int i = 0;
    token = strtok(string, " ");

    while (token != NULL) {
        command[i] = token;
        i++;

        token = strtok(NULL, " ");
    }

    command[i] = NULL;
    return command;
}

int waitArg(char **array) {
    int lastElm;
    int index = 0;
    while (1) {
        index++;
        if (array[index] == NULL){
            lastElm = index - 1;
            break;
        }
    }

    if (strcmp(array[lastElm], "&") == 0){
        array[lastElm] = NULL;
        return -1;
    }
    return 1;
}

int cd(char *path) {
    return chdir(path);
}

int main() {
    //declaring variables.
    int wait;
    int stat;

    char recentpath[MAX_SIZE];
    char currentDir[MAX_SIZE];
    char **commandArray;

    pid_t child_pid;
    pid_t return_pid;

    //getting the current dir.
    getcwd(currentDir, sizeof(currentDir));
    strcpy(recentpath,currentDir);

    while(1){
        //accepting input.
        char buffer[MAX_SIZE];
        char secondBuffer[MAX_SIZE];

        printf("prompt>");
        fgets(buffer, MAX_SIZE, stdin);

        //removing unnecessary parts of the input line.
        buffer[strcspn(buffer, "\r\n")] = '\0';

        strcpy(secondBuffer,buffer);
        // turning input command into tokens.
        commandArray = dissectInput(buffer);

        //checking for & Arg (if the command should run in background).
        wait = waitArg(commandArray);

        if(strcmp(buffer,"exit") == 0){
            printf("%d",getpid());
            break;
        }

        if (strcmp(commandArray[0], "cd") == 0) {
            printf("%d\n", getpid());

            if (commandArray[1] == NULL) {
                if (cd(currentDir) < 0) {
                    fprintf(stderr, "%s", "Error in system call\n");
                }
                free(commandArray);
                //skipping the fork.
                continue;
            }

            if(strcmp(commandArray[1],"-") == 0){
                if (cd(recentpath) < 0) {
                    fprintf(stderr, "%s", "Error in system call\n");
                }
                free(commandArray);
                //skipping the fork.
                continue;
            }

            if(strcmp(commandArray[1],"~") == 0){
                strcpy(commandArray[1],"..");
                if (cd(commandArray[1]) < 0) {
                    fprintf(stderr, "%s", "Error in system call\n");
                }
                free(commandArray);
                //skipping the fork.
                continue;
            }


            if (cd(commandArray[1]) < 0) {
                fprintf(stderr, "%s", "Error in system call\n");
            }else{
                strcpy(recentpath,commandArray[1]);
            }

            free(commandArray);
            //skipping the fork.
            continue;
        }


        if(strcmp(commandArray[0], "jobs") == 0) {
            printList();
            free(commandArray);
            //skipping the fork.
            continue;
        }


        if(strcmp(commandArray[0], "kill") == 0) {
            execv(commandArray[0], commandArray);
        }

        //forking the main process to run the command.
        child_pid = fork();

        if (child_pid == 0) {
            //child process.
            //running the command.
            execvp(commandArray[0], commandArray);
            //if execvp failed print error.
            fprintf(stderr, "%s", "Error in system call\n");
        } else {
            //parent process.

            printf("%d\n",child_pid);

            if (wait != RUN_BACKGROUND) {
                waitpid(child_pid, &stat, WUNTRACED);
            }else{
                char* e = strchr(secondBuffer, '&');
                int index = (int)(e - secondBuffer);
                secondBuffer[index] = ' ';
                insertCommand(child_pid,secondBuffer);
            }
        }

    }

    free(commandArray);
    deleteCommandList();
    return 0;


}
