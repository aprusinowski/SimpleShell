/* 
 * File:   p2.h
 * Author: Artur Prusinowski
  *     Class: CS570
 *      Instructor: Dr. Carroll
 *      Assignment: Program 4, Due 04/23/2016
 *
 *      Header file for p2.c
 */

#ifndef P2_H
#define P2_H

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "CHK.h"
#include "getword.h"

#define MAXITEM 100 
#define MAXPIPECOMMANDS 2
#define PROC_1 0
#define PROC_2 1
#define neg -1
#define true 1
#define none -1
#define false 0
#define NOT_DIR 150
#define PARSE_ERR 151
#define ARG_COUNT_ERR 152
#define FILE_CREAT_FL  O_WRONLY|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR
#define END_OF_INPUT(C,B,D)(((C==0||C==-1||C==-2)&&B>0)||(D==1)) 
#define TERMINATE(C,B)(C==-1&&B==0)
#define BLANK_LINE(C,B)(((C==0||C==-1||C==-2)&&B==0)||(B==0&&AMPERSAND)||(B==0&&PIPE))
#define OUT_PIPE(C,B)(B&&(C==PROC_1))
#define IN_PIPE(C,B)(B&&(C==PROC_2))
#define INCOMPLETE_PIPE(C,B)(C==B)
#define IS_PARSABLE(C) (C>0)
#define IS_AMPERSAND(C)(C=='&')
#define IS_PIPE(C)(C=='|')


static int filedes[2];                      //file descriptors for pipe() function
static int AMPERSAND = false;               //Background process flag
static int EXIT_PROGRAM = false;            //EOF encountered.Exit P2
static int AMBIGUITY = false;               //Ambiguous I/O redirect encountered 
static int INCOMPLETE_OUT_REDIRECT = false; //No filename provided to redirect 
static int INCOMPLETE_IN_REDIRECT = false;  //No filename provided to redirect 
static int INCOMPLETE_COMMAND = false;      //No command specified
static int BLANK_INPUT = false;             //Nothing entered on line
static int PIPE = false;                    //I/O pipe flag
static int PIPE_POS = false;                //position of I/O pipe in input line
static int MULT_PIPE = false;               //Multiple pipes error flag
static int CLOSE_PIPE = false;              //indicates to parent process when to close pipe
static int NULL_COMMAND = false;            //null command error flag
static int INVALID_ENV_VAR = false;         //invalid env variable error flag
int ENV_VAR_VALUE = false;                  //getword() detects env variable flag
int UM_QUOTE = false;                       //Unmatched quote  flag

static const char* metaChars[] = { ">", "<"}; 


/*******************************************************************************
 * struct Command: Contains all necessary elements of an executable command.   *
 *                 All char* variables are reassigned in parse() function to   *
 *                 locations in the char* argv[] array declared in main().     *
 *                                                                             *
 * char* filePathOut  --  Pointer to STDOUT redirect filepath, if one provided *
 * char* filePathIn   --  Pointer to STDIN redirect filepath, if one provided  *
 * char* args[]       --  Pointers to the executable command along with its    *
 *                        arguments. args[] is closed with a NULL pointer in   *
 *                        so satisfy the requirements of execvp().             *
 * int   argsCount    --  Number of command arguments passed plus 1 to account * 
 *                        for command name.                                    *
 * int (*execute)(struct Command *) -- Pointer to the appropriate command      *
 *                                     execution function.                     *
 ******************************************************************************/
struct Command;
typedef struct Command CMD;
struct Command {    
    int PIPE_IN;
    int PIPE_OUT;
    int RD_IN;
    int RD_OUT;
    char* filePathOut;
    char* filePathIn;
    char* args[MAXITEM + 1];
    int argsCount;
    int pipeIndex;
    int (*execute)(CMD *);
}; 


int  parse(char **, CMD*); 
int  execLsFCommnd(CMD*);
int  execDirCommnd(CMD*);
int  execPrintEnvCommnd(CMD *);
int  execSetEnvCommnd(CMD*);
int  execBackground(CMD*);
int  execForeground(CMD*);
int  rd_stdout(char*);
int  rd_stdin(char*);
int  printExecError(int , char *);
int  printParseError(char *name);
void resetFlags();
int  parseErrors();
int  isSpecialString(char*);
void signalHandler(int signum){}
void setupPipe(int);
int  getEnvVar(char*);
void resetProcs(CMD[] );
void eraseLine(FILE *);

#endif
