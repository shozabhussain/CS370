/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Copyright John Wiley & Sons - 2018
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include<fcntl.h>
#include<stdlib.h>

#define MAX_LINE        80 /* 80 chars per line, per command */

int main(void)
{
    char *args[MAX_LINE/2 + 1]; /* command line (of 80) has max of 40 arguments */
    int should_run = 1;
    char recent[MAX_LINE/2 + 1 ] ;
    char variable_name_arr[10][100] ;
    char variable_arr[10][100] ;
    int number_variables = 0 ;

        while (should_run){
            printf("osh>");
            fflush(stdout);

            char inp[MAX_LINE/2 + 1] ;
            fgets(inp, MAX_LINE/2 + 1, stdin) ;

            inp[strlen(inp)-1] = '\0' ;

            int check_recent = strcmp(inp, "!!") ;
            if(check_recent == 0)
            {
                char *temp = recent[0] ;
                if(temp == NULL)
                {
                    printf("No commands found\n") ;
                    continue ;
                }
                else
                {
                    strcpy(inp, recent) ;
                    printf("%s\n", inp) ;
                }
            }

            int ampercent = 0 ;
            if(strchr(inp, '&')!= NULL)
            {
                char temp[MAX_LINE/2 + 1] ;
                strcpy(temp, inp) ;

                char *temp1 = strtok(temp, "&") ;
                strcpy(inp, temp1) ;
                inp[strlen(inp)-1] = '\0' ;
                ampercent = 1 ;
            }

            if(strchr(inp, '=')!= NULL)
            {
                char temp[ ] = " " ;
                char temp1[ ] = " " ;
                strcpy(temp, inp) ;
                strcpy(temp1, inp) ;
                int found = 0;

                char *variable_name = strtok(temp1,"=") ;
                char *variable_value = strtok(NULL,"=") ;

                for(int i=0; i<number_variables; i++)
                {
                    if(strcmp(variable_name_arr+i, variable_name) == 0)
                    {
                        strcpy(variable_arr[i], variable_value);
                        found = 1 ;
                    }
                }

                if(found ==1)
                {
                    continue ;
                }


                if(number_variables == 10)
                {
                    printf("limit reached for variables\n") ;
                    continue ;
                }

                strcpy(variable_name_arr[number_variables], strtok(temp,"="));
                strcpy(variable_arr[number_variables], strtok(NULL,"="));

                number_variables = number_variables +1 ;
                continue ;
            }

            int var_found = 0 ;
            char *var ;
            if(strchr(inp, '$')!= NULL)
            {
                char arr1[ ] = " " ;
                strcpy(arr1, inp) ;

                char *temp = strtok(arr1,"$") ;
                char *temp1  = strtok(NULL,"$") ;
                char *var_name = strtok(temp1," ") ;

                for(int i=0; i<number_variables; i++)
                {
                    if(strcmp(variable_name_arr+i, var_name) == 0)
                    {
                        var_found = 1 ;
                        var = variable_arr+i ;
                    }
                }

                if(var_found == 0)
                {
                    printf("variable not found\n") ;
                    continue ;
                }
            }

            char * cmd1 ;
            char * arg1 ;
            char * filename ;
            char * cmd2 ;
            char * arg2 ;

            char temp[ ] = " " ;
            strcpy(temp, inp) ;

            int redirect = 0 ;
            if(strchr(temp, '>')!= NULL)
            {
                char * command = strtok(temp, ">") ;
                filename = strtok(NULL, " ") ;
                redirect = 1 ;
                strcpy(temp, command) ;
                temp[strlen(temp)-1] = '\0' ;
            }

            int pipecmd = 0 ;
            if(strchr(temp, '|')!= NULL)
            {
                pipecmd = 1 ;
                char * command1 = strtok(temp, "|") ;
                char * command2 = strtok(NULL, "\0") ;

                strcpy(temp, command1) ;
                temp[strlen(temp)-1] = '\0' ;

                cmd2 = strtok(command2, " ") ;
                arg2 = strtok(NULL, " ") ;
            }

            cmd1 = strtok(temp, " ") ;
            arg1 = strtok(NULL, "\0") ;

            if(var_found == 1)
            {
                arg1 = var ;
            }

            pid_t pid ;
            int status ;
            pid = fork();

            if(pid == 0)
            {
                if (pipecmd == 1)
                {
                    int status1 ;
                    pid_t pid_grandchild ;
                    int fd[2] ;

                    pipe(fd);
                    pid_grandchild = fork();

                    if(pid_grandchild == 0)
                    {
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[0]);
                        close(fd[1]);

                        char* argument_list[] = {cmd1, arg1, NULL};
                        execvp(cmd1, argument_list) ;
                        perror("exec");
                        exit(1);
                    }
                    else if(pid_grandchild > 0)
                    {
                        while(wait(&status1)!=pid_grandchild) ;

                        dup2(fd[0], STDIN_FILENO);
                        close(fd[1]);
                        close(fd[1]);

                        char* argument_list[] = {cmd2, arg2, NULL};
                        execvp(cmd2, argument_list) ;
                        fprintf(stderr, "Failed to execute\n");
                        exit(1);
                    }
                }

                if(redirect == 1)
                {
                    int file_desc = open(filename,O_CREAT|O_TRUNC|O_WRONLY,0644);
                    dup2(file_desc, 1) ;
                    close(file_desc) ;
                    char* argument_list[] = {cmd1, arg1, NULL};
                    execvp(cmd1, argument_list) ;
                    perror("exec");
                    exit(1);
                }
                else
                {
                    char* argument_list[] = {cmd1, arg1, NULL};
                    execvp(cmd1, argument_list) ;
                    perror("exec");
                    exit(1);
                }
            }
            else if(pid > 0)
            {
                if(ampercent == 0)
                {
                    while(wait(&status)!=pid) ;
                }

                strcpy(recent, inp) ;
            }

            /**
             * After reading user input, the steps are:
             * (1) fork a child process
             * (2) the child process will invoke execvp()
             * (3) if command includes &, parent and child will run concurrently
             */
        }

    return 0;
}

