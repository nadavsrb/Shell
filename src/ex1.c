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
#define HISTORY_COMMAND "cd"
#define JOBS_COMMAND "cd"
#define EXIT_COMMAND "exit"

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

typedef struct 
{
    Command* com;
    pid_t processID;
}BGRunningCommand;

typedef struct 
{
    Command* coms[COMMAND_MAX_NUM];
    int size;
}History;

typedef struct 
{
    BGRunningCommand bgRunningComs[COMMAND_MAX_NUM];
    int size;
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

int string_starts_with(const char *str, const char *pre)
{
    int str_len = strlen(str);
    int pre_len = strlen(pre);

  return (str_len >= pre_len) &&
    (0 == strncmp(str, pre, pre_len));
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

int isExitCommand(Command* com) {
    return (strcmp(EXIT_COMMAND, com->comStr) == 0);
}

void killAllBGProcess(BGRunning* bgRunning) {
    for(int i = 0; i < bgRunning->size; ++i){
        kill(bgRunning->bgRunningComs[i].processID, SIGKILL);
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

    waitpid(sonPID, NULL, 0);
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
void doCommand(Command* com, History* history, BGRunning* bgRunning) {
    char* args[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed
    char strArgs[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed
    strcpy(strArgs, com->comStr);
    getArgs(args, strArgs);
    // //get the args:
    // int i = 0;
    // printf(".............|%s|\n", com->comStr);////////
    // char comStr[COMMAND_MAX_CHARS + 1] , *word;
    // strcpy(comStr, com->comStr);
    // char* endComStr = comStr + strlen(comStr);
    // word = strtok(comStr, SPACE);
    // char* args[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed

    // while (word != NULL) {
    //     if(string_starts_with(word, QUOTE)){
    //         ++word;
    //         char* endWord = word + strlen(word);
    //         if(endComStr != endWord) {
    //             *(endWord) = SPACE_CHAR;
    //         }
    //         printf("%s.....\n", word);/////
    //         char* phrase = strtok(word, QUOTE);
    //         if(phrase == NULL) {// everything ends with """"
    //             break;
    //         }
    //         printf("%s,%s.....\n", word, word + 2);/////
    //         char* endPhrase = phrase + strlen(phrase) - 1;
    //         while(*endPhrase == BACKSLASH_CHAR && (endPhrase + 1) < endComStr){
    //             if(strtok(NULL, QUOTE) == NULL){ //so in the end we have only "
    //                 *(endPhrase + 2) = '\0';
    //             }
    //             printf("%s,%s.....\n", word, word + 2);/////
    //             *(++endPhrase) = QUOTE_CHAR;
    //             endPhrase = phrase + strlen(phrase) - 1;
    //             printf("%s.....\n", word);/////
    //         }
    //         args[i] = word;
    //         char* contChars = endPhrase + 2;// endPhrase + 2 its the chars after the phrase
    //         if(contChars < endComStr){
    //             word = strtok(contChars, SPACE);
    //         } else {
    //             word = NULL;
    //         }
    //         ++i;

    //         continue;
    //     }
        
        // if(string_ends_with(word, BACKSLASH)){
        //     char* endWord = word + strlen(word) - 1;

        //     while(*endWord == BACKSLASH_CHAR && (endWord + 1) < endComStr) {
        //         strtok(NULL, SPACE);
        //         *(++endWord) = SPACE_CHAR;
        //         endWord = word + strlen(word) - 1;
        //     }
        // }

    //     args[i] = word;
    //     word = strtok(NULL, SPACE);
    //     ++i;
    // }
    // args[i] = NULL;


    //     //get the args:
    // int i = 0;
    // char *quoteStrtok, *spaceStrtok;
    // char* token = strtok_r(com->comStr, QUOTE, &quoteStrtok);
    // char* args[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed

    // Bool isInQuotes = False;
    // while (token != NULL) {
    //     if(isInQuotes){
    //         args[i] = token;
    //         ++i;
    //     }else {
    //         char* word = strtok_r(token, SPACE, &spaceStrtok);
    //         while (word != NULL){
    //             args[i] = word;
    //             word = strtok_r(NULL, SPACE, &spaceStrtok);
    //             ++i;
    //         }
            
    //     }
    //     token = strtok_r(NULL, QUOTE, &quoteStrtok);
    //     isInQuotes = !isInQuotes;
    // }
    // args[i] = NULL;

    for(int j = 0; args[j] != NULL; ++j) {
        printf("|%s|\n", args[j]);
    }

    //do the command
    if(strcmp(args[0], CD_COMMAND) == 0){
        //doCdCommand();
        com->isRunning = False;
    }else if (strcmp(args[0], HISTORY_COMMAND) == 0){
        //doHistoryCommand();
        com->isRunning = False;
    }else if (strcmp(args[0], JOBS_COMMAND) == 0){
        //doJobsCommand();
        com->isRunning = False;
    }else if(com->isBGCommand){
        //doBGCommand();
    }else{
        doFGCommand(args);
        com->isRunning = False;
    }

}

int main(int argc, char const *argv[]) {
    History history;
    history.size = EMPTY;

    BGRunning bgRunning;
    bgRunning.size = EMPTY;

    do{
        printShell();

        Command com;
        getCommand(&history, &com);
        
        if(isExitCommand(&com)) {
            killAllBGProcess(&bgRunning);
            break;
        }

        doCommand(&com, &history, &bgRunning);
    } while(True);

    return 0;
}