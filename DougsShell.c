//////////////////////////////////////
//Programmer: Doug Tait             //
//Date: 2.15.2013                   //
//File: MyShell.c                   //
//////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

//----------------------------------//
//--------GLOBAL-CONSTANTS----------//
//----------------------------------//

#define COMMAND_SIZE 100 
#define ARGUMENT_SIZE 50 
#define HISTORY_CAPACITY 10 
#define FALSE 0
#define TRUE 1

//----------------------------------//
//-------FUNCTION-PROTOTYPES--------//
//----------------------------------//

void GetCommand(char *command);
void ParseCommand(char *command, char **args); //tokenize command into separate strings
int ExecuteCommand(char **args);   
void MonitorChildProcess();   //parent calls this to check that child process executes 
int CheckForContinue(char *command); //check whether or not "exit" or "logout" was given as a command
void AddToHistory(char **history, char *command, int *list_size); 
void PrintHistory(char **history, int list_size);
void GetEcho(char **history, char *command, int list_size); //determine proper command from !! or !N

int main()
{
 
  int history_size = 0;       //size of history list 
  char command[COMMAND_SIZE];  //string for most recent command
  char *arguments[ARGUMENT_SIZE]; //array of arguments to send to exec
  char *history[HISTORY_CAPACITY]; //array of strings for history
  struct sigaction ignore;   //set of signals for signal handlers
  
  //set all array values to null
  memset(command,'\0',COMMAND_SIZE - 1);
  memset(arguments,'\0',ARGUMENT_SIZE - 1);
  memset(history, '\0',HISTORY_CAPACITY - 1);

  //set sigaction struct
  ignore.sa_handler = SIG_IGN;
  ignore.sa_flags = 0;
  sigemptyset(&(ignore.sa_mask));
  sigaddset(&(ignore.sa_mask),SIGINT);

  //ignore SIGINT and SIGTSTP signals
  sigaddset(&(ignore.sa_mask),SIGTSTP);
  sigaction(SIGINT,&ignore,NULL);
  sigaction(SIGTSTP,&ignore,NULL);
 
  GetCommand(command);   //get command from user
  if(command[0] == '!' && strlen(command) == 2)//check for !! or !n and determine proper command
  {
      GetEcho(history, command, history_size);
  }
  if(command[0] != '\n' && command[0] != '!')//do not add endline, !!, or !N commands to history
    {
      AddToHistory(history,command, &history_size);//add most recent command to history
    }
  
  //while command is not "exit" or "logout" or command is newline. get and process commands
  while(CheckForContinue(command) == TRUE || command[0] == '\n') 
    {
      //if the command entered is not endline then it should be tokenized
      if(command[0] != '\n' && strcmp("history",command) != 0)
	{
	  ParseCommand(command,arguments); //populate arguments array by tokenizing commands
	  int child_pid = fork();   //fork child process to make call to exec
	  //determine proper action after fork
	   switch(child_pid)
	     {
	     case 0: /*child process*/
	       ExecuteCommand(arguments);
	       break;
	     case -1:
	       perror("Error: command cannot be executed due to failed fork command\n");
	       exit(1);
	     default:/*parent process*/
	       MonitorChildProcess(child_pid);//check to see that child process 
	       break;                         //executes properly
	       }
	 }
      if(strcmp("history",command) == 0)//check if command is "history"
	{
	  //print history
	  PrintHistory(history, history_size);
	}

       //get new command
      GetCommand(command);

      if(command[0] == '!' && strlen(command) == 2)//check for !! or !n and determine proper command
	{
	   GetEcho(history, command, history_size);
	}
      if(command[0] != '\n' && command[0] != '!')
        {
           AddToHistory(history,command,&history_size);
        }

       //the next command may have fewer characters than the last
      memset(arguments,'\0',ARGUMENT_SIZE - 1); 
      
    }
 
  exit(0);

}

//-----------------------------------//
//-------FUNCTION-DEFINITIONS--------//
//-----------------------------------//

//Function: GetCommand
//Purpose: Retrieve command from user
void GetCommand(char *command)
{
    memset(command,'\0',COMMAND_SIZE - 1);
    char *endline;    //pointer to a character in command
   
    printf("DougsShell--->");//prompt user

    if((fgets(command,COMMAND_SIZE,stdin)) == NULL)
      {
	perror("command input failed\n");
      }

    // if user entered command, replace newline with '\0'
    else if(command[0] != '\n' && (endline = strchr(command, '\n')) != NULL)
      {
        *endline = '\0';
      }
}

//Function: ParseCommand
//Purpose: Tokenize command in order to supply execvp with args[] array
void ParseCommand(char *command, char **args)
{

  char *tokenPtr; //string pointer to track tokens
  int i = 0;
    
  tokenPtr = strtok(command," ");

  while(tokenPtr != NULL)
    {
       args[i++] = tokenPtr;
       tokenPtr = strtok(NULL," ");
    }
}

//Function: ExecuteCommand
//Purpose: Execute command when called from child process
int ExecuteCommand(char **args)
{    
  execvp(args[0],args); //execute command
  printf("command failed to execute\n");
  exit(1); //exit with non-zero value since command was not executed
}

//Function: MonitorChildProcess
//Purpose: MonitorChildProcess is called from parent process and checks the exit
//         code of child process
void MonitorChildProcess(int child_pid)
{
  int received_pid, status;
    
    while(received_pid = wait(&status))
      {
	if(received_pid == child_pid)
	  {
	    break;
	  }
	if(received_pid == -1)
	  {
	    perror("error while waiting for child process to execute command");
	    return;
	  }
      }

    if(WIFEXITED(status) == 0)
      {
	printf("child process exited with value %d\n", WEXITSTATUS(status));
      }
}

//Function:CheckForContinue
//Purpose: Check if command is "exit" or "logout"
int CheckForContinue(char *command)
{
  
  if((strcmp("exit",command) == 0) || (strcmp("logout",command) == 0))
    {
      return FALSE;
    }
   
  else
    {
      return TRUE;
    }
}

//Function: AddToHistory
//Purpose: Add command to history and adjust history list
//         when list size is equal to history capacity
void AddToHistory(char **history, char *command, int *list_size)
{
  //keep track of where to add the next string in the array
  int index_to_access = *(list_size);
  char commandCopy[COMMAND_SIZE]; //string to hold copy of most recent command
  strcpy(commandCopy,command);

  //when history is too big, remove oldest command
      if(index_to_access ==  HISTORY_CAPACITY)
	{
	  int i;
	  for(i = 0; i <= index_to_access - 2;i++)//shift history entries
	    {
	      free(history[i]); //free memory pointed to by history[i]
	      history[i] = malloc(strlen(history[i+1] + 1)); //allocate memory by size of next/newer element
	      strcpy(history[i],history[i+1]); //copy next/newer element to proper location
	     }
	  //assign command most recently entered by user
	  free(history[index_to_access - 1]);
	  history[index_to_access - 1] = malloc(strlen(commandCopy) + 1);
	  strcpy(history[index_to_access - 1], commandCopy);
	 
	}
      else
	{  //copy command into history array
	  history[index_to_access] = malloc(strlen(commandCopy + 1));
	  strcpy(history[index_to_access], commandCopy);
	  // printf("the size of this command is %d",strlen(history[index_to_access]));
	  index_to_access++;
     	}

      *(list_size) = index_to_access;//update history size
  
}

//Function: PrintHistory
//Purpose: Print history
void PrintHistory(char **history, int list_size)
{ 
  int i = 0;
  for(i = 0; i <= list_size - 1; i++)
    {
      printf("%d. %s\n",i+1,history[i]);
    }
}

//Function: GetEcho
//Purpose: Determine correct history element to echo as next
//         command
void GetEcho(char **history, char *command, int list_size)
{
  int intValue = atoi(&command[1]);
  if(command[1] == '!')
    {
      intValue = 1; //list_size - 1 will be the most recent command
    }

  if(intValue > list_size)//cannot echo commands that have not been entered
    {
      printf("error: there have not been %d command(s) entered",intValue);
    }
  else
    {
      //copy proper command from history
      strcpy(command,history[list_size - intValue]);
      printf("%s\n", command);
      sleep(1); //sleep for a second for a nice effect 
    }
 
}
 
