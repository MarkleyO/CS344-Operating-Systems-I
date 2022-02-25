#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>//taking in all the .h files that are required for the project

int getInput(char* input);
void execArgForeground(char** args);
int parseArgs(char* input, char** args);
void catchSIGINT(int signo);
void catchSIGTSTP(int signo);
void runBackground(char** args, int* children, int* numChildren);
void checkBuiltIns(char** args);
void printArgs(char** args);
void expandParentPIDVariable(char* unformatedInput);
void displayStatus();
void redirection(char** args);
void shakeTheBaby(int* children, int* numChildren);

int isBG = 0;//tells whether or not the current process is background
int bGAllowed = 1;//toggle for foreground only mode
int status = 0;// exitstatus of last process
int numArgs;//tells how many arguments were passed through

int main(){

	char inputString[2048];//raw input string
	char* parsedArgs[512];//array of parsed arguments
	int childPIDs[100];//holds the pids of background processes
	memset(childPIDs, '0', 50);//makes sure array is clean
	int numChildren = 0;//number of child arrays
	int i, j;//for indexing

	struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};//creates struct for the signal handlers

	// SIGINT_action.sa_handler = catchSIGINT;
	SIGTSTP_action.sa_handler = catchSIGTSTP;//registers new function for the handler
	// sigfillset(&SIGINT_action.sa_mask);
	sigfillset(&SIGTSTP_action.sa_mask);//fills the list of signals that are held
	// sigaction(SIGINT, &SIGINT_action, NULL);
	SIGTSTP_action.sa_flags = SA_RESTART;//assigns to flag variable in struct
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);//doesn't do anything now


	while(1){
		isBG = 0;//default is not to be background
		shakeTheBaby(childPIDs, &numChildren);//kills all the children that are done doing their thing

		getInput(inputString);//gets input from the command line
		if(inputString[0] == '#')//checks for comments
			continue;

		expandParentPIDVariable(inputString);//expands the money signs if present

		numArgs = parseArgs(inputString, parsedArgs);//parses the argument into different pieces
		if(parsedArgs[0]==NULL)//if there is an empty line skip everything else
			continue;
		
		//background processes
		if(strcmp(parsedArgs[numArgs-1], "&") == 0){//looks to see if a process is a background process
			parsedArgs[numArgs-1] = NULL;
			if(bGAllowed == 1)
				isBG = 1;
		}
		checkBuiltIns(parsedArgs);//checks for built in functions

		if(isBG == 1)
			runBackground(parsedArgs, childPIDs, &numChildren);//runs in background
		else
			execArgForeground(parsedArgs);//runs in foreground
	}
	return 0;
}

int getInput(char* input){
	char* bufferString;
	size_t bufferSize = 0;//variables to be set up to get input
	int numChars = -5;
	int currChar = -5;

	while(1){
		printf(":");//prints out to indicate user can type
		fflush(stdout);
		numChars = getline(&bufferString, &bufferSize, stdin);//grabs line
		if(numChars == -1)
			clearerr(stdin);
		else
			break;
	}
	bufferString[strcspn(bufferString, "\n")] = '\0';//replaces new line with null
	strcpy(input, bufferString);//copies to the main array
	free(bufferString);
	bufferString = NULL;

	return 1;
}

//returns the number of arguments
int parseArgs(char* input, char** args){
	int i = 0, j;//index vars
	char* temp = strtok(input, " ");//grabs first instance till space

	while(temp != NULL){//keeps going from last position, grabbing until spaces 
		args[i] = temp;//adds to the array of args
		temp = strtok(NULL, " ");
		i++;
	}
	free(temp);
	args[i] = NULL;//makes last arg a null

	return i;
}

void checkBuiltIns(char** args){
	int i;
	if(strcmp(args[0],"exit") == 0){//if exit is entered, exit the shell
		status = 0;
		exit(0);
	}
	else if(strcmp(args[0], "psme") == 0){//for my own purposes
		char* psme[] = {"ps", "-o", "ppid,pid,euser,stat\%cpu,rss,args", NULL};
		//adjust to allow for bg later
		for(i=0; i<4; i++)
			args[i] = psme[i];
	}
	else if(strcmp(args[0], "cd") == 0){//allows for file navigation
		if(args[1] == NULL){
			chdir(getenv("HOME"));//defaults to the home folder
		}
		else{
			chdir(args[1]);//navigates to the argument first after cd
		}
		args[0] = NULL;//set to empty so that the args are not put into the execvp
	}
	else if(strcmp(args[0], "status") == 0){//calls status function if first arg is status
		displayStatus();
		status = 0;
		args[0] = NULL;//same as above function
	}
}

void execArgForeground(char** args){
	int execStatus;
	pid_t spawnPID = -5;//holds process id
	int childExitMethod = -5;
	spawnPID = fork();//creates the child copy process


	if(spawnPID == -1){//error
		perror("something is wrong");
		status = 1;
		exit(1);
	}else if(spawnPID == 0){//child
		redirection(args);//looks to see if there are things to redirect
		execStatus = execvp(args[0], args);//executes the arguments
		if(execStatus == -1){
			printArgs(args);
			printf(": no such file or directory\n");
			fflush(stdout);
		}
		status = execStatus;
		exit(0);
	}else{//parent
		waitpid(spawnPID, &childExitMethod, 0);//waits to continue until the children are dead
	}
}

void runBackground(char** args, int* children, int* numChildren){
	//this function is the same as above except for the lower part
	int execStatus;
	int i;
	pid_t spawnPID = -5;
	int childExitMethod = -5;//t
	spawnPID = fork();

	if(spawnPID == -1){
		perror("something is wrong");
		status = 1;
		exit(1);
	}else if(spawnPID == 0){//child
		redirection(args);
		execStatus = execvp(args[0], args);
		if(execStatus == -1){
			printArgs(args);
			printf(": no such file or directory\n");
			fflush(stdout);
		}
		status = execStatus;
		exit(0);
	}else{//parent
		printf("background pid is %d\n", spawnPID);//prints bg id of child process
		for(i=0; i<*numChildren; i++)//selects proper location in memory
			children++;
		*children = spawnPID;//assigns id to the arrray of pids
		(*numChildren)++;//increments the variable
	}
}

void catchSIGINT(int signo){
	//char* message 
}

void catchSIGTSTP(int signo){
	//toggles between allowing or not allowing background commands, write must be used in here not printf
	if(bGAllowed){
		char* noBgMsg = "\nEntering foreground-only mode (& is ignored)\n:";
		write(1, noBgMsg, 48);
		fflush(stdout);
		bGAllowed = 0;
	}else if(!bGAllowed){
		char* yBgMsg = "Exiting foreground-only mode\n:";
		write(1, yBgMsg, 30);
		fflush(stdout);
		bGAllowed = 1;
	}
}

void expandParentPIDVariable(char* unformatedInput){
	//looks for the pid variable symbol. if this is found, then it is replaced in the user input string with the number of the pid
	int pid = getpid();
	char* temp;
	char *varLoc = strstr(unformatedInput, "$$");
	if(varLoc != NULL){
		temp = strtok(unformatedInput, "$$");
		sprintf(unformatedInput, "%s%d",temp,pid);
	}
}

void printArgs(char** args){
	int i;
	for(i=0; i<numArgs; i++){
		printf("%s ", args[i]);
		fflush(stdout);
	}
}

void displayStatus(){
	//prints out most recent exit value
	printf("exit value: %d\n", status);
	return;
}

void redirection(char** args){
	int fileDescriptorInput = -5, fileDescriptorOutput = -5, i = 0;
	char *backgroundpath = "/dev/null";//used for background processes
	char filename[50];
	memset(filename, '\0', 50);

	while(args[i] != NULL){
		if(strcmp(args[i], "<") == 0){//looks for symbol
			strcpy(filename, args[i+1]);
			fileDescriptorInput = open(filename, O_RDONLY);//attempts to open file arg passed
			if(fileDescriptorInput == -1){
				printf("File: %s, could not be opened.\n", filename);
				fflush(stdout);
				status = 1;
				exit(1);
			}
			if(dup2(fileDescriptorInput, 0) == -1){//rerouts to file
				perror("dup2");
				status = 1;
				exit(1);
			}
			close(fileDescriptorInput);//closes after done
		}
		if(strcmp(args[i], ">") == 0){//same as above but for output file not input
			strcpy(filename, args[i+1]);
			fileDescriptorOutput = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0600);
			if(fileDescriptorOutput == -1){
				printf("File: %s, could not be opened.\n", filename);
				fflush(stdout);
				status = 1;
				exit(1);
			}
			if(dup2(fileDescriptorOutput, 1) == -1){
				perror("dup2");
				status = 1;
				exit(1);
			}
			close(fileDescriptorOutput);
		}
		i++;
	}

	i=0;//replaces the file arguments from redirect and symbols in the args
	while(args[i] != NULL){
		if(strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0){
			args[i] = NULL;
			args[i+1] = NULL;
			i++;
		}
		i++;
	}
	//following two large if statments check to see if a file has been apssed through and if a background process is here, if 
	//both are true, then the defulat file path is applied
	if(fileDescriptorInput == -5 && isBG == 1){
		i = open(backgroundpath, O_RDONLY);
		if(i == -1){
			perror("open");
			fflush(stdout);
			exit(1);
		}
		if(dup2(i, 0) == -1){

		}
	}
	if(fileDescriptorOutput == -5 && isBG == 1){
		i = open(backgroundpath, O_WRONLY | O_TRUNC | O_CREAT, 0600);
		if(i == -1){
			perror("open");
			fflush(stdout);
			exit(1);
		}
		if(dup2(i, 1) == -1){

		}
	}
}

void shakeTheBaby(int* children, int* numChildren){
	int i;
	int exitVar;
	//reaps the zombie processes that are left hanging around
	for (i=0; i<*numChildren; i++){
		//non zero means it has to be a pid
		if(*children != 0){
			if(waitpid(*children, &exitVar, WNOHANG)){//checks to see if a process is done
				if(WIFEXITED(exitVar) != 0){//checking for a exit
					printf("[%d] finished PID: %d Exit Value: %d\n", i, *children, WEXITSTATUS(exitVar));
					fflush(stdout);
				}
				if(WIFSIGNALED(exitVar) != 0){//checking for a signal
					printf("[%d] finished PID: %d Term Signal: %d\n", i, *children, WTERMSIG(exitVar));//fix later
					fflush(stdout);
				}
				*children = 0;
			}
			//printf("[%d] PID: %d\n", i, *children);
			//fflush(stdout);
		}
		children++;
	}
}