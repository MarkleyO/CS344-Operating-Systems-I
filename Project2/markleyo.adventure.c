#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

pthread_mutex_t MUTEX;//kind of chooses which thread is running

//room struct that files are read into
struct Room{
	char name[10];
	char type[11];

	int numConnects;
	char connects[6][10];//connections are a string in here rather than the numbers in buildrooms
};

//node is used for the linked list to store the path taken by the player/user
struct Node{
	char* roomPassed;
	struct Node* next;
};

char* getNewestDirectory();
void readRoomsIn(char* roomDirectory, struct Room maze[7]);
int countConnections(char* fileName);
int findStartRoom(struct Room maze[7]);
int findEndRoom(struct Room maze[7]);
void displayCurrentRoom(struct Room maze[7], int index);
int isValidInput(struct Room maze[7], int currentRoom, char line[]);
int switchRooms(struct Room maze[7], char line[]);
void printMaze(struct Room maze[7]);
struct Node* addToRecord(struct Node* list, char roomName[]);
void printListReversed(struct Node* head);
void freeListMemory(struct Node* head);
static void* timeSet(void* arg);

int main(){
	pthread_mutex_init(&MUTEX, NULL);//initializes the mutex thread

	char* newestDirectory = getNewestDirectory();//gets the most recently created directory
	struct Room maze[7];//holds the seven rooms in an array
	readRoomsIn(newestDirectory, maze);//reads in the data from the files and stores into the struct array
	struct Node* head = NULL;//begins the linked list used to store path
	FILE* timeFile;//file is for writing to the currentTime file 
	char* timeContent = NULL;//used for the process of reading in from the time file
	size_t length;//same as above
	int currentRoomIndex = findStartRoom(maze);//finds the start room from the array, and sets the current room index to this
	int endRoomIndex = findEndRoom(maze);//same as above but for the end, provides a way to know when the game is over
	int stepCount = 0;//counts how many steps have been taken
	char line[1024];//stores the text gathered from the player on the command line

	while(currentRoomIndex != endRoomIndex){//while the room the user is at is not the last, keep playing
		displayCurrentRoom(maze, currentRoomIndex);//prints out the data of the current room from the room array
		do{
			scanf("%s", line);//prompts user for input, reads as string
			if((isValidInput(maze, currentRoomIndex, line) == 0) && (!strstr(line, "time"))){//if it is an invalid command(not a good connection) and not time, redisplay and continue to prompt
				printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
				displayCurrentRoom(maze, currentRoomIndex);
			}else if(strstr(line, "time")){//if time is entered a new thread is created here, and the time is displayed, the player does notmove
				pthread_t timethread;//data type returned by create function
				pthread_create(&timethread, NULL, &timeSet, NULL);//creates the actual new thread, as well as tells what function to run in new thrad
				pthread_join(timethread, NULL);//waits for a thread to end
				pthread_mutex_lock(&MUTEX); //holds mutex on current thread
				timeFile = fopen("currentTime.txt", "r");//reads the file to see what time was put on the file in function called in create
				getline(&timeContent, &length, timeFile);//gets line with time added to the text file
				fclose(timeFile);//closes the file after getting the data
				pthread_mutex_unlock(&MUTEX);//unlocks the mutex since this thread is done by this point
				printf("\n%s\nWHERE TO? >", timeContent);//prints out the data gathered from the file to the user
				free(timeContent);//frees the file pointer created earlier
			}else{//if a valid move has been made, it will be added to the path list to keep track for the end of the game
				head = addToRecord(head, line);
			}
		}while(isValidInput(maze, currentRoomIndex, line) == 0);//loop repeats as long as valid input has not been entered, aka a proper move forward has not been taken
		currentRoomIndex = switchRooms(maze, line);//sets the current room inded to the index of the room with the name entered by the user
		stepCount++;//counts a step as being taken
	}

	printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\nYOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepCount);//if above loop is broken game is won
	printListReversed(head);//list is printed in reversed order, since items are pushed to top, in a stack manner

	freeListMemory(head);//memory used by list is freed
	free(newestDirectory);//directory opened is freed

	pthread_mutex_destroy(&MUTEX);//mutex is destroyed

	return 0;
}

//gets the newest directory, this mostly comes from the example code provided on canvas
char* getNewestDirectory(){
	char* nameToReturn;//name of the newest directory
	int newestDirTime = -1;//current newest time of creation
	char targetDirPrefix[32] = "markleyo.rooms.";//prefix to look for in current dir
	char newestDirName[256];//name of newset directory
	memset(newestDirName, '\0', sizeof(newestDirName));//adjusts memory

	DIR* dirToCheck;//current directory being checked
	struct dirent *fileInDir;//stores info on directory
	struct stat dirAttributes;//more info on directory

	dirToCheck = opendir(".");//opend the current directory

	if(dirToCheck > 0){//if the directory opened is valid, proceed
		while((fileInDir = readdir(dirToCheck)) != NULL){ //reads the directories from the current location, until there are no more
			if(strstr(fileInDir->d_name, targetDirPrefix) != NULL){//checks to see if the prefix is present in the name of the dir
				stat(fileInDir->d_name, &dirAttributes);//gets the info and stores into attributes
				if((int)dirAttributes.st_mtime > newestDirTime){//if the createion time is more recent that the current newest it will be saved as the new newest
					newestDirTime = (int)dirAttributes.st_mtime;
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, fileInDir->d_name);
				}
			}
		}
	}

	closedir(dirToCheck);//after search is complete, the directory is closed
	nameToReturn = malloc(strlen(newestDirName)+1);//space is created to store the name in the string
	strcpy(nameToReturn, newestDirName);//copied over a new string to be returned
	return nameToReturn;
}

//reads all the data from the room files into the array of structs
void readRoomsIn(char* roomDirectory, struct Room maze[7]){
	DIR* roomDirect = opendir(roomDirectory);//selects the new directory to look at
	int i = 0;//exists because the first two are . and ..
	int j;
	FILE* currentRoomFile;//current file being looked at
	char buffer[256];//holds current string being read in

	int numConnections; // subtract 2 to get number of functions
	struct dirent *currentRoom;//holds info from current file

	chdir(roomDirectory);//changes directory to proper directry
	

	if(roomDirect){//if directory is valid proceed
		while((currentRoom = readdir(roomDirect)) != NULL){//as long as there are files to read, continue reading
			if(i>1){//only attempt to truly read file if . and .. are passed
				currentRoomFile = fopen(currentRoom->d_name, "r");//this is the file being read
				//a file has been selected here
				if(currentRoomFile){//if a valid file is chosen then proceed
					numConnections = countConnections(currentRoom->d_name);//counts the number of connections present in file
					maze[i-2].numConnects = numConnections;//sets this to the var in the struct

					fgets(buffer, 11, currentRoomFile);//skips the first eleven character
					fscanf(currentRoomFile, "%s", buffer);//gets name string
					strcpy(maze[i-2].name, buffer);//copies to string in struct
					fgets(buffer, 2, currentRoomFile);//proceeds to next line

					for(j=0; j<numConnections; j++){//repeat for proper number of connections
						fgets(buffer, 14, currentRoomFile);//skips first 14 characters
						fscanf(currentRoomFile, "%s", buffer);//gets string for connection
						strcpy(maze[i-2].connects[j], buffer);//copies connection to array of connections as string
						fgets(buffer, 2, currentRoomFile);//proceeds to next line
					}

					fgets(buffer, 11, currentRoomFile);//skips first 11 characters
					fscanf(currentRoomFile, "%s", buffer);//gets string of type to buffer
					strcpy(maze[i-2].type, buffer);//copies this to the struct array

					fclose(currentRoomFile);//closes seleced file
				}	
			}
			i++;//increment what file you are on
		}
		chdir("..");//moves back directory so that the time file will be created in proper location
	}
}

//counts the number of connections by subtracting two from the line ocunt
int countConnections(char* fileName){
	FILE* toBeCounted = fopen(fileName, "r");//reads in a file
	int characters = 0;
	int lines = 0;

	if(toBeCounted == NULL)
		return 0;

	while((characters = fgetc(toBeCounted)) != EOF){//looks through and counts how many new line characters there are
		if(characters == '\n')
			lines++;
	}
	fclose(toBeCounted);//close file
	return lines - 2;//return the proper number of connections since two lines are name and type
}

//prints out the entire maze, not used anymore in final product
void printMaze(struct Room maze[7]){
	int i, j;
	for(i=0; i<7; i++){
		printf("Name: %s, Type: %s, NumConnects: %d\n", maze[i].name, maze[i].type, maze[i].numConnects);
		for(j=0; j<maze[i].numConnects; j++){
			printf("\tConnection %d: %s\n", j+1, maze[i].connects[j]);
		}
	}
}

//finds the start room and returns index in array
int findStartRoom(struct Room maze[7]){
	int i;
	for(i=0; i<7; i++){//looks to see if there is a file with type string containing start
		if(strstr(maze[i].type, "START") != NULL){
			return i;//if found returns index
		}
	}
	return -1;
}

//finds the last room in the same manner as the above find start room function
int findEndRoom(struct Room maze[7]){
	int i;
	for(i=0; i<7; i++){
		if(strstr(maze[i].type, "END") != NULL){
			return i;
		}
	}
	return -1;
}

//prints out the current room with proper format to the user
void displayCurrentRoom(struct Room maze[7], int index){
	int i;//prints out all the data regarding the room at the index provided
	printf("\nCURRENT LOCATION: %s\nPOSSIBLE CONNECTIONS: ", maze[index].name);
	for(i=0; i<maze[index].numConnects; i++){
		if(i != maze[index].numConnects-1){
			printf("%s, ", maze[index].connects[i]);
		}
		else{
			printf("%s.\n", maze[index].connects[i]);
		}
	}
	printf("WHERE TO? >");
}

//checks to see if a connection chosen is valid to the index of the current room
int isValidInput(struct Room maze[7], int currentRoom, char line[]){
	int i;
	for(i=0; i<maze[currentRoom].numConnects; i++){//indexes through connections of room at index provided, if the user input string matches a string here, it is valid
		if(strstr(line, maze[currentRoom].connects[i])){
			return 1;
		}
	}
	return 0;
}

//returns the index of the room selected by the user
int switchRooms(struct Room maze[7], char line[]){
	int i;
	for(i=0; i<7; i++){
		if(strstr(line, maze[i].name)){//looks through room array to find room with name matching the user input
			return i;//returns index of matching room
		}
	}
	return -1;
}

//adds a valid user move to the linked list keeping track of the players path
struct Node* addToRecord(struct Node* list, char roomName[]){
	struct Node* newNode;
	newNode = malloc(sizeof(struct Node));//allocates a new node in linked list
	newNode->roomPassed = malloc(strlen(roomName)+1);//makes string for new node
	strcpy(newNode->roomPassed, roomName);//copies user input to the string
	newNode->next = list;//sets pointer to next element in list

	return newNode;
}

//prints the list in reversed order
void printListReversed(struct Node* head){
	if(head == NULL){
		return;//end recursion if on the last element of the list
	}
	printListReversed(head->next);//recursive call
	printf("%s\n", head->roomPassed);//prints name of the room at this spot in the list
}

//frees the memory from the linked list
void freeListMemory(struct Node* head){
	struct Node* temp;//creates temporay node
	while(head != NULL){//while there are still nodes to move through
		temp = head;
		head = head->next;//move to next node
		free(temp);//delete previous node
	}
}

//does the actual calculation for the time
static void* timeSet(void* arg){
	FILE* file;
	time_t rawtime;//gets the time in its raw format
	struct tm* info; //info on time
	time(&rawtime);//stores time in raw time variable
	info = localtime(&rawtime);//translates to local time
	char timeBuffer[80];//holds the string that is returned with time

	strftime(timeBuffer, 80, "%l:%M%P, %A, %B %e, %G", info);//translates into a readable time format
	pthread_mutex_lock(&MUTEX);//locks the mutex thread
	file = fopen("currentTime.txt", "w+");//opens the file to put the time in
	fprintf(file, "%s\n", timeBuffer);//prints the time string to the file
	fclose(file);//closes the file with time
	pthread_mutex_unlock(&MUTEX);//unlocks the thread
}
