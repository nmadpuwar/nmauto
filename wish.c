#include <stdio.h>
#include <fcntl.h>
#include <regex.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

char **getParsedCommand(char *inputCommand)
{
    char *inputCopy = strdup(inputCommand), *delimiter = " ", *token = NULL;
    char **parsedInput = malloc(sizeof(char *) * 20);
    int i = 0, flag = 0, index = 0;

    if (parsedInput == NULL || inputCopy == NULL)
    {
        perror("malloc failed");
        exit(1);
    }

    token = strtok(inputCopy, delimiter); /* Parse the input based on the delimiter */

    for (i = 0; i < strlen(token); i++)
    {
        if ((token[i] >= 33 && token[i] <= 47 && token[i] != 46) || (token[i] >= 58 && token[i] <= 64 && token[i] != 62))
        {
            if (i == 0)
            {
                flag = 1; /* special character present */
                break;
            }
        }
    }

    if (flag == 1)
    {
        parsedInput = NULL;
    }
    else
    {
        while (token != NULL)
        {
            parsedInput[index] = strdup(token);
            index++;
            token = strtok(NULL, delimiter);
        }
        if (parsedInput[index - 1][strlen(parsedInput[index - 1]) - 1] == '\n')
        {
            parsedInput[index - 1][strlen(parsedInput[index - 1]) - 1] = '\0';
        }
        parsedInput[index] = NULL;
    }
    // free(inputCopy);
    return parsedInput;
}

int changeDirectory(char **parsedCommand, char error_message[])
{
    if (chdir(parsedCommand[1]) < 0) /* change directory to parsedCommand[1] */
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    return 0;
}

int getCurrentDirectory(char **parsedCommand)
{
    char *cwd = (char *)malloc(100 * sizeof(char));

    getcwd(cwd, 100); /* fetches current working directory */
    printf("%s\n", cwd);
    return 0;
}

int setPath(char **parsedCommand)
{
    int i = 1;
    char path[20480] = "PATH=", pathCopy[2048];

    strcpy(pathCopy, path); /* copy path array to pathcopy array */
    if (parsedCommand[1])
    {
        while (parsedCommand[i + 1] != NULL)
        {
            strcat(pathCopy, parsedCommand[i]); /* concatenate the path variable with path array */
            strcat(pathCopy, ":");              /* set the delimiter after every path */
            i++;
        }
        strcat(pathCopy, parsedCommand[i]);
    }
    printf("Path : %s\n", pathCopy);
    putenv(pathCopy); /* set path env variable */
    return 0;
}

int echoCommand(char **parsedCommand)
{
    int index = 0, i = 1;
    char *firstOccPointer = NULL, *env = NULL, *end = NULL;

    if (strlen(parsedCommand[1]) > 0)
    {
        firstOccPointer = strchr(parsedCommand[1], '$'); /* check for env variables */
        index = (int)(firstOccPointer - parsedCommand[1]);
        if (index == 0) /* If $ is at the first position, check for env variable */
        {
            parsedCommand[1]++;
            env = getenv(parsedCommand[1]);
            if (env != NULL)
            {
                printf("%s", env);
            }
        }
        else
        {
            while (parsedCommand[i] != NULL)
            {
                while (isspace((unsigned char)*parsedCommand[i])) /* check for leading whitespaces */
                    parsedCommand[i]++;

                if (*parsedCommand[i] == 0) /* All spaces */
                    break;

                end = parsedCommand[i] + strlen(parsedCommand[i]) - 1;
                while (end > parsedCommand[i] && isspace((unsigned char)*end)) /* check for trailing whitespaces */
                    end--;

                end[1] = '\0';
                printf("%s", parsedCommand[i]);
                if (parsedCommand[i + 2] != NULL)
                {
                    printf(" ");
                }
                i++;
            }
        }
    }
    printf("\n");
    return 0;
}

int listDirectory(char **parsedCommand, char error_message[])
{
    char extraPathCopy[20480];
    int syntaxError = 0, redirection = 0, index = 0, fd = -1, ofd, stat_loc;
    char *getPath = NULL, *pathCopy = NULL, *parsedPaths, *checkRedirectionString = NULL, **redirectionArray = malloc(10 * sizeof(char));

    while (parsedCommand[index] != NULL)
    {
        if ((strcmp(parsedCommand[index], ">") == 0) &&
            (index == 0 || parsedCommand[index + 1] == NULL || strcmp(parsedCommand[index + 1], "") == 0 ||
             strcmp(parsedCommand[index + 1], ">") == 0 || strcmp(parsedCommand[index + 2], ">") == 0 || parsedCommand[index + 2] != NULL))
        {
	    printf("Failed to list directory:\n");
            syntaxError = 1;
            break;
        }
        index++;
    }

    if (syntaxError == 1)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }

    getPath = getenv("PATH");
    if (strlen(getPath) == 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else
    {
        pathCopy = strdup(getPath);
        parsedPaths = strtok(pathCopy, ":");
        while (parsedPaths != NULL)
        {
            strcpy(extraPathCopy, parsedPaths);
            strcat(extraPathCopy, "/ls");
            fd = access(extraPathCopy, X_OK);
            if (fd != -1)
            {
                if (parsedCommand[1] && (strchr(parsedCommand[1], '>') > 0))
                {
                    redirection = 1;
                    checkRedirectionString = strtok(parsedCommand[1], ">");
                    index = 0;
                    while (checkRedirectionString != NULL)
                    {
                        redirectionArray[index] = checkRedirectionString;
                        index++;
                        checkRedirectionString = strtok(NULL, ">");
                    }
                }

                pid_t child_pid = fork();
                if (child_pid < 0)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
                if (child_pid == 0)
                {
                    if (redirection == 1)
                    {
                        close(STDOUT_FILENO);
                        ofd = open(redirectionArray[index - 1], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
                        //   dup(fileno(ofp));
                    }
                    if (execv(extraPathCopy, parsedCommand) < 0)
                    {
			printf("Exec failed\n");
                        perror(parsedCommand[0]);
                    }
                }

                if (redirection == 1)
                {
                    close(ofd);
                    fflush(stdout);
                }
                waitpid(child_pid, &stat_loc, WUNTRACED);
                break;
            }
            parsedPaths = strtok(NULL, ":");
        }
        if (fd == -1)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    return 0;
}

int externalCommands(char **parsedInput, char error_message[])
{
    printf("External command : %s\n", parsedInput[0]);
    char *getPath = NULL, *pathCopy = NULL, *token = NULL;
    char extraString[20468];
    int i = 0, parallelCount = 1, index = 0, fd = 0, stat_loc;

    getPath = getenv("PATH");
    if (strlen(getPath) == 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else
    {
        i = 0;
        while (parsedInput[i] != NULL) /* check for parallel commands */
        {
            if ((strcmp(parsedInput[i], "&") == 0) && (parsedInput[i + 1] != NULL))
            {
                parallelCount++;
            }
            i++;
        }
        pid_t child_pids[parallelCount + 1]; /* Intialize the child_pids based on the number of commands */
        for (i = 0, index = 0; i < parallelCount && parsedInput[index] != NULL; i++, index = index + 2)
        {
            pathCopy = getenv("PATH");
            token = strtok(pathCopy, ":");
            while (token != NULL)
            {
                strcpy(extraString, token);
                strcat(extraString, "/");
                strcat(extraString, parsedInput[index]);
                printf("$$$$$$$$$$$$$$$$$$$$$$$$$$%s\n", extraString);
                fd = access(extraString, X_OK);
                if (fd != -1)
                {
		    printf("Created access\n");
                    child_pids[i] = fork();
                    if (child_pids[i] < 0)
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
			printf("Failed to create child process\n");
                        exit(1);
                    }

		    printf("Child process created : %d\n", child_pids[i]);
                    if (child_pids[i] == 0)
                    {
                        if (execv(extraString, parsedInput) < 0)
                        {
			    printf("Error while executing the command \n");
                            perror(parsedInput[index]);
                        } else {
			   printf("Execution failed with non zero exit code\n");
			}
                    }
                    else
                    {
			printf("Waiting for the process %d...\n", child_pids[i]);
                        waitpid(child_pids[i], &stat_loc, WUNTRACED);
			printf("Process completed. %d \n", child_pids[i]);
                        break;
                    }
                } else {
			printf("Failed to create file descriptor\n");
		}
                token = strtok(NULL, ":");
                if (fd == -1)
                {
		    printf("Failed to create file descriptor\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
        }
    }
    return 0;
}

int executeCommands(char *command, char error_message[])
{
    int commandStatus = 0;
    char **parsedInput;

    while (isspace((unsigned char)*command)) /* check for whitespaces */
    {
        command++;
        if (*command == 0)
            break;
    }
    if (strlen(command) == 0)
    {
        return 0;
    }
    parsedInput = getParsedCommand(command);
    if (parsedInput == NULL) /* Parsed command is NULL */
    {
        return 0;
    }
    else if (strcmp(parsedInput[0], "exit") == 0) /* exit built-in command */
    {
        if (parsedInput[1])
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return 0;
        }
        return 1;
    }
    else if (strcmp(parsedInput[0], "cd") == 0) /* cd built-in command */
    {
        commandStatus = changeDirectory(parsedInput, error_message);
    }
    else if (strcmp(parsedInput[0], "pwd") == 0) /* pwd built-in command */
    {
        commandStatus = getCurrentDirectory(parsedInput);
    }
    else if (strcmp(parsedInput[0], "path") == 0) /* path built-in command */
    {
        commandStatus = setPath(parsedInput);
    }
    else if (strcmp(parsedInput[0], "echo") == 0) /* echo command */
    {
        commandStatus = echoCommand(parsedInput);
    }
    else if (strcmp(parsedInput[0], "ls") == 0) /* ls command */
    {
        commandStatus = listDirectory(parsedInput, error_message);
    }
    else        /* other external commands like sh, rm etc. */
    {
        commandStatus = externalCommands(parsedInput, error_message);
    }
    return commandStatus;
}

int main(int argc, char *argv[])
{
    FILE *ifp;
    size_t len = 0;
    int status = 0;
    char *commandline = NULL, *fileLine = NULL;
    char error_message[30] = "An error has occurred\n";

    putenv("PATH=/bin");

    if (argc == 1) /* command-line */
    {
        do
        {
            printf("wish> ");
            getline(&commandline, &len, stdin);
            status = executeCommands(commandline, error_message);
        } while (status == 0);
    }
    else if (argc == 2) /* batch file */
    {
        ifp = fopen(argv[1], "r");
        if (ifp == NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        while (getline(&fileLine, &len, ifp) != -1)
        {
            status = executeCommands(fileLine, error_message);
            if (status == 1)
            {
                break;
            }
        }
    }
    else /* More than one file */
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    return 0;
}
