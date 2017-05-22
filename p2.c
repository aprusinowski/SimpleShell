/*
 * p2.c *
 *
 *      Author: Artur Prusinowski
 *      Class: CS570
 *      Instructor: Dr. Carroll
 *      Assignment: Program 4, Due 04/23/2016
 *
 *      Implementation of the p2 shell.
 */

#include "p2.h"

int main(void) {

    int i = 0;

    //Holds 1 or 2 struct Command items depending on whether I/O pipe is used   
    CMD userCommand[2];

    char* argv[MAXITEM];
    for (i = 0; i < MAXITEM + 1; i++)
        argv[i] = malloc(STORAGE * sizeof (char));

    signal(SIGTERM, signalHandler);

    for (;!EXIT_PROGRAM;) {

        if (AMPERSAND)eraseLine(stdin); //clear remainder of input line
        (void) printf("p2: ");
        resetFlags();
        resetProcs(userCommand);

        if ((parse(argv, userCommand)) == PARSE_ERR)
            continue;
        if (PIPE) {
            CHK(pipe(filedes));
            /*Set PIPE flags on per process level.Individual process is unaware of
             its position in the input and must be told whether to setup input or
             output pipe*/
            userCommand[PROC_1].PIPE_OUT = true;
            userCommand[PROC_2].PIPE_IN = true;
        }

        for (i = 0; i <= PIPE; i++) {
            userCommand[i].execute(&userCommand[i]);
            CLOSE_PIPE = true;
        }
    }

    for (i = 0; i < MAXITEM + 1; i++)
        free(argv[i]);

    killpg(getpid(), SIGTERM);
    (void) printf("p2 terminated.\n");
}

/*******************************************************************************
 * int Parse(char **, struct Command*)                                         *
 *                                                                             *
 * Function parses input stream and fills in the components of                 *
 * struct Command in an attempt to create an exectutable command.              *
 * PARAMETERS:                                                                 *
 * **argv     -- Pointer to character arrays. Each array contains single       *
 *               string of user input                                          *
 *                                                                             *
 * *Indexes   --Pointer of type struct Command which contains all components of*
 *              a user entered command.                                        *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 if Parse was able to assemble an executable command. Otherwise returns    *
 * PARSE_ERR.                                                                  *
 ******************************************************************************/
int parse(char ** argv, CMD Indexes[]) {

    int i = 0;
    int argc = 0;
    int tokenCount = 0;
    int process_no = PROC_1;

    for (; tokenCount <= MAXITEM;) {
        argc = getword(argv[tokenCount]);

        //EOF Terminate P2            
        if TERMINATE(argc, tokenCount) {
            EXIT_PROGRAM = true;
            return PARSE_ERR;
        }

        //Check if a string was read in. argc > 0        
        if IS_PARSABLE(argc) {

            /*if variable detected swap variable for value or return PARSE_ERR if
             not found*/
            if (ENV_VAR_VALUE)
                if (getEnvVar(argv[tokenCount]) == PARSE_ERR)
                    return PARSE_ERR;

            //if string was "&" set flag           
            if IS_AMPERSAND(argv[tokenCount][0])
                AMPERSAND = true;
                //if I/O pipe detected
            else if IS_PIPE(argv[tokenCount][0]) {
                PIPE = !PIPE; //PIPE should be 0 at this point
                PIPE_POS = (PIPE) ? tokenCount : none;
                //if !PIPE, PIPE was previously set to 1. Set MULT_PIPE error flag
                if ((MULT_PIPE = (!PIPE) ? true : false) == true)break;
            } else
                tokenCount++;
        }

        /*If blank line entered, or line with ';' or '&' only.
         Return and reissue prompt*/
        if BLANK_LINE(argc, tokenCount) {
            BLANK_INPUT = true;
            break;
        }
        /*Line of input entered. Stop reading. Begin parsing
          END_OF_INPUT(C,B,D) (  ( (C==0 || C==-1) && B > 0) || D==1) */
        if END_OF_INPUT(argc, tokenCount, AMPERSAND)
            break;

        //ENV_VAR_VALUE = false;
    }

    /********End of reading input stream. Begin constructing executable command******/

    //if input was entered initially assume command was incomplete 
    if (tokenCount > 0)INCOMPLETE_COMMAND = true;

    //if nothing follows | symbol on input line
    if INCOMPLETE_PIPE(tokenCount, PIPE_POS) {
        NULL_COMMAND = true;
        tokenCount = 0;
    }

    /*then loop through all strings entered on a line of input. 
     * tokenCount contains number of strings read in from command line not
     *including ';' and '&'
     * */
    for (; i < tokenCount; i++) {

        //if reached PIPE position switch to Process 2
        if (i == PIPE_POS)process_no = PROC_2;

        if (strcmp(argv[i], ">") == 0) {
            //in case of output redirect, initially assume no redirect file specified
            INCOMPLETE_OUT_REDIRECT = true;
            /*next item in should be filepath to redirect STDOUT to. Advance 
              counter and as long as there us more strings left, check to 
              make sure string is not a metacharacter */
            if (++i < tokenCount && !isSpecialString(argv[i])) {
                //since there is a string left, treat it as the filepath
                INCOMPLETE_OUT_REDIRECT = false;

                /*Check if RD_OUT flag has already been set. If it has we have 
                  an ambiguous out redirect condition.
                  
                  If PIPE not set PROC_2*PIPE = 0 and the first and only process has 
                  output redirected.
                  If PIPE set PROC_2*PIPE = 1 and output redirection is applied to
                  second process.                 */
                if (Indexes[PROC_2 * PIPE].RD_OUT) {
                    INCOMPLETE_COMMAND = false;
                    AMBIGUITY = true;
                    break;
                }
                Indexes[PROC_2 * PIPE].filePathOut = argv[i];
                Indexes[PROC_2 * PIPE].RD_OUT = true;
            }
            /*Algorithm is same as above for redirect out*/
        } else if (strcmp(argv[i], "<") == 0) {
            INCOMPLETE_IN_REDIRECT = true;
            if (++i < tokenCount && !isSpecialString(argv[i])) {
                INCOMPLETE_IN_REDIRECT = false;

                /*Input redirection,if requested,will always apply to first process*/
                if (Indexes[PROC_1].RD_IN) {
                    INCOMPLETE_COMMAND = false;
                    AMBIGUITY = true;
                    break;
                }
                Indexes[PROC_1].filePathIn = argv[i];
                Indexes[PROC_1].RD_IN = true;
            }
            /*If more strings left, since they are not part of I/O redirection,
             * assume they are part of the executable command */
        } else if (i < tokenCount) {
            Indexes[process_no].args[Indexes[process_no].argsCount++]=argv[i];
            INCOMPLETE_COMMAND = false;
        }
    }    
    /*If any error flags were thrown, print error and return to main() */
    if (parseErrors())
        return printParseError(NULL);

        /*If no errors, close the array of args with NULL in last element
         *as required by execvp() function then assign the appropriate command 
         * execution function to the function pointer element of struct Command */

    else {
        process_no = PROC_1;
        /*Executed once if no PIPE, twice if PIPE set*/
        for (i = 0; i <= PIPE; i++) {
            Indexes[process_no].args[Indexes[process_no].argsCount] = NULL;

            if (strcmp(Indexes[process_no].args[0], "cd") == 0)
                Indexes[process_no].execute = &execDirCommnd;

            else if (strcmp(Indexes[process_no].args[0], "ls-F") == 0)
                Indexes[process_no].execute = &execLsFCommnd;

            else if (strcmp(Indexes[process_no].args[0], "printenv") == 0)
                Indexes[process_no].execute = &execPrintEnvCommnd;

            else if (strcmp(Indexes[process_no].args[0], "setenv") == 0)
                Indexes[process_no].execute = &execSetEnvCommnd;

            else if (AMPERSAND)
                Indexes[process_no].execute = &execBackground;

            else
                Indexes[process_no].execute = &execForeground;

            //switch to process 2. If loop runs once this line is harmless
            process_no = PROC_2;
        }
    }

    return 0;
}

/******************************************************************************
 *int execBackground(struct Command*)                                          *
 *                                                                             *
 * Function executes command in struct Command as a new background process     *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *userCommand --Pointer of type struct Command which contains all components*
 *               of a user entered command.                                    *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 once a child process has been spawned                                     *
 ******************************************************************************/
int execBackground(CMD *userCommand) {
    char *tmpInStream;
    fflush(stderr);
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        /*If RD_IN flag thrown, redirect STDIN to the specified file, otherwise
        redirect to "/dev/null" */
        tmpInStream=(userCommand->RD_IN)?userCommand->filePathIn :"/dev/null";
        if ((rd_stdin(tmpInStream) == 1))
            exit(1);

        if (userCommand->RD_OUT)
            if ((rd_stdout(userCommand->filePathOut) == 1))
                exit(1);

        /*Setup IO pipe if necessary*/
        if (userCommand->PIPE_IN)
            setupPipe(STDIN_FILENO);
        else if (userCommand->PIPE_OUT)
            setupPipe(STDOUT_FILENO);

        if (execvp(userCommand->args[0], userCommand->args) == -1)
            exit(printExecError(errno, userCommand->args[0]));
    } else
        (void) printf("%s [%ld]\n", userCommand->args[0], pid);

    if (CLOSE_PIPE) {
        CHK(close(filedes[0]));
        CHK(close(filedes[1]));
    }

    //prevents child from printing on same line as p2: prompt
    usleep(100000);
    return 0;
}

/******************************************************************************
 *int execForeground(struct Command*)                                          *
 *                                                                             *
 * Function executes command in struct Command as a new foreground process     *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *userCommand --Pointer of type struct Command which contains all components*
 *               of a user entered command.                                    *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 upon completion of execution of the child process                         *
 ******************************************************************************/
int execForeground(CMD *userCommand) {
    fflush(stderr);
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        if (userCommand->RD_IN)
            if ((rd_stdin(userCommand->filePathIn) == 1))
                exit(1);

        if (userCommand->RD_OUT)
            if ((rd_stdout(userCommand->filePathOut) == 1))
                exit(1);

        /*Setup IO pipe if necessary*/
        if (userCommand->PIPE_IN)
            setupPipe(STDIN_FILENO);
        else if (userCommand->PIPE_OUT)
            setupPipe(STDOUT_FILENO);

        if (execvp(userCommand->args[0], userCommand->args) == -1)
            exit(printExecError(errno, userCommand->args[0]));
    } else {
        if (CLOSE_PIPE) {
            CHK(close(filedes[0]));
            CHK(close(filedes[1]));
        }
        while (wait(NULL) != pid);
    }

    return 0;
}

/******************************************************************************
 *int execDirCommnd(struct Command*)                                           *
 *                                                                             *
 * Function executes cd command with parameters contained in struct Command    *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *userCommand --Pointer of type struct Command which contains all components*
 *               of a user entered command.                                    *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 upon successful execution of the cd command. 1 if any errors were         *
 * encountered                                                                 *
 ******************************************************************************/
int execDirCommnd(CMD *cd) {
    int tmpErr_no = false;

    const char *home = "HOME";
    if (cd->argsCount == 1) {
        cd->args[1] = getenv(home);
        cd->argsCount++;
    }

    if (cd->argsCount == 2)
        /*if errors in chdir() assign errno to tmpErr_no, otherwise  false. 
          Saving state of errno in case changes before printing error message */
        tmpErr_no = (chdir(cd->args[1]) == -1) ? errno : false;
    else
        tmpErr_no = ARG_COUNT_ERR;

    //if no errors return 0 otherwise send error message. printExecError() returns 1
    return (tmpErr_no) ? printExecError(tmpErr_no, cd->args[1]) : 0;
}

/******************************************************************************
 *int execLsFCommnd(struct Command*)                                           *
 *                                                                             *
 * Function executes ls-F command with parameters contained in struct Command  *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *userCommand --Pointer of type struct Command which contains all components*
 *               of a user entered command.                                    *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 upon successful execution of the ls-F command. 1 if any errors were       *
 * encountered                                                                 *
 ******************************************************************************/
int execLsFCommnd(CMD *lsF) {
    char fullPath[STORAGE];
    char *dir;
    DIR *dirp;
    struct dirent *dp;
    struct stat sb;

    //assign either current dir or argument value as argument to ls-F
    dir = (lsF->argsCount == 1) ? "." : lsF->args[1];

    //if no error, see if argument is not a directory.If it is not print the
    //name and exit
    if ((stat(dir, &sb)) != -1)
        if (!S_ISDIR(sb.st_mode)) //((sb.st_mode & S_IFMT) != S_IFDIR)
            return (printExecError(NOT_DIR, dir));

    //Now try to open the directory. if error, print error and exit
    if ((dirp = opendir(dir)) == NULL)
        return (printExecError(errno, dir));

    //loop through dir printing out entries
    while (dirp) {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {

           /*Assemble complete path string. If ls-F command applied to dir other than
              current working dir, lstat() and stat() require full path*/
            (void)snprintf(fullPath, sizeof (fullPath),"%s/%s",dir,dp->d_name);
            lstat(fullPath, &sb);
            switch ((sb.st_mode & S_IFMT)) {

                    //if entry is a directory
                case S_IFDIR: (void) printf("%s/\n", dp->d_name);
                    break;

                    //if entry is a regular file, check if executable  
                case S_IFREG:
                    if (sb.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR))
                        (void)printf("%s*\n", dp->d_name);
                    else (void) printf("%s\n", dp->d_name);
                    break;

                    //if entry is a link, try to follow it with stat()  
                case S_IFLNK: stat(fullPath, &sb);
                    if (errno == ENOENT || errno == EACCES)
                        (void)printf("%s&\n", dp->d_name);
                    else(void) printf("%s@\n", dp->d_name);
            }
        } else {
            closedir(dirp);
            break;
        }
    }
    return 0;
}

/******************************************************************************
 *int execPrintEnvCommnd(struct Command*)                                      *
 *                                                                             *
 * Function executes printenv command with parameters contained in struct      *
 * Command                                                                     *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *printenv --Pointer of type struct Command which contains all components   *
 *               of a user entered command.                                    *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 upon successful execution of the printenv command. 1 if any errors were   *
 * encountered                                                                 *
 ******************************************************************************/
int execPrintEnvCommnd(CMD *printenv) {
    char* ptr;
    if (printenv->argsCount != 2)
        return printExecError(EINVAL, "printenv");
    else
        /*Print value of variable or if printenv returns NULL print empty string*/
        (void) printf("%s\n", (ptr = getenv(printenv->args[1])) ? ptr : "");
    return 0;
}

/******************************************************************************
 *int execSetEnvCommnd(struct Command*)                                        *
 *                                                                             *
 * Function executes setenv command with parameters contained in struct        *
 * Command                                                                     *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *setenv -- Pointer of type struct Command which contains all components    *
 *               of a user entered command.                                    *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 upon successful execution of the setenv command. 1 if any errors were     *
 * encountered                                                                 *
 ******************************************************************************/
int execSetEnvCommnd(CMD* setenv) {
    char *arg;

    if (setenv->argsCount != 3)
        return printExecError(EINVAL, "setenv");
    else {
        (void) asprintf(&arg, "%s=%s", setenv->args[1], setenv->args[2]);
        /*if putenv() returns nonzero value, error occured*/
        if (putenv(arg))printExecError(errno, "setenv");
    }

    return 0;
}

/******************************************************************************
 *int getEnvVar(char enVar*)                                                   *
 *                                                                             *
 * Function swaps environment variable passed as argument with its value       *
 * Command                                                                     *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *enVar  -- Pointer of type char containing the environment variable        *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 if variable was found and its and PARSE_ERR otherwise                     *
 ******************************************************************************/
int getEnvVar(char* enVar) {
    char tmp[STORAGE];
   /*store variable name.In case error in retrieving value,pass to printParseError()*/
    strcpy(tmp, enVar);

    char *val = getenv(enVar + 1);
    strcpy(enVar, (val == NULL) ? "" : val);

    if (val == NULL) {
        INVALID_ENV_VAR = true;  
        return printParseError(tmp + 1);
    }
    return 0;
}

/******************************************************************************
 *int rd_stdout(char*)                                                         *
 *                                                                             *
 * Function attempts to redirect STDOUT to file specified in argument          *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *filePathOut --Pointer of type char which contains the filename, or full   *
 *                file path where STDOUT is to be redirected                   *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 upon successful redirection of STDOUT. 1 if errors were encountered       *
 ******************************************************************************/
int rd_stdout(char* filePathOut) {
    int output_File;
    if ((output_File = open(filePathOut, FILE_CREAT_FL)) < 1)
        return printExecError(errno, filePathOut);
    else {
        dup2(output_File, STDOUT_FILENO);
        close(output_File);
    }
    return 0;
}

/******************************************************************************
 *int rd_stdin(char*)                                                          *
 *                                                                             *
 * Function attempts to redirect STDIN  to file specified in argument          *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  *filePathIn  --Pointer of type char which contains the filename, or full   *
 *                file path where STDIN is to be redirected from               *
 *                                                                             *
 * RETURNS:                                                                    *
 * 0 upon successful redirection of STDIN.  1 if errors were encountered       *
 *                                                                             *
 ******************************************************************************/
int rd_stdin(char* filePathIn) {
    int input_File;
    if ((input_File = open(filePathIn, O_RDONLY)) < 1)
        return printExecError(errno, filePathIn);

    else {
        dup2(input_File, STDIN_FILENO);
        close(input_File);
    }
    return 0;
}

/*******************************************************************************
 *int printExecError(int , char *)                                             *
 *                                                                             *
 * In case where the argument passed to int err_no is equal to either NOT_DIR, *
 * or ARG_COUNT_ERR, as defined in p2.c. Appropriate message is sent to        *
 * STDOUT/STDERR. In all other cases perror(char*) function is invoked.        *
 * PARAMETERS:                                                                 *
 *                                                                             *
 *  err_no -- error number as defined in errno.h, NOT_DIR, or ARG_COUNT_ERR    *
 *            as defined in p2.c *                                             *
 *                                                                             *
 *  *name  -- string containing the error invoking argument                    *
 *                                                                             *
 * RETURNS:   1                                                                *
 ******************************************************************************/
int printExecError(int err_no, char *name) {
    errno = err_no;
    if (errno == NOT_DIR)
        (void)printf("%s\n", name);

    else if (errno == ARG_COUNT_ERR)
        (void)fprintf(stderr, "command takes 0 or 1 argument.\n");
    else
        perror(name);

    return 1;
}

/*******************************************************************************
 *int printParseError()                                                        *
 *                                                                             *
 * Function prints error messages to STDERR depending on error flags           *
 * thrown up by the parse(char**, struct Command*) function.                   *
 *                                                                             *
 * PARAMETERS: No Parameters                                                   *
 *                                                                             *
 * RETURNS:   PARSE_ERR. (Defined in P2.c)                                     *
 ******************************************************************************/
int printParseError(char *name) {
    if (INCOMPLETE_COMMAND == true) {
        (void) fprintf(stderr, "No command specified.\n");
        return PARSE_ERR;
    }
    if (AMBIGUITY == true) {
        (void) fprintf(stderr, "Ambiguous redirect.\n");
        return PARSE_ERR;
    }
    if (INCOMPLETE_OUT_REDIRECT == true) {
        (void) fprintf(stderr, "Output redirection filename not specified.\n");
        return PARSE_ERR;
    }
    if (INCOMPLETE_IN_REDIRECT == true) {
        (void) fprintf(stderr, "Input redirection filename not specified.\n");
        return PARSE_ERR;
    }
    if (UM_QUOTE == true) {
        (void) fprintf(stderr, "Unmatched quotes.\n");
        return PARSE_ERR;
    }
    if (MULT_PIPE == true) {
        eraseLine(stdin);
        (void) fprintf(stderr, "Error, multiple pipes.\n");
        return PARSE_ERR;
    }
    if ((AMPERSAND == true || PIPE == true) && BLANK_INPUT == true) {
        eraseLine(stdin);
        (void) fprintf(stderr, "Invalid null command.\n");
        return PARSE_ERR;
    }
    if (NULL_COMMAND == true) {
        eraseLine(stdin);
        (void) fprintf(stderr, "Invalid null command.\n");
        return PARSE_ERR;
    }
    if (INVALID_ENV_VAR == true) {
        eraseLine(stdin);
        (void) fprintf(stderr, "%s:Undefined variable.\n", name);
        return PARSE_ERR;
    }


    return PARSE_ERR;
}

/*******************************************************************************
 *void resetFlags()                                                            *
 *                                                                             *
 * Function resets flags thrown up by the parse(char**, struct Command)        *
 *                                                                             *
 * PARAMETERS: No Parameters                                                   *
 *                                                                             *
 * RETURNS: No return value                                                    *
 ******************************************************************************/
void resetFlags() {
    AMPERSAND = false;
    AMBIGUITY = false;
    INCOMPLETE_COMMAND = false;
    INCOMPLETE_IN_REDIRECT = false;
    INCOMPLETE_OUT_REDIRECT = false;
    BLANK_INPUT = false;
    UM_QUOTE = false;
    PIPE = false;
    MULT_PIPE = false;
    PIPE_POS = none;
    CLOSE_PIPE = false;
    NULL_COMMAND = false;
    INVALID_ENV_VAR = false;
    ENV_VAR_VALUE = false;
}

/*******************************************************************************
 *void resetProcs()                                                            *
 *                                                                             *
 * Function resets process specific flags and values                           *
 *                                                                             *
 * PARAMETERS: No Parameters                                                   *
 *                                                                             *
 * RETURNS: No return value                                                    *
 ******************************************************************************/
void resetProcs(CMD Commands[]) {

    Commands[PROC_1].PIPE_IN = false;
    Commands[PROC_1].PIPE_OUT = false;
    Commands[PROC_1].RD_IN = false;
    Commands[PROC_1].RD_OUT = false;
    Commands[PROC_1].argsCount = 0;

    Commands[PROC_2].PIPE_IN = false;
    Commands[PROC_2].PIPE_OUT = false;
    Commands[PROC_2].RD_IN = false;
    Commands[PROC_2].RD_OUT = false;
    Commands[PROC_2].argsCount = 0;

}

/*******************************************************************************
 *int parseErrors()                                                            *
 *                                                                             *
 * Function checks the state of error flags thrown up by                       *
 * parse(char**, struct Command)                                               *
 *                                                                             *
 * PARAMETERS: No Parameters                                                   *
 *                                                                             *
 * RETURNS: Returns 1 if any flags have been thrown, 0 otherwise               *
 ******************************************************************************/
int parseErrors() {
    return ((BLANK_INPUT
            || INCOMPLETE_COMMAND
            || AMBIGUITY
            || INCOMPLETE_OUT_REDIRECT
            || INCOMPLETE_IN_REDIRECT
            || UM_QUOTE
            || MULT_PIPE
            || NULL_COMMAND
            || INVALID_ENV_VAR));

}

/*******************************************************************************
 *int isSpecialString(char*)                                                   *
 *                                                                             *
 * Helper function to parse(char**, struct Command) which checks if given      *
 * argument is either a "<" or a ">" string                                    *
 *                                                                             *
 * PARAMETERS:                                                                 *
 *   *string -- given string is compared against  "<"and ">"                   *
 *                                                                             *
 * RETURNS: Returns 1 argument string matches either of the metaCharacter      *
 * constants                                                                   *
 ******************************************************************************/
int isSpecialString(char* string) {
    return(!(strcmp(string, metaChars[0])) || !(strcmp(string, metaChars[1])));
}

/*******************************************************************************
 *void setupPipe(int STDIO_FILENO)                                             *
 *                                                                             *
 * Helper function to setup I/O pipes                                          *
 *                                                                             *
 * PARAMETERS:                                                                 *
 *   int  -- file descriptor                                                   *
 * RETURNS: No return value                                                    *
 ******************************************************************************/

void setupPipe(int STDIO_FILENO) {
    if (STDIO_FILENO == STDOUT_FILENO) {
        CHK(dup2(filedes[1], STDOUT_FILENO));
        CHK(close(filedes[0]));
        CHK(close(filedes[1]));
    } else if (STDIO_FILENO == STDIN_FILENO) {
        CHK(dup2(filedes[0], STDIN_FILENO));
        CHK(close(filedes[0]));
        CHK(close(filedes[1]));
    }
}

/*******************************************************************************
 *void eraseLine(FILE*)                                                        *
 *                                                                             *
 * Helper function eats up remaining line of input up to newline or EOF in     *
 * cases of errors or when background process is created                       *
 *                                                                             *
 * PARAMETERS:                                                                 *
 *   *FILE - input file stream to read from                                    *
 * RETURNS: No return value                                                    *
 ******************************************************************************/

void eraseLine(FILE * fp) {
    int tmp;
    while ((tmp = fgetc(fp)) != '\n' && tmp != EOF) {
    }
}
