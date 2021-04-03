// Nadav Sharabi 213056153
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#define SHELL_SIGN "$ "
#define COMMAND_MAX_CHARS 100
#define COMMAND_MAX_NUM 100
#define SPACE_CHAR ' '
#define QUOTE_CHAR '\"'
#define BACKSLASH_CHAR '\\'
#define END_STR_CHAR '\0'
#define BG_SIGN " &"
#define EMPTY 0

#define CD_COMMAND "cd"
#define HISTORY_COMMAND "history"
#define JOBS_COMMAND "jobs"
#define EXIT_COMMAND "exit"

#define RUNNING_STR "RUNNING"
#define DONE_STR "DONE"

#define EXEC_ERROR_MASSAGE "exec failed"
#define FORK_ERROR_MASSAGE "fork failed"
#define CHDIR_ERROR_MASSAGE "chdir failed"
#define CD_ARGS_ERROR_MASSAGE "Too many argument"
#define UNKNOWN_ERROR_MASSAGE "An error occurred"
typedef enum{
    False,
    True
}Bool;

typedef enum{
    SUCCESS,
    EXEC_ERROR,
    FORK_ERROR,
    CHDIR_ERROR,
    CD_ARGS_ERROR,
    UNKNOWN_ERROR
}Status;

typedef struct
{
    Bool isBGCommand;
    Bool isRunning;
    char comStr[COMMAND_MAX_CHARS + 1]; //+ 1 for NULL in the end
}Command;
struct BGRunningCommand
{
    Command* com;
    pid_t processID;
    struct BGRunningCommand* next;
};

typedef struct BGRunningCommand BGRunningCommand;

typedef struct 
{
    Command* coms[COMMAND_MAX_NUM];
    int size;
}History;

typedef struct 
{
    BGRunningCommand bgRunningComs[COMMAND_MAX_NUM];
    BGRunningCommand* end;
    BGRunningCommand* start;
    int nextIndex;
}BGRunning;

void printShell() {
    printf(SHELL_SIGN);
    fflush(stdout);
}

int string_ends_with(const char * str, const char * suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
    (0 == strcmp(str + (str_len-suffix_len), suffix));
}

void getCommand(History* history, Command* com) {
    history->coms[history->size] = com;
    ++history->size;
    
    com->isBGCommand = False;
    com->isRunning = True;

    scanf(" %[^'\n']s", com->comStr);

    if(string_ends_with(com->comStr, BG_SIGN)){
        com->isBGCommand = True;
        com->comStr[strlen(com->comStr) - strlen(BG_SIGN)] = '\0';
    }
}

void killAllBGProcess(BGRunning* bgRunning) {
    BGRunningCommand *bgCom = bgRunning->start;
    while(bgCom != NULL){
        kill(bgCom->processID, SIGKILL);
        bgCom = bgCom->next;
    }
}

void handleStatus(Status status) {
    char* massage;
    switch(status){
        case EXEC_ERROR:
            massage = EXEC_ERROR_MASSAGE;
            break;
        case FORK_ERROR:
            massage = FORK_ERROR_MASSAGE;
            break;
        case CHDIR_ERROR:
            massage = CHDIR_ERROR_MASSAGE;
            break;
        case CD_ARGS_ERROR:
            massage = CD_ARGS_ERROR_MASSAGE;
            break;
        case UNKNOWN_ERROR:
            massage = UNKNOWN_ERROR_MASSAGE;
            break;
        default: // SUCCESS
            return;
    }

    printf("%s\n", massage); 
}

void getArgs(char* args[], char strArgs[]) {
    char strArgsCopy[COMMAND_MAX_CHARS + 1];
    strcpy(strArgsCopy, strArgs);

    int indexNextArg = 0;
    char* pNextArg = strArgs;
    char* iterSrc = strArgsCopy;
    char* iterDst = strArgs;
    Bool isInSpecialChar = False;
    Bool isInQuotes = False;
    Bool isDelayedSpaces = True;
    while(*iterSrc != END_STR_CHAR) { //go over the str
        if (*iterSrc == SPACE_CHAR && !isInQuotes) {
            if(isDelayedSpaces) { //spaces in row between args
                ++iterSrc;
                continue;
            }
            *iterDst = END_STR_CHAR; //end of arg
            args[indexNextArg] = pNextArg;
            ++iterSrc;
            ++iterDst;
            ++indexNextArg;
            pNextArg = iterDst;
            isInSpecialChar = False;
            isDelayedSpaces = True;
            continue;
        }
        isDelayedSpaces = False;

        if(*iterSrc == QUOTE_CHAR && !isInSpecialChar) {
            isInQuotes = !isInQuotes;
            ++iterSrc;
            continue;
        }

        if(*iterSrc == BACKSLASH_CHAR || isInSpecialChar) {
            isInSpecialChar = !isInSpecialChar;
        }

        *iterDst = *iterSrc;
        ++iterDst;
        ++iterSrc;
    }

    *iterDst = END_STR_CHAR; //end of arg
    args[indexNextArg] = pNextArg;
    ++indexNextArg;
    args[indexNextArg] = NULL; //end of args
}

void doFGCommand(char* args[]){
    pid_t sonPID;

    if((sonPID = fork()) == -1) { //fork failed
        handleStatus(FORK_ERROR);
        return;
    } else if (sonPID == 0){ // we are the son process
        execvp(args[0], args);
        handleStatus(EXEC_ERROR);

        exit(0);
    }
    // we are the father
    waitpid(sonPID, NULL, 0);
}

void doBGCommand(char* args[], BGRunning* bgRunning, Command* com){
    pid_t sonPID;

    if((sonPID = fork()) == -1) { //fork failed
        handleStatus(FORK_ERROR);
        com->isRunning = False;
        return;
    
    } else if (sonPID == 0){ // we are the son process
        execvp(args[0], args);
        handleStatus(EXEC_ERROR);

        exit(0);
    }

    BGRunningCommand* thisCom = &bgRunning->bgRunningComs[bgRunning->nextIndex];
    thisCom->com = com;
    thisCom->processID = sonPID;
    thisCom->next = NULL;
    ++bgRunning->nextIndex;

    if(bgRunning->start == NULL){
        bgRunning->start = thisCom;
    }

    if(bgRunning->end != NULL) {
        bgRunning->end->next = thisCom;
    }
    bgRunning->end = thisCom;
}

void updateBgRunning(BGRunning* bgRunning){
    BGRunningCommand* beforeBGCom = NULL;
    BGRunningCommand* BGCom = bgRunning->start;

    while (BGCom != NULL){
        pid_t pidSon = waitpid(BGCom->processID, NULL, WNOHANG);

        if(pidSon != 0) {//the process is dead
            BGCom->com->isRunning = False;
            if(beforeBGCom == NULL){
                bgRunning->start = BGCom->next;
            } else {
                beforeBGCom->next = BGCom->next;
            }

            if(BGCom->next == NULL) {
                bgRunning->end = beforeBGCom;
            }
        }

        beforeBGCom = BGCom;
        BGCom = BGCom->next;
    }
    
}

void doJobsCommand(BGRunning* bgRunning){
    BGRunningCommand *bgCom = bgRunning->start;
    while(bgCom != NULL){
        printf("%s\n", bgCom->com->comStr);
        bgCom = bgCom->next;
    }
}

void doHistoryCommand(History* history){
    for(int i = 0; i < history->size; ++i) {
        printf("%s ", history->coms[i]->comStr);

        if (history->coms[i]->isRunning) {
            printf(RUNNING_STR);
        } else {
            printf(DONE_STR);
        }

        printf("\n"); 
    }
}
void doCommand(Command* com, History* history, BGRunning* bgRunning, Bool* isExitCommand) {
    //get the args
    char* args[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed
    char strArgs[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed
    strcpy(strArgs, com->comStr);
    getArgs(args, strArgs);

///////
    for(int j = 0; args[j] != NULL; ++j) {
        printf("|%s|\n", args[j]);
    }
//////

    //do the command
    if(strcmp(args[0], EXIT_COMMAND) == 0){
        killAllBGProcess(bgRunning);
        *isExitCommand = True;

    }else if(strcmp(args[0], CD_COMMAND) == 0){
        //doCdCommand();
        com->isRunning = False;
    }else if (strcmp(args[0], HISTORY_COMMAND) == 0){
        updateBgRunning(bgRunning);
        doHistoryCommand(history);
        com->isRunning = False;
    }else if (strcmp(args[0], JOBS_COMMAND) == 0){
        updateBgRunning(bgRunning);
        doJobsCommand(bgRunning);
        com->isRunning = False;
    }else if(com->isBGCommand){
        doBGCommand(args, bgRunning, com);
    }else{
        doFGCommand(args);
        com->isRunning = False;
    }

}

int main(int argc, char const *argv[]) {
    History history;
    history.size = EMPTY;

    BGRunning bgRunning;
    bgRunning.nextIndex = EMPTY;
    bgRunning.end = NULL;
    bgRunning.start = NULL;

    Bool isExitCommand = False;
    Command com[COMMAND_MAX_NUM];
    int indexCom = 0;
    do{
        printShell();

        getCommand(&history, &com[indexCom]);

        doCommand(&com[indexCom], &history, &bgRunning, &isExitCommand);

        ++indexCom;
    } while(!isExitCommand);

    return 0;
}