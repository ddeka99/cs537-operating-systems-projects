#include <stdio.h>  // IO operations
#include <stdlib.h>  // memory allocations
#include <string.h>  // string operations
#include <unistd.h>  // symbolic constants
#include <ctype.h>  // character operations
#include <sys/wait.h>  // for wait() system call
#include <fcntl.h>  // for access() system call

#define LINE_BUFSIZE 1024  // max number of bytes in an input line on my shell
#define ARG_BUFSIZE 32  // max number of space separated tokens in an input line on my shell
char *gp_errorMessage = "An error has occurred\n";
int g_errorReturn;  // file descriptor codes

void PrintError()
{
    g_errorReturn = write(STDERR_FILENO, gp_errorMessage, strlen(gp_errorMessage));
}

char *ReadLine()
{
    char *p_line;  // string input
    size_t BUFSIZE = LINE_BUFSIZE;
    p_line = (char *) malloc(LINE_BUFSIZE * sizeof(char));  // allocating space for the input string
    if(getline(&p_line, &BUFSIZE, stdin) == -1)  // error in reading line
    {
        //print error message
        PrintError();
        return NULL;
    }
    return p_line;
}

char **SplitLine(char *line)
{
    int numTokens = 0;  // number of arguments or tokens
    char **tokens = malloc(ARG_BUFSIZE * sizeof(char*));  //allocating space for the arguments
    char *token;  //each argument token

    if(strstr(line, ">") != NULL)
    {
        char *leftRedirection = strsep(&line, ">");
        char *rightRedirection = strsep(&line, ">");

        //extracting arguments
        while((token = strsep(&leftRedirection, " \t")) != NULL)
            tokens[numTokens++] = token;
        tokens[numTokens - 1][strcspn(tokens[numTokens - 1], "\n")] = 0;
        tokens[numTokens++] = ">";
        while((token = strsep(&rightRedirection, " \t")) != NULL)
            tokens[numTokens++] = token;
        tokens[numTokens - 1][strcspn(tokens[numTokens - 1], "\n")] = 0;
        tokens[numTokens] = NULL;
    }
    else
    {
        //extracting arguments
        while((token = strsep(&line, " \t")) != NULL)
            tokens[numTokens++] = token;
        tokens[numTokens - 1][strcspn(tokens[numTokens - 1], "\n")] = 0;
        tokens[numTokens] = NULL;
    }
    return tokens;
}

int IsBuiltInCommand(char *cmd)
{
    int numBuiltInCmds = 4, builtInCmdNo = 0;
    char *listBuiltInCmds[numBuiltInCmds];

    listBuiltInCmds[0] = "exit";
    listBuiltInCmds[1] = "cd";
    listBuiltInCmds[2] = "path";
    listBuiltInCmds[3] = "loop";

    for(int i = 0; i < numBuiltInCmds; i++)
    {
        if(strcmp(cmd, listBuiltInCmds[i]) == 0)
        {
            builtInCmdNo = i + 1;
            break;
        }
    }
    return builtInCmdNo;
}

int CheckCommand(char *p_path, char **dp_pathDir, char **dp_args)
{
    for(int i = 0; dp_pathDir[i] != NULL; i++)
    {
        // making the first argument as the absolute path to the exec file i.e. <path>/<filename>
        strcpy(p_path, dp_pathDir[i]);
        strcat(p_path, "/");
        strcat(p_path, dp_args[0]);
        // check if the exec file with absolute path exists
        if(access(p_path, X_OK) == 0)
            return 0;
    }
    return 1;
}

int ExecuteBuiltInCommand(char *p_cmd, char **dp_args, char **dp_pathDir, int cmdNo)
{
    int countArgs;
    int countArgsModified;
    int loopCount;
    char **dp_argsModified = malloc(ARG_BUFSIZE * sizeof(char*));

    switch(cmdNo)
    {
        // exit
        case 1:
            if(dp_args[1] != NULL)  // incorrect number of arguments
            {
                // print error message
                PrintError();
                return 1;
            }
            exit(0);
        // cd
        case 2:
            // early exit condition
            if(dp_args[1] == NULL)  // incorrect number of arguments (0)
            {
                PrintError();
                return 1;
            }
            else if(dp_args[2] != NULL)  // incorrect number of arguments (>1)
            {
                PrintError();
                return 1;    
            }
            else  
            {
                // if chdir did not succeed
                if(chdir(dp_args[1]) != 0)
                {
                    PrintError();
                    return 1;
                }
                else
                    return 0;
            }
        // path
        case 3:
            // overwriting path directory with passed arguments
            for(countArgs = 1; dp_args[countArgs] != NULL; countArgs++)
                dp_pathDir[countArgs - 1] = dp_args[countArgs];
            dp_pathDir[countArgs - 1] = NULL;
            return 0;

        // loop
        case 4:
            if(dp_args[1] == NULL)
            {
                PrintError();
                return 1;
            }
            else
            {
                loopCount = atoi(dp_args[1]);
                if(loopCount <= 0)
                {
                    PrintError();
                    return 1;
                }
            }
            countArgsModified = 0;
            for(int i = 2; dp_args[i] != NULL; i++)
            {
                dp_argsModified[i - 2] = strdup(dp_args[i]);
                countArgsModified++;
            }
            dp_argsModified[countArgsModified] = NULL;
            char p_loopCmdPath[128];
            for(int i = 0; i < loopCount; i++)
            {
                size_t pid = fork();
                if(pid < 0)
                {
                    PrintError();
                    exit(1);
                }
                else if(pid == 0)
                {
                    // replace $loop occurences with counter
                    for(int j = 0; j < countArgsModified; j++)
                    {
                        if(strcmp(dp_argsModified[j], "$loop") == 0)
                        {
                            int length = snprintf(NULL, 0, "%d", i + 1);
                            char* str = malloc(length + 1);
                            snprintf(str, length + 1, "%d", i + 1);
                            dp_argsModified[j] = strdup(str);
                        }
                    }
                    if(CheckCommand(p_loopCmdPath, dp_pathDir, dp_argsModified) == 0)
                        execv(p_loopCmdPath, dp_argsModified);
                    PrintError();
                    exit(1);
                }
                else
                    wait(NULL);
            }
    }
    return 0;
}

char *TrimWhiteSpace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int main(int argc, char **argv)
{
    if(argc > 2) // shell invoked with more than one file
    {
        PrintError();
        exit(1);
    }    
    FILE *fp;

    if(argc == 2)  // batch mode
    {
        if((fp = fopen(argv[1], "r")) == NULL)  //batch file does not exist
        {
            PrintError();
            exit(1);
        }
    }
    
    char *p_inputLine, **dp_args, *dp_pathDir[32];
    int builtInCmdNo;
    
    // initial default contents of path directory
    dp_pathDir[0] = strdup("/bin");
    dp_pathDir[1] = NULL;

    while(1)
    {
        if(argc == 2) // batch mode
        {
            size_t BUFSIZE = LINE_BUFSIZE;
            p_inputLine = (char *) malloc(LINE_BUFSIZE * sizeof(char));  // allocating space for the input string
            if(getline(&p_inputLine, &BUFSIZE, fp) == -1)  // error in reading line
            {
                fclose(fp);
                exit(0);
            }
        }
        else  // interactive mode
        {
            // print prompt message by writing into stdin
            printf("wish> ");
            p_inputLine = ReadLine();
        }

        int continueFlag = 0;
        // handling case where no command is written
        if((p_inputLine == NULL) || (strcmp(p_inputLine, "\n") == 0))
            continue;
        // handling case where input is all whitespaces
        for(int countWhite = 0; isspace(p_inputLine[countWhite]) != 0; countWhite++)
            if(countWhite == strlen(p_inputLine) - 1)
                continueFlag = 1;
        if(continueFlag == 1)
            continue;
        
        p_inputLine = TrimWhiteSpace(p_inputLine); // remove leading and trailing whitespaces

        // parse input line for arguments
        dp_args = SplitLine(p_inputLine);
        // handling commands
        char *p_cmd = strdup(dp_args[0]);
        builtInCmdNo = IsBuiltInCommand(p_cmd);
        if(builtInCmdNo > 0)  //built-in command
        {
            continueFlag = ExecuteBuiltInCommand(p_cmd, dp_args, dp_pathDir, builtInCmdNo);
            if(continueFlag == 1)
                continue;
        }
        else  //non built-in command
        {
            // check if command present in the path
            continueFlag = 1;
            char p_path[32];
            continueFlag = CheckCommand(p_path, dp_pathDir, dp_args);

            // if file not found, print error
            if(continueFlag == 1)
            {
                PrintError();
                continue;
            }

            // find redirection position in <one or more args> '>' <filename>
            int redirectionPos = -1, countArgs = 0;
            for(int i = 0; dp_args[i] != NULL; i++)
            {
                countArgs++;
                if(redirectionPos != -1)
                    continue;
                if(strcmp(dp_args[i], ">") == 0)
                    redirectionPos = i;
            }
            // illegal redirection position
            if(redirectionPos != -1 && redirectionPos != countArgs - 2)
            {
                PrintError();
                continue;
            }
            int pid = fork();
            if(pid == 0)  // non-builtin commands run in child process
            {
                if(redirectionPos == -1)  // no redirection
                {
                    execv(p_path, dp_args);
                    PrintError();
                    exit(1);
                }
                else  // redirection present
                {
                    char p_filename[128];
                    strcpy(p_filename, dp_args[countArgs - 1]);
                    int fd_out = open(p_filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
                    int fd_err = open(p_filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
                    if(fd_out < 0 || fd_err < 0)
                    {
                        PrintError();
                        exit(1);    
                    }
                    
                    int dup2_out = dup2(fd_out, STDOUT_FILENO);
                    int dup2_err = dup2(fd_err, STDERR_FILENO);
                    if(dup2_out < 0 || dup2_err < 0)
                    {
                        PrintError();
                        exit(1);   
                    }

                    // arguments including > and filename have to be removed
                    dp_args[countArgs - 1] = NULL;
                    dp_args[countArgs - 2] = NULL;

                    execv(p_path, dp_args);
                    PrintError();
                    exit(1);
                }
            }
            else if(pid > 0)  // parent, wait for child to complete its process
            {
                if((int) wait(NULL) != pid)
                {
                    PrintError();
                    exit(1);                   
                }
            }
            else  // error in forking 
            {
                PrintError();
                exit(1);
            }
        }
    }
}
