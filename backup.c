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

char **get_input(char *input)
{
    int index = 0, flag = 0, i = 0;
    char **command = malloc(sizeof(char *) * 20), *separator = " ", *parsed = NULL;
    if (command == NULL)
    {
        perror("malloc failed");
        exit(1);
    }

    parsed = strtok(input, separator); // (parsed[i] >= '!' && parsed[i] <= '/') || (parsed[i] >= ':' && parsed[i] <= '@' && parsed[i] != '>')

    for (i = 0; i < strlen(parsed); i++)
    {

        if ((parsed[i] >= 33 && parsed[i] <= 47 && parsed[i] != 46) || (parsed[i] >= 58 && parsed[i] <= 64 && parsed[i] != 62))
        {
            if (i == 0)
            {
                flag = 1;
                break;
            }
        }
    }

    if (flag == 1)
    {
        command = NULL;
    }
    else
    {
        while (parsed != NULL)
        {
            command[index] = parsed;
            index++;
            parsed = strtok(NULL, separator);
        }
        if (command[index - 1][strlen(command[index - 1]) - 1] == '\n')
        {
            command[index - 1][strlen(command[index - 1]) - 1] = '\0';
        }
        command[index] = NULL;
    }
    return command;
}

int parseLine(char *line)
{
    int i = 0, index = 0, syntaxError = 0, redirection = 0, fd = 0, ofd, stat_loc, parallelCount = 0;
    char error_message[30] = "An error has occurred\n", extraPath[2048], path[1024] = "PATH=", extraString[20468], extraParallelString[20468];
    char **command, *buf = (char *)malloc(100 * sizeof(char)), *ret, *env, *end, *checkpath = NULL, *splitPath = NULL, *checkRedirectionString = NULL, **redirectionArray = malloc(10 * sizeof(char));
    while (isspace((unsigned char)*line))
    {
        line++;
        if (*line == 0)
            break;
    }
    if (strlen(line) == 0)
    {
        return 0;
    }
    command = get_input(line);
    if (command == NULL)
    {
        return 0;
    }
    else if (strcmp(command[0], "exit") == 0)
    {
        if (command[1])
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return 0;
        }
        return 1;
    }
    else if (strcmp(command[0], "cd") == 0)
    {
        if (chdir(command[1]) < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        return 0;
    }
    else if (strcmp(command[0], "pwd") == 0)
    {
        getcwd(buf, 100);
        printf("%s\n", buf);
        return 0;
    }
    else if (strcmp(command[0], "path") == 0)
    {
        strcpy(extraPath, path);
        if (command[1])
        {
            i = 1;
            while (command[i + 1] != NULL)
            {
                strcat(extraPath, command[i]);
                strcat(extraPath, ":");
                i++;
            }
            strcat(extraPath, command[i]);
        }
        putenv(extraPath);
        return 0;
    }
    else if (strcmp(command[0], "echo") == 0)
    {
        if (strlen(command[1]) > 0)
        {
            ret = strchr(command[1], '$'); // check for env variables
            index = (int)(ret - command[1]);
            if (index == 0)
            {
                command[1]++;
                env = getenv(command[1]);
                if (env != NULL)
                {
                    printf("%s", env);
                }
            }
            else
            {
                i = 1;
                while (command[i] != NULL)
                {
                    while (isspace((unsigned char)*command[i])) // check for leading whitespaces
                        command[i]++;

                    if (*command[i] == 0) // All spaces?
                        break;

                    end = command[i] + strlen(command[i]) - 1;
                    while (end > command[i] && isspace((unsigned char)*end)) // check for trailing whitespaces
                        end--;

                    end[1] = '\0';
                    printf("%s", command[i]);
                    if (command[i + 2] != NULL)
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
    else if (strcmp(command[0], "ls") == 0)
    {
        syntaxError = 0, redirection = 0, index = 0;
        while (command[index] != NULL) /* check for invalid redirection */
        {
            if ((strcmp(command[index], ">") == 0) &&
                (index == 0 || command[index + 1] == NULL || strcmp(command[index + 1], "") == 0 ||
                 strcmp(command[index + 1], ">") == 0 || strcmp(command[index + 2], ">") == 0 || command[index + 2] != NULL))
            {
                syntaxError = 1;
                write(STDERR_FILENO, error_message, strlen(error_message));
                break;
            }
            index++;
        }
        if (syntaxError == 1)
        {
            return 0;
        }
        checkpath = getenv("PATH");
        if (strlen(checkpath) == 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else
        {
            splitPath = strtok(checkpath, ":"); /* check for path */
            while (splitPath != NULL)
            {
                strcpy(extraString, splitPath);
                strcat(extraString, "/ls");
                fd = access(extraString, X_OK);
                if (fd != -1)
                {
                    if (command[1] && (strchr(command[1], '>') > 0)) /* no spaces between redirection */
                    {
                        redirection = 1;
                        checkRedirectionString = strtok(command[1], ">");
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
                        if (execv(extraString, command) < 0)
                        {
                            perror(command[0]);
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
                splitPath = strtok(NULL, ":");
            }
            if (fd == -1)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
        return 0;
    }
    else
    {
        checkpath = getenv("PATH");
        if (strlen(checkpath) == 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else if (strchr(command[0], '&') > 0)
        { 
            int parallelIndex = 0;
            char parallelSpace[20480];
            strcpy(parallelSpace, command[0]);
            char *parallelSeparator = strtok(parallelSpace, "&");
            char **parallelSeparatorArray = malloc(sizeof(char *) * 20);
            while (parallelSeparator != NULL)
            {
                parallelSeparatorArray[parallelIndex] = parallelSeparator;
                parallelIndex++;
                parallelSeparator = strtok(NULL, "&");
            }
            parallelSeparatorArray[parallelIndex] = NULL;
            pid_t child_pids[parallelIndex];
            for (i = 0, index = 0; i < parallelIndex && parallelSeparatorArray[index] != NULL; i++, index++)
            {
                strcpy(extraParallelString, "tests/");
                strcat(extraParallelString, parallelSeparatorArray[index]);
                fd = access(extraParallelString, X_OK);
                if (fd != -1)
                {
                    child_pids[i] = fork();
                    if (child_pids[i] < 0)
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(1);
                    }
                    if (child_pids[i] == 0)
                    {
                        if (execv(extraParallelString, parallelSeparatorArray) < 0)
                        {
                            perror(parallelSeparatorArray[index]);
                        }
                    }
                    else
                    {
                        waitpid(child_pids[i], &stat_loc, WUNTRACED);
                    }
                    // break;
                }
                if (fd == -1)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
            return 0;
        } 
        else
        {
            i = 0;
            while (command[i] != NULL) /* check for parallel commands */
            {
                if ((strcmp(command[i], "&") == 0) && (command[i + 1] != NULL))
                {
                    parallelCount++;
                }
                i++;
            }
            pid_t child_pids[parallelCount + 1];
            for (i = 0, index = 0; i < parallelCount + 1 && command[index] != NULL; i++, index = index + 2)
            {
                strcpy(extraString, "tests/");
                strcat(extraString, command[index]);
                fd = access(extraString, X_OK);
                if (fd != -1)
                {
                    child_pids[i] = fork();
                    if (child_pids[i] < 0)
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(1);
                    }
                    if (child_pids[i] == 0)
                    {
                        if (execv(extraString, command) < 0)
                        {
                            perror(command[index]);
                        }
                    }
                    else
                    {
                        waitpid(child_pids[i], &stat_loc, WUNTRACED);
                    }
                    // break;
                }
                if (fd == -1)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char error_message[30] = "An error has occurred\n";
    char *line = NULL, *fileLine = NULL;
    size_t len = 0;
    int status = 0;
    FILE *ifp;

    putenv("PATH=/bin");

    if (argc == 1) /* command-line */
    {
        do
        {
            printf("wish> ");
            getline(&line, &len, stdin);
            status = parseLine(line);
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
            status = parseLine(fileLine);
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