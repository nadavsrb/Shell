// Nadav Sharabi 213056153

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <pwd.h>

#define SHELL_SIGN "$ "
#define COMMAND_MAX_CHARS 100
#define COMMAND_MAX_NUM 100
#define SPACE_CHAR ' '
#define TILDE "~"
#define SLASH_STR "/"
#define QUOTE_CHAR '\"'
#define BACKSLASH_CHAR '\\'
#define END_STR_CHAR '\0'
#define HOME_STR "~/"
#define HYPHEN "-"
#define BG_SIGN " &"
#define EMPTY 0
#define MAX_CD_ARGS 2
#define HOME_ENV_STR "HOME"

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

/**
 * @brief This enum is a to represent boolean variables.
 */
typedef enum{
    False,
    True
}Bool;

/**
 * @brief This enum is a to represent status (types of errors and success).
 */
typedef enum{
    SUCCESS,
    EXEC_ERROR,
    FORK_ERROR,
    CHDIR_ERROR,
    CD_ARGS_ERROR,
    UNKNOWN_ERROR
}Status;

/**
 * @brief This struct is to represent a shell command.
 */
typedef struct
{
    Bool isBGCommand;
    Bool isRunning;
    char comStr[COMMAND_MAX_CHARS + 1]; //+ 1 for NULL in the end
}Command;

/**
 * @brief This struct is to represent a shell command that is:
 * 1) a Background command
 * 2) running
 */
struct BGRunningCommand
{
    Command* com;
    pid_t processID;
    struct BGRunningCommand* next;
};
typedef struct BGRunningCommand BGRunningCommand;

/**
 * @brief This struct is used as data structure to instore all the Commaneds.
 */
typedef struct 
{
    Command* coms[COMMAND_MAX_NUM];
    int size;
} History;

/**
 * @brief This struct is used as data structure to instore all the BGRunningCommands. 
 */
typedef struct 
{
    BGRunningCommand bgRunningComs[COMMAND_MAX_NUM];
    BGRunningCommand* end;
    BGRunningCommand* start;
    int nextIndex; //the location to put in the bgRunningComs the next BGRunningCommand.
}BGRunning;

/**
 * @brief Prints the shell symbol to the screen.
 */
void printShell() {
    printf(SHELL_SIGN);
    fflush(stdout);
}

/**
 * @brief Checks if str string ends with the suffix string.
 * (this code is from stuckoverflow :) ).
 * 
 * @param str the string to check if ends with the suffix.
 * @param suffix the suffix string.
 * @return True if the str ends with the suffix, otherwise return False.
 */
int string_ends_with(const char * str, const char * suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
    (0 == strcmp(str + (str_len-suffix_len), suffix));
}

/**
 * @brief Checks if str string starts with the pre string.
 * (this code is from stuckoverflow :) ).
 * 
 * @param str the string to check if starts with the pre.
 * @param pre the pre string.
 * @return True if the str starts with the pre, otherwise return False.
 */
int string_starts_with( const char *str, const char *pre)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}

/**
 * @brief read the next commaned from the shell.
 * 
 * @param history the data structure that instores all the commands.
 * @param com the struct to store the command in.
 */
void getCommand(History* history, Command* com) {
    //adds the commaned to the history database.
    history->coms[history->size] = com;
    ++history->size;
    
    //initialize the commands flags.
    com->isBGCommand = False;
    com->isRunning = True;

    //reads from the shell the command (reads until end of line).
    fgets(com->comStr, COMMAND_MAX_CHARS + 1 , stdin);
    com->comStr[strlen(com->comStr) - 1] = END_STR_CHAR; //removing '\n' char from the end.

    //checks if this command should run in the background.
    if(string_ends_with(com->comStr, BG_SIGN)){
        com->isBGCommand = True;

        //remove the background symbole from the command str.
        com->comStr[strlen(com->comStr) - strlen(BG_SIGN)] = END_STR_CHAR;
    }
}

/**
 * @brief BGRunning database is lazy, only when we use it
 * we will coll this func to update it (remove from this database
 * all the commandes that already finished).
 * 
 * @param bgRunning the database to update.
 */
void updateBgRunning(BGRunning* bgRunning){
    //iterating the database.
    BGRunningCommand* beforeBGCom = NULL;
    BGRunningCommand* BGCom = bgRunning->start;
    while (BGCom != NULL){

        //checks if the proccess that is running this commaned is dead.
        pid_t pidSon = waitpid(BGCom->processID, NULL, WNOHANG);
        if(pidSon != 0) {//the process is dead
            //updating command flag
            BGCom->com->isRunning = False;

            //removing the command from the database.
            if(beforeBGCom == NULL){ //this command was the first commaned in the database.
                bgRunning->start = BGCom->next;
            } else { //this command has commaned before.
                beforeBGCom->next = BGCom->next;
            }

            
            if(BGCom->next == NULL) {//it was was the last command in the database.
                //updates the end of the database.
                bgRunning->end = beforeBGCom;
            }
        }

        //updates iterate variables.
        beforeBGCom = BGCom;
        BGCom = BGCom->next;
    }
    
}

/**
 * @brief This func kills all the child proccess,
 * of our main process, notice that only background proccess
 * can be running if we got to here.
 * 
 * @param bgRunning the database that instores all the 
 * commands that running in the background.
 */
void killAllBGProcess(BGRunning* bgRunning) {
    //updates the database.
    updateBgRunning(bgRunning);

    //iterats the commands that running in the background.
    BGRunningCommand *bgCom = bgRunning->start;
    while(bgCom != NULL){
        //kill the running commaned
        kill(bgCom->processID, SIGKILL);

        bgCom = bgCom->next;
    }
}

/**
 * @brief this func gets Status end handles it.
 * 
 * @param status the status to handle.
 */
void handleStatus(Status status) {

    //gets the message to print if needed.
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

    //prints the message.
    printf("%s\n", massage); 
}

/**
 * @brief Gets the args from command str.
 * 
 * @param args the array to put in the args (stores only pointres to the string);
 * @param strArgs the command str, and also, in the end, stores in it the full args' strings.
 */
void getArgs(char* args[], char strArgs[]) {
    //an helpful copy of the command str.
    char strArgsCopy[COMMAND_MAX_CHARS + 1];
    strcpy(strArgsCopy, strArgs);

    //iterats the command str.
    int indexNextArg = 0; //the location to store the arg in the args array.
    char* pNextArg = strArgs; //pointer to the start location of the next arg.
    char* iterSrc = strArgsCopy;
    char* iterDst = strArgs;
    Bool isInSpecialChar = False;
    Bool isInQuotes = False;
    Bool isDelayedSpaces = True;
    while(*iterSrc != END_STR_CHAR) { //go over the command str.
        //if we got to a space char and we are not inside of quotes so we finished
        //to read an arg.
        if (*iterSrc == SPACE_CHAR && !isInQuotes) {
            if(isDelayedSpaces) { //spaces in row between args.
                //skipping trailling spaces between args.
                ++iterSrc;
                continue;
            }

            *iterDst = END_STR_CHAR; //end of arg.
            args[indexNextArg] = pNextArg; //saves the arg.

            //intializing the iter variabels
            ++iterSrc;
            ++iterDst;
            ++indexNextArg;
            pNextArg = iterDst;

            //intializing flags.
            isInSpecialChar = False;
            isDelayedSpaces = True;
            continue;
        }
        isDelayedSpaces = False; //if we got here than we didn't have space and in new arg.

        //if we got quote and we they are not part of special char: \"
        if(*iterSrc == QUOTE_CHAR && !isInSpecialChar) {
            //we now stats new quotes or in the ends of ones.
            isInQuotes = !isInQuotes;
            ++iterSrc;//we skipping the quotes and don't put the in the arg.
            continue;
        }

        //if we on '\' then it could be a special char after it so we should remmeber,
        //notice that could be '\\' so the next '\' is not indicates special char.
        //also if we are in special char so now we exit from it.
        if(*iterSrc == BACKSLASH_CHAR || isInSpecialChar) {
            isInSpecialChar = !isInSpecialChar;
        }

        //copies the char to the arg, and moving to the next one.
        *iterDst = *iterSrc;
        ++iterDst;
        ++iterSrc;
    }
    
    //after the last arg not must be space so we saving it,
    //if there was we only end the args with NULL witch indicates end of args.
    *iterDst = END_STR_CHAR; //end of arg
    args[indexNextArg] = pNextArg;
    ++indexNextArg;

    args[indexNextArg] = NULL; //we make sure we  put NULL in the end of args.
}

/**
 * @brief Excutes command in forground.
 * 
 * @param args the command args.
 */
void doFGCommand(char* args[]){
    pid_t sonPID;

    if((sonPID = fork()) == -1) { //fork failed
        handleStatus(FORK_ERROR);
        return;
    } else if (sonPID == 0){ //we are the son process
        execvp(args[0], args); //do the command.

        //if we got here exec failed:
        handleStatus(EXEC_ERROR);
        exit(0);
    }

    // we are the father
    waitpid(sonPID, NULL, 0);
}

/**
 * @brief Excutes command in background.
 * notice if fork failed no proccess runs the command,
 * so we don't add the command to the database and we
 * mark the command as not running. 
 * 
 * @param args the command args.
 * @param bgRunning the database of the background running commands.
 * @param com the command to run.
 */
void doBGCommand(char* args[], BGRunning* bgRunning, Command* com){
    pid_t sonPID;

    if((sonPID = fork()) == -1) { //fork failed
        handleStatus(FORK_ERROR);
        com->isRunning = False;
        return;
    
    } else if (sonPID == 0){ // we are the son process
        execvp(args[0], args);//do the command.

        //if we got here exec failed:
        handleStatus(EXEC_ERROR);
        exit(0);
    }

    //adding the command to the database:

    //intializing the command as bg command.
    BGRunningCommand* thisCom = &bgRunning->bgRunningComs[bgRunning->nextIndex];
    thisCom->com = com;
    thisCom->processID = sonPID;
    thisCom->next = NULL;
    ++bgRunning->nextIndex;

    //if no commands in the database:
    if(bgRunning->start == NULL){
        bgRunning->start = thisCom;
    }

    //updates the end of the database
    if(bgRunning->end != NULL) {
        bgRunning->end->next = thisCom;
    }
    bgRunning->end = thisCom;
}

/**
 * @brief printing all the bg command that running.
 * 
 * @param bgRunning the database of the bg running proccesses.
 */
void doJobsCommand(BGRunning* bgRunning){
    updateBgRunning(bgRunning);// updating the database.

    //iterating the database.
    BGRunningCommand *bgCom = bgRunning->start;
    while(bgCom != NULL){
        printf("%s\n", bgCom->com->comStr); //printing the bg running command.
        bgCom = bgCom->next;
    }
}

/**
 * @brief prints all the commands and their status.
 * notice! this func assumes all the data is updated,
 * so before this func you should use the updateBgRunning
 * to check if there are bg command that was running but now ended.
 * 
 * @param history the commands database.
 */
void doHistoryCommand(History* history){
    //iterates the database.
    int i;
    for(i = 0; i < history->size; ++i) {
        //gets the str that represents the status of the command.
        char* status = RUNNING_STR;
        if (!history->coms[i]->isRunning) {
            status = DONE_STR;
        }

        //prints the commands and their status.
        printf("%s %s\n", history->coms[i]->comStr, status); 
    }
}

/**
 * @brief Gets the length of args array.
 * 
 * @param args the args array.
 * @return the length of args array.
 */
int getArgsLength(char* args[]) {
    //calculating the length of the args array.
    int length = 0;
    while(args[length] != NULL){
        ++length;
    }

    return length;
}

/**
 * @brief Excutting the cd command.
 * 
 * @param args the command args.
 */
void doCdCommand(char* args[]){
    //this static var is to save the last path we excute cd command successful from.
    static char lastPath[COMMAND_MAX_CHARS + 1] = ""; // +1 for NULL in the end.

    //checks if there are too much args.
    int length = getArgsLength(args);
    if(length > MAX_CD_ARGS){
        handleStatus(CD_ARGS_ERROR);
        return;
    }

    //get the path for the cd command if no arg path then
    //we put the home path: "~".
    char* pathArg = args[1];
    if(length != MAX_CD_ARGS) {
        pathArg = TILDE;
    }
    
    //we get the home directory path of our proccess.
    char *homeDirPath;
    if ((homeDirPath = getenv(HOME_ENV_STR)) == NULL) {
        homeDirPath = getpwuid(getuid())->pw_dir;
    }
    
    //this var would contain our path after proccessing it.
    char path[COMMAND_MAX_CHARS + 1] = "";

    /*
    according to the shell:
    1) the - can only be used by: cd -
    2) the ~ only should change tho the home directory
    if the command is: cd ~ or cd ~/[the continue of the path]
    
    *** I noticed chdir handles .. by itself.
    */ 
    if(strcmp(pathArg, HYPHEN) == 0){ // cd -
        strcpy(path, lastPath);
    }else if(strcmp(pathArg, TILDE) == 0) {// cd ~
        strcpy(path, homeDirPath);
    }else if(string_starts_with(pathArg, HOME_STR)){ //cd ~/[...]
        strcpy(path, homeDirPath);
        strcat(path, pathArg + 1); //adds the continue of the path.
    }else{
        strcpy(path, pathArg); //else chdir should know how to handle the path.
    }

    //gets the path we are on.
    char currentPath[COMMAND_MAX_CHARS + 1]; // +1 for NULL in the end.
    if(getcwd(currentPath, COMMAND_MAX_CHARS + 1) == NULL) {
        handleStatus(UNKNOWN_ERROR);
        return;
    }

    //tries to excute chdir on the path.
    if(chdir(path) == -1){
        handleStatus(CHDIR_ERROR);
        return;
    }

    //chdir succeeded, saves as last path we were for cd -.
    strcpy(lastPath, currentPath);
}

/**
 * @brief execute shell command.
 * 
 * @param com the command to execute.
 * @param history the commands database.
 * @param bgRunning the bg running command database.
 * @param isExitCommand this var is set to True if the command is exit.
 */
void doCommand(Command* com, History* history, BGRunning* bgRunning, Bool* isExitCommand) {
    //get the args
    char* args[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed.
    char strArgs[COMMAND_MAX_CHARS + 1]; //+1 for the NULL in the end if needed.
    //we copy the str command so the get args won't change the val of  com->comStr.
    strcpy(strArgs, com->comStr); 
    getArgs(args, strArgs); //get the args.

    //do the command
    if(strcmp(args[0], EXIT_COMMAND) == 0){// exit command.
        killAllBGProcess(bgRunning);
        *isExitCommand = True;
    }else if(strcmp(args[0], CD_COMMAND) == 0){// cd command.
        doCdCommand(args);
        com->isRunning = False;
    }else if (strcmp(args[0], HISTORY_COMMAND) == 0){// history command.
        updateBgRunning(bgRunning);
        doHistoryCommand(history);
        com->isRunning = False;
    }else if (strcmp(args[0], JOBS_COMMAND) == 0){// jobs command.
        doJobsCommand(bgRunning);
        com->isRunning = False;
    }else if(com->isBGCommand){// bg command.
        doBGCommand(args, bgRunning, com);
    }else{// else must be fg command.
        doFGCommand(args);
        com->isRunning = False;
    }
}

/**
 * @brief this is a shell program.
 */
int main() {
    //initialize the command database.
    History history;
    history.size = EMPTY;

    //initialize the bg running command database.
    BGRunning bgRunning;
    bgRunning.nextIndex = EMPTY;
    bgRunning.end = NULL;
    bgRunning.start = NULL;

    //initialize the command structures we might need.
    Command com[COMMAND_MAX_NUM];
    int indexCom = 0;

    Bool isExitCommand = False;
    do{
        //prints the shell.
        printShell();

        //gets the next command.
        getCommand(&history, &com[indexCom]);

        //execute the next command.
        doCommand(&com[indexCom], &history, &bgRunning, &isExitCommand);

        //moving to the next command.
        ++indexCom;

    } while(!isExitCommand);//check if should exit shell.

    return 0; //exit shell.
}