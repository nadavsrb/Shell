// Nadav Sharabi 213056153
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define SHELL_SIGN "$ "
#define COMMAND_MAX_CHARS 100
#define COMMAND_MAX_NUM 100
#define SPACE " "
#define BG_SIGN " &"
#define EMPTY 0

#define CD_COMMAND "cd"
#define HISTORY_COMMAND "cd"
#define JOBS_COMMAND "cd"
#define EXIT_COMMAND "exit"
typedef enum{
    False,
    True
}Bool;

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

// void doCommand(Command* com, History* history, BGRunning* bgRunning) {
//     //get the args:
//     int i = 0;
//     char* word = strtok(com->comStr, SPACE);
//     char* args[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed

//     while (word != NULL) {
//         args[i] = word;
//         word = strtok(NULL, SPACE);
//         ++i;
//     }
//     args[i] = NULL;

//     //do the command
//     if(strcmp(args[0], CD_COMMAND) == 0){
//         doCdCommand();
//     }else if (strcmp(args[0], HISTORY_COMMAND) == 0){
//         doHistoryCommand();
//     }else if (strcmp(args[0], JOBS_COMMAND) == 0){
//         doJobsCommand();
//     }else if(!com->isBGCommand){
//         doBGCommand();
//     }else{
//         doFGCommand();
//     }
// }

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

        //doCommand(&com, &history, &bgRunning);
    } while(True);

    return 0;
}