//Assignment 3: Smallsh
////Peri Cabrales

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


//global variables
int ex = 1;
//must support up to max length of 2048 characters
char answer[2049];
//must support max of 512 arguments
char input[513][50];
//find length of input
int length = 0;
//char* input[513];
//fill with given commands just in case it isnt a built in one
char* args[513];
char cdName[50];
char pids[2000];

//pid variables and such
pid_t spawnpid = -5;
pid_t currpid;
int childExitStatus = -5;
int childExitMethod;
int pidD;
int backgroundpid = 0;;
int exitType = 0;

//holds number of processes that have been going
int processes = 0;

//for the ctrl z foreground only mode
int fg = 0;

int i;
int j = 0;
int k;
//chec to see if it has an & at the end
int hasAnd = 0;
//will say if it has a > in its 2nd index
int hasGreat = 0;
//will say what index has the >
int greatIndex = -1;
//check if there is a < in one of its places
int hasLess = 0;
//will say which index it has the < in
int lessIndex = -1;


//clean a 1d array
void cleanSingle(int n, char array[]){
  int i;
  for(i = 0; i < n; i++){
    array[i] = '\0';
  }
}
//clean a 2d array
void cleanDouble(int m, int n, char array[][n]){
  int i, j;
  array[0][0] = '\0';
  for(i = 0; i < m; i++){
    for(j = 0; j < n; j++){
      array[i][j] = '\0';
    }
  }
}
//cleans out a pointer char array
void cleanInput(int n, char* array[]){
  int i;
  for(i = 0; i < n; i++){
    array[i] = NULL;
  }
}

//if not running in background
void getStatus(){
  //use waitpid() and wait()
  int childExitMethod;
  pid_t childPID = wait(&childExitMethod);
  //if the pid isnt created
  if(childPID == -1){
    printf("exit value 1\n");
  }
  else if(WIFEXITED(childExitMethod)){
    int exitStatus = WEXITSTATUS(childExitMethod);
    printf("background pid %d is done: exit value %d\n", currpid, exitStatus);
  }
  else if(WIFEXITED(childExitMethod) != 0){
    int termSignal = WTERMSIG(childExitMethod);
    printf("background pid %d is done: terminated by signal %s\n", currpid, termSignal);
  }
}

void cdCommand(){
  //must start in the HOME directory, not the directory smallish is located in, unless it is in HOME
  char* dirHome = getenv("HOME");
  char cwd[200];
  //user wants to go back to home directory
  if(strcmp(input[1], "\0") == 0 ){
    //change to home directory
    chdir(&dirHome);

    //continue moving backwards in directories until reach home directory
    while(strcmp(cwd, dirHome) != 0){
      chdir("..");
      getcwd(cwd, sizeof(cwd));
    }
  }

  //user wants to go to a directory found within the current directory
  else if(strcmp(input[1], "\0") != 0 && strcmp(input[2], "\0") == 0){
    //varaible to show if full directory path
    int absolute = 0;
    int wasSlash = 0;
    for(i = 0; i < 50; i++){
      if(input[1][0] == '/' && i != 0 ){
        input[1][i-1] = input[1][i];
        absolute = 1;
        wasSlash = 1;
      }
      else if(wasSlash == 1 && input[1][i] != '\0'){
        input[1][i-1] = input[1][i];
      }
      else if(input[1][0] != '/' && input[1][i] == '/'){
        absolute = 1;
      }
      else if(input[1][i] == '\0'){
        input[1][i-1] = '\0';
        break;
      }
    }
    //if its an absolute file path
    if(absolute == 1){
      //continue moving backwards in directories until reach home directory
      while(strcmp(cwd, dirHome) != 0){
        chdir("..");
        getcwd(cwd, sizeof(cwd));
      }
      //then go to the directories they want
      chdir(cdName);
      getcwd(cwd, sizeof(cwd));
      //printf("cwd: %s\n", cwd);
    }
    //if relative call
    else if(absolute == 0){
      //goes into the directory, but if there is no directory of that name, send an error
      if(chdir(cdName) != 0){
        perror("");
        //exit(2);
      }
    }
  }
  getcwd(cwd, sizeof(cwd));
}

//the function that has all the command if statements
void possibleCommands(){
    //create variables for pid
    spawnpid = -5;

      //if a background process (meaning it has a & at the end)
      if(hasAnd == 1){
        execvp(args[0], args);
        //getBackgroundStatus();
      }
      //else{
        //> moves things from the left into the file or directory on the right
        // only a > has been given, no < after it
        if(hasLess == 1 || hasGreat == 1){
          int write = 0;
          int read = 0;
          char out[512];
          char in[512];

          if(length > 1 && strcmp(args[length-2], ">") == 0){
            //out = args[length-1];
            strcpy(out, args[length-1]);
            args[length-2] = NULL;
            args[length-1] = NULL;
            length = length-2;
            write = 1;
          }

          if(length > 1 && strcmp(args[length-2], "<") == 0){
            //in = args[length-1];
            strcpy(in, args[length-1]);
            args[length-2] = NULL;
            args[length-1] = NULL;
            length = length-2;
            read = 1;
          }

            //now try to open the files
            int fileout;
            int filein;
            //if the user wanted to write to a file
            if(write == 1){
              //open file with writing properties
              if((fileout = open(out, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0){
                perror(out);
                return(-1);
              }
              //send it to dup to do things on
              dup2(fileout, 1);
            }

            //if user wanted to open a file to read
            if(read == 1){
              //open file with read only properties
              if((filein = open(in, O_RDONLY, 0644)) < 0){
                printf("cannot open %s for input\n", in);
                return(-1);
              }
              dup2(filein, 0);
            }
            //then do within teh files what is asked by the user
            execvp(args[0], args);
            printf("cannot open %s for input\n", in);

            //close all files
            if(write == 1){
              close(fileout);
            }
            if(read == 1){
              close(filein);
            }
      }
      else{
        //if you weren't trying to open a file, then just go through this way to do what you need to do in bash
        execvp(args[0], args);
        printf("%s: no file or directory found\n", input[0]);
      }
}
void catchSIGINT(int signo){
  char* message = "Caught SIGINT, sleeping for 5 seconds\n";
  write(STDOUT_FILENO, message, 38);
  raise(SIGUSR2);
  sleep(5);
}
void catchSIGUSR2(int signo)
{
//char* message = "Caught SIGUSR2, exiting!\n";
//write(STDOUT_FILENO, message, 25);
exit(0);
}

/*void catchSIGSTP(int signo)
{

  if(fg == 0){
    fg = 1;
    char* messageIn = "Entering foreground only mode!\n";
    write(STDOUT_FILENO, messageIn, 35);
  }
  else if(fg == 1){
    fg = 0;
    char* messageOut = "Exiting foreground only mode!\n";
    write(STDOUT_FILENO, messageOut, 35);
  }
//char* message = "Caught SIGUSR2, exiting!\n";
//write(STDOUT_FILENO, message, 25);
exit(0);
}*/

int main(){
  pidD = getpid();
  pids[0] = pidD;
  processes++;

//taken from Signals slides on Canvas
struct sigaction SIGINT_action = {0}, SIGUSR2_action = {0}, ignore_action = {0}, SIGSTP_action = {0};

SIGINT_action.sa_handler = catchSIGINT;
sigfillset(&SIGINT_action.sa_mask);
SIGINT_action.sa_flags = 0;

SIGUSR2_action.sa_handler = catchSIGUSR2;
sigfillset(&SIGUSR2_action.sa_mask);
SIGUSR2_action.sa_flags = 0;
/*
SIGSTP_action.sa_handler = catchSIGSTP;
sigfillset(&SIGSTP_action.sa_mask);
SIGSTP_action.sa_flags = 0;
*/
ignore_action.sa_handler = SIG_IGN;

sigaction(SIGINT, &SIGINT_action, NULL);
//sigaction(SIGTSTP, &SIGSTP_action, NULL);
sigaction(SIGUSR2, &SIGUSR2_action, NULL);
sigaction(SIGTERM, &ignore_action, NULL);
sigaction(SIGHUP, &ignore_action, NULL);
sigaction(SIGQUIT, &ignore_action, NULL);

//keep all of this going until the user decides to quit
do{
//flush out output buffer before getting input
  fflush(stdin);
  fflush(stdout);
  //Clean answer out
  cleanSingle(2049, answer);
  //clean input out
  cleanDouble(513, 50, input);
  //get input from the user (command line)
  printf(": ");
  fgets(answer, 2049, stdin);

  i;
  j = 0;
  k;
//chec to see if it has an & at the end
  hasAnd = 0;
//will say if it has a > in its 2nd index
  hasGreat = 0;
//will say what index has the >
  greatIndex = -1;
//check if there is a < in one of its places
  hasLess = 0;
//will say which index it has the < in
  lessIndex = -1;

//check for $$, which then replaces them with the shell's pid
  for(i = 0; i < 2049; i++){
    if(answer[i] == '$'){
      if(answer[i+1] == '$'){
        answer[i] = '\0';
        answer[i+1] = '\0';
        char b[50];
        sprintf(b, "%d", pidD);
        strcat(answer, b);
        //alength++;
        //printf("alength: %d\n", alength);
      }
    }
  }

int alength = strlen(answer);

  //parse the command line input
  for(i = 0; i < 513; i++){
    k = 0;
    while(j < strlen(answer)){
      if(answer[j] != ' ' && answer[j] != '\0' && answer[j] != '\n'){
        if(answer[j] == '>'){
           hasGreat = 1;
           greatIndex = i;
        }
        else if(answer[j] == '<'){
          hasLess = 1;
          lessIndex = i;
        }
        //if there is a &, dont add to 2d array, but make variable 1 so the rest of the program knows
        if(answer[j] == '&' && j == (alength-2)){
          hasAnd = 1;
        }
        else{
          input[i][k] = answer[j];
        }
      }
      //if we get to a space between commands, or the end of the command break out
      else if(answer[j] == ' ' || answer[j] == '\0'){
        j++;
        break;
      }
      else{
        j++;
        break;
      }
      j++;
      k++;
    }
  }

//copy into char because had trouble when using for cd
strcpy(cdName, input[1]);

//reset child method each time
childExitMethod = -5;

//find length of input
length = 0;
for(i = 0; i < 513; i++){
  if(strcmp(input[i], "\0") != 0){
        length++;
  }
}
//clean the array for the next set of input
cleanInput(513, args);
//put all of command line into the a new pointer 1d array
for(i = 0; i < length; i++){
  args[i] = input[i];
}

//if statement for exit
//if the user enters the word exit, all processes will be stopped and the script exited
if(strcmp(input[0], "exit") == 0 && strcmp(input[1], "\0") == 0){

    //stopre all pids in pids[] and then have a while loop that goes through all pids until they are all killed, and then exit
    for(i = 0; i < processes; i++){
      kill(pids[i], SIGKILL);
    }

    //then exit out of the program (shell)
    ex = 1;
    exit(0);
    break;

}

//just skip if they enter nothing or a comment
if(input[0][0] == '#' || strcmp(input[0], "\0") == 0){
  //do nothing because its a comment
}

//if they want statuses
else if(strcmp(input[0], "status") == 0){
  if(hasAnd == 1){
    getStatus();
  }
  else{
   printf("exit value %d\n", exitType);
  }
}
//if they want cd
else if(strcmp(input[0], "cd") == 0){
  cdCommand();
}

else{
  //if user has & wants to be run in background
  if(hasAnd == 1){
    spawnpid = fork();
    pids[processes] = spawnpid;
    backgroundpid = spawnpid;
    processes++;
    //if its not a built in function, fork it and send it through to bash
      switch(spawnpid){
        //if something bad happens
        case -1:
          printf("exit value 1\n");
        //in fork, child process returns 0
        //this is where all child processes will happen
        case 0:
          sleep(1);
          possibleCommands();

        //in fork, parent process returns pid of child process created
        default:
            //getStatus();
            printf("background pid is %d\n", spawnpid);
            signal(SIGINT, getStatus);
            signal(SIGCHLD, SIG_IGN);
            fflush(stdout);
    }
  }
  //if user has no & wants to be run in foreground
  else if(hasAnd == 0){
    spawnpid = fork();
    currpid = spawnpid;
    pids[processes] = spawnpid;
    processes++;
  //if its not a built in function, fork it and send it through to bash
    switch(spawnpid){
      //if something bad happens
      case -1:
        printf("Exit value 1\n");

      //in fork, child process returns 0
      //this is where all child processes will happen
      case 0:
        //passes into bash functioning
        possibleCommands();

      //in fork, parent process returns pid of child process created
      default:
        //sleep(2);
        signal(SIGINT, getStatus);
        signal(SIGCHLD, SIG_IGN);
        waitpid(spawnpid, &childExitMethod, 0);
	if(WIFEXITED(childExitMethod)){
          //exitType = 1;
          exitType = WEXITSTATUS(childExitMethod);
          //statPrint(exitType, exitStatus);
          //printf("exit value %d\n", currpid, exitStatus);
        }
        if(strcmp(input[0], "pkill") == 0 && WIFSIGNALED(childExitMethod)){
            int signal = WTERMSIG(childExitStatus);
            //printf("%d\n", signal);
            printf("background pid %d done: terminated by signal 15\n", backgroundpid);
          }
        fflush(stdout);
    }
  }
}

}while(ex == 1);
return 0;
}
