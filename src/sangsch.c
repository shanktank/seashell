#include "sangsch.h"

void printWelcomeScreen();
void printShellPrompt();
void getTextFromShell();

void getUserInput();
void populateGetArgs();
void destroyBuffer();
void appendEnv();
char *checkForDollar(char *argv[], int argc);
int checkForString(char *args[], int argCount, char *target);
int dIndex;

void printShell(char *arg[])
{
    int i;
    printf("==============\n");
    for (i = 0; arg[i] != NULL; ++i)
        printf("%s ", arg[i]);
    printf("\n============\n\n");
}

int main(int argc, char *argv[])
{
    printWelcomeScreen();
    while(1) {
        printShellPrompt();
        userInput = getchar();
        appendEnv();
        int status, fdin = 0, fdout = 1, pipeStatus = 0;
        switch(userInput) {
            case '\n':
                break;
            default:
                getUserInput();
                // Returns argv and argc as shellArgv, shellArgc

                // Check for >>, <, >, |
                while ((checkForString(shellArgv, shellArgc, ">>") == 0) 
                  || checkForString(shellArgv, shellArgc, ">") == 0)
                    fdout = open(redirect.file, redirect.oFlags, 0600);
                while ((checkForString(shellArgv, shellArgc, "<") == 0))
                    fdin = open(redirect.file, redirect.oFlags);

                if (checkForString(shellArgv, shellArgc, "|") == 0) {
                    if (pipe(pipeS.pipe) < 0) perror("Pipe failed: ");
                    pipeStatus = 1;
                }

                char *dCommand = checkForDollar(shellArgv, shellArgc);
                if (dCommand != NULL) {
                    // Execute the command
                    pid_t pid;
                    char buffer[256];
                    char *arg;
                    int fd;
                    int status;
                    int n;
                    fd = open("tempFile", O_RDWR | O_CREAT, 0666);
                    pid = fork();
                    switch(pid) {
                        case -1: 
                            printf("Fork failed\n");
                            exit(1);
                            break;
                        case 0:
                            dup2(fd, 1);
                            n = execlp(dCommand, dCommand, NULL);
                            if (n == -1) {printf("Not executed"); exit(1);}
                        default:
                            wait(&status);
                            n = open("tempFile", O_RDONLY);
                            read(n, buffer, 256);
                            /*
                            shellArgc--;
                            shellArgv[dIndex] = '\0';
                            arg = strtok(buffer, "\n");
                            while(arg != NULL && shellArgc < 5) {
                                shellArgv[dIndex] = arg;
                                dIndex++;
                                shellArgc++;
                                arg = strtok(NULL, "\n");
                            }
                            while(arg != NULL) {
                                shellArgv[dIndex++] = arg;
                                arg = strtok(NULL, "\n");
                                shellArgc++;
                            }
                            printShell(shellArgv);
                            
                            */

                            shellArgc--;
                            shellArgv[dIndex] = '\0';
                            arg = strtok(buffer, "\n");
                            while (arg != NULL) {
                                int stat;
                                pid_t pid;
                                pid = fork();
                                shellArgv[dIndex] = arg;
                                dIndex++;
                                shellArgc++;

                                if (pid == 0)
                                    execlp(shellArgv[0], shellArgv[0], arg, NULL);
                                else if (pid > 0) 
                                    wait(&stat);
                                arg = strtok(NULL, "\n");
                            }

                            remove("tempFile");
                            break;
                    }

                }


                pid_t pid, pid2;
                pid = fork();
                if (pid < 0) {
                    perror("Fork failed due to: ");
                    exit(1);
                } 
                else if (pid == 0) {
                    if (redirect.file) 
                        if (redirect.output == 1) {
                            dup2(fdout, 1);
                            close(fdout);
                        }
                        if (redirect.input == 1) {
                            dup2(fdin, 0);
                            close(fdin);
                        }
                    if (pipeStatus == 1) {
                        dup2(pipeS.pipe[1], 1);
                        close(pipeS.pipe[1]);
                        close(pipeS.pipe[0]);
                        execvp(pipeS.inPipe, pipeS.inPipeArgs);
                        // if execvp returns negative
                        printf("Not valid command\n");
                        exit(1);
                    }
                    execvp(shellArgv[0], shellArgv);
                    printf("Not valid command\n");
                }
                if (pipeStatus == 1) {
                    pid2 = fork();
                    if (pid2 == 0) {
                        dup2(pipeS.pipe[0], 0);
                        close(pipeS.pipe[0]);
                        close(pipeS.pipe[1]);
                        execvp(pipeS.outPipe, pipeS.outPipeArgs);
                        printf("Not valid command\n");
                        exit(1);
                    }
                    close(pipeS.pipe[0]);
                    close(pipeS.pipe[1]);
                }
                if (pipeStatus == 1)
                    while(pid + pid2 > 0) {
                        int p = wait(NULL);
                        if (p == pid) pid = 0;
                        if (p == pid2) pid2 = 0;
                    }
                else
                    wait(&status);
                if (redirect.output == 1) close(fdout);
                if (redirect.input == 1) close(fdin);
                break;
        }
    }
}

int checkForString(char *args[], int argCount, char *target) 
{
    int i;
    int result;

    for (i = 0; i < argCount; ++i) {
        if ((result = strcmp(args[i], target)) == 0) {
            if (args[i+1] == NULL) { 
                return -1;
            }
            else if (strcmp(args[i+1], target) == 0) {
                fprintf(stdout, "Syntax error near %s \n", args[i]);
                return -1;
            }
            else {
                // Next argument is valid
                if (strcmp(target, ">>") == 0) {
                    redirect.oFlags = O_WRONLY | O_CREAT | O_APPEND;
                    redirect.output = 1;
                }
                else if (strcmp(target, ">") == 0) {
                    redirect.oFlags = O_WRONLY | O_CREAT | O_TRUNC;
                    redirect.output = 1;
                }
                else if (strcmp(target,"<") == 0) {
                    redirect.oFlags = O_RDONLY;
                    redirect.input = 1;
                }
                else if (strcmp(target, "|") == 0) {
                    int strlength;
                    int j, k = 0;
                    strlength = strlen(args[0]) + 1;
                    pipeS.inPipe = (char *)malloc(strlength);
                    strcpy(pipeS.inPipe, args[0]);
                    for (j = 0; j < i; ++j)
                        pipeS.inPipeArgs[j] = args[j];
                    pipeS.inPipeArgs[j] = NULL;

                    strlength = strlen(args[i+1]) + 1;
                    pipeS.outPipe = (char *)malloc(strlength);
                    strcpy(pipeS.outPipe, args[i+1]);
                    for (j = i + 1; j < argCount; ++j)
                        pipeS.outPipeArgs[k++] = args[j];
                    pipeS.outPipeArgs[k] = NULL;
                }
                // Shift all the args down 1
                // argv = [cat, foo, >>, foo1]
                // argv = [cat, foo] -> stdout points to foo
                // Check for 1 off error when I get some sleep :)
                if (redirect.output == 1 || redirect.input == 1) {
                    int strlength = strlen(args[i+1])+1;
                    redirect.file = (char *)malloc(strlength);
                    strcpy(redirect.file, args[i+1]);
                }
                // This won't work for second half. Have to implement 
                // If the second half is longer than 1, eliminate the rest
                // For piping :)
                int j;
                for (j = i; j < shellArgc-1; ++j) 
                    args[j] = args[j+2];
                shellArgc -= 2;
                return 0;
            }
        }
    }
    return -1;
}

char *checkForDollar(char *argv[], int argc)
{
    // Argv should be processed at this point
    char *arg;
    int i = 0;
    while(argv[i] != NULL) {
        if ((arg = strpbrk(argv[i], "$")) != NULL)
            break;
        ++i;
    }
    if (i != argc) {// means found
        int len = strlen(argv[i]);
        if (argv[i][0] != '$') return NULL;
        if (argv[i][1] != '\0' && argv[i][1] != '(') return NULL;
        if (argv[i][len-1] != '\0' && argv[i][len-1] != ')') return NULL;
        char *c;
        c = strchr(argv[i], '(');
        char *t = (char *)malloc(sizeof(argv[i]));;
        strncpy(t, ++c, len-3);
        dIndex = i;
        return t;
    }
    return NULL;
}

void getUserInput()
{
    destroyBuffer();
    while ((userInput != '\n') && (bufferCount < BUFFER_MAX)) {
        buffer[bufferCount++] = userInput;
        userInput = getchar();
    }
    buffer[bufferCount] = '\0';
    populateGetArgs();
}
void populateGetArgs()
{
    char *argPointer;
    argPointer = strtok(buffer, " ");
    while (argPointer != NULL) {
        shellArgv[shellArgc++] = argPointer;
        argPointer = strtok(NULL, " ");
    }
    shellArgv[shellArgc] = NULL;
}
void destroyBuffer()
{
    int i = 0;
    while(shellArgv[i]) shellArgv[i++] = NULL;
    i = 0;
    while(pipeS.inPipeArgs[i] || pipeS.outPipeArgs[i]) {
        pipeS.inPipeArgs[i] = pipeS.outPipeArgs[i] = NULL;
        ++i;
    }
    if (redirect.file) { redirect.file = "\0";}
    redirect.input = redirect.output = redirect.oFlags = 0;
    pipeS.pipe[0] = pipeS.pipe[1] = -1;
    shellArgc = bufferCount = dIndex = 0;
}

void printWelcomeScreen()
{
    int i;

    printf("\n-------------------------------------------------\n");
    printf(ANSI_COLOR_GREEN "\tsangsch designed and implemented by: \n" ANSI_COLOR_RESET);
    for (i = 0; people[i] != NULL; ++i)
        printf(ANSI_COLOR_BLUE "\t\t%s\n", people[i]);
    printf(ANSI_COLOR_RESET "-------------------------------------------------\n");
    printf("\n\n");

}

void printShellPrompt()
{
    user = getpwuid(geteuid());
    printf(ANSI_COLOR_CYAN "%s $ " ANSI_COLOR_RESET , user->pw_name);
}

void appendEnv()
{
    char localEnv[256];
    char *localEnvp;
    const char *path = ":/Users/sang/Desktop/C++/sangsch/bin";
    localEnvp = getenv("PATH");
    strcpy(localEnv, localEnvp);
    strcat(localEnv, path);
    setenv("PATH", localEnv, 1);
}
