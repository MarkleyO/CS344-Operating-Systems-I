#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//Room struct, holds the name of a room, and the type as c strings, along with an 
struct Room{
	char* name;
	char* type;
	int id;	//each ID receives an id numnber, this might not be necessary, but it's still implemented

	int numConnects;//number of connections is not utilized
	int connects[6]; // holds the id numbers that correspond to the different rooms it connects to, -1 means there is no connection at this index in the array
};
//if a function returns only 0 or 1 this means that it is functioning as a bool, 0 = false, 1 = true
void initializeRooms(struct Room maze[7]);
int isGraphFull(struct Room maze[7]);
void addRandomConnection(struct Room maze[7]);
int getRandomRoom();
int canAddConnectionFrom(struct Room maze[7], int roomX);
int connectionAlreadyExists(struct Room maze[7], int roomX, int roomY);
void connectRoom(struct Room maze[7], int roomX, int roomY);
int isSameRoom(int roomX, int roomY);

int main(){
	srand(time(0));//seeds the random numbers
	struct Room maze[7];//holds all the room structs. always of size seven/
	
	int pid = getpid(); // gets the process id and saes to number
	char username[] = "markleyo.rooms."; //prefix for the new file directory
	char newDirectoryName[30]; //stores the new combined string
	sprintf(newDirectoryName, "%s%d", username, pid); //combines the int 
	mkdir(newDirectoryName, 0755); //makes the new directorhy with the new name

	initializeRooms(maze); //takes the maze array, and populates each one with randomized names and connections
	
	//while the graph is still not full, new connections will be added
	while(isGraphFull(maze) == 0){
		addRandomConnection(maze);
	}

	int i, j; //indexes for the two for loops
	chdir(newDirectoryName); //changes the directory to the newly created one so that new files will be put in there
	char fileName[20]; //string to store filenames in
	char* nameEnd = "_room"; //suffix of each file name
	FILE* filePointer; // declare the file that will be made
	for(i=0; i<7; i++){ //loop through seven times to create seven files
		strcat(fileName, maze[i].name); //appends the name of the maze file at index it to the string
		strcat(fileName, nameEnd); //appends the suffix to there
		filePointer = fopen(fileName, "w"); //opens a new file with this newly created name to write
		if(filePointer != NULL){ //if the file was successfully created the file will be populated with data
			fprintf(filePointer, "ROOM NAME: %s\n", maze[i].name); //enters the room name
			for(j=0; j<6; j++){ //loops through the array of connections
				if(maze[i].connects[j] != -1){
					fprintf(filePointer, "CONNECTION %d: %s\n", j+1, maze[maze[i].connects[j]].name); //if it isn't a -1, the connection will printed to the file
				}	
			}
			fprintf(filePointer, "ROOM TYPE: %s\n", maze[i].type);//prints the room type on the file
		}
		fileName[0] = '\0'; //resets the file name holder to be empty after the creation of each file
	}

	return 0;
}
//fills each room in the array with data
void initializeRooms(struct Room maze[7]){	
	int i, j;
	//combine all these loops at the end
	//initializing the names of the rooms
	char* names[10] = {"Magnolia", "Location", "Foreign", "Molly", "KOD", "Broke", "Cudi", "Choppa", "Lookin", "Fredo"};//all the names to choose from
	int tempRand; //holds index of randomly selected name
	int tempRandRange = 10; //range of indeces that can be selected
	char* tempName;
	for(i=0; i<7; i++){
		tempRand = rand()%tempRandRange; // get random number within range
		tempName = names[tempRand]; // save name to temp variable
		names[tempRand] = names[tempRandRange-1]; // move last name in list to selected place
		names[tempRandRange-1] = tempName; // move selected name to last place in list
		maze[i].name = tempName; // assign randomly selected name to a room
		tempRandRange = tempRandRange - 1; // decrement the range
	}

	//initializing the types of the rooms
	int startIndex = rand()%7;//chooses a random index to be the start
	int endIndex = rand()%7; //keeps choosing new end indexes while they match the start
	while(startIndex == endIndex){
		endIndex = rand()%7;
	}
	for(i=0; i<7; i++){ //increments through each room, based off if the index in the array matches the selected start or end indexes, a different type will be defined
		if(i == startIndex){
			maze[i].type = "START_ROOM";
		} else if (i == endIndex){
			maze[i].type = "END_ROOM";
		} else{
			maze[i].type = "MID_ROOM";
		}
		maze[i].id = i;
		for(j=0; j<6; j++){
			maze[i].connects[j] = -1; // initializes the connect arrays as -1 for each room
		}
	}
}

//checks to see if the graph is full or not, returns 1 or 0 to serve as a bool
int isGraphFull(struct Room maze[7]){
	int i, j;
	int roomConnects; //temporary counter for how many connections a room has
	for(i=0; i<7; i++){
		roomConnects = 0;//resets on each loop through
		for(j=0; j<6; j++){
			if(maze[i].connects[j] != -1){
				roomConnects = roomConnects + 1; //if a number is something other than -1 then room connects is incremented
			}
		}
		if(roomConnects < 3){ //if a single room has less than 3 connects, then the graph cannot be full
			return 0;//false
		}
	}
	
	return 1;//true
}

//main algorithm to add random connections to populate the maze
void addRandomConnection(struct Room maze[7]){
	int roomA;//integers refer to the index of the different rooms, this is simpler than passing structs around
	int roomB;
	
	while(1){//grab
		roomA = getRandomRoom();//grabs a random number, and continues to generate numbers until it has one that can take a new connection
		if(canAddConnectionFrom(maze, roomA) == 1)
			break;
	}
	do{
		roomB = getRandomRoom(); //grabs another random number, and keeps grabbing until it has one that is distinct from a, can take a connecion from a, and doesn't already have a connection with a
	} while(canAddConnectionFrom(maze, roomB) == 0 || isSameRoom(roomA, roomB) == 1 || connectionAlreadyExists(maze, roomA, roomB) == 1);

	connectRoom(maze, roomA, roomB);//at this point the validity has been checked and the connection is established between the rooms
}

//used to get a random room, just returns a random integer representing an index/id
int getRandomRoom(){ //just returns the id
	return rand()%7;
}

//checks to see if a connection can be added from a certain room represetned by an integer
int canAddConnectionFrom(struct Room maze[7], int roomB){
	int i;
	int connectCounter = 0;
	for(i=0; i<6; i++){/
		if(maze[roomB].connects[i] != -1){//counts the amount of non -1 numbers
			connectCounter = connectCounter + 1;
		}
	}
	//the only condition of concern here, is whether or not the array to hold connections is full yet or not.
	if(connectCounter < 6){
		return 1; // true
	}
	return 0;//false
}

//checks to see if a connection already exists between rooms returns an int functioning as a bool
int connectionAlreadyExists(struct Room maze[7], int roomX, int roomY){
	int i, j;
	int xHasY = 0; //these two integers serve as booleans in this function
	int yHasX = 0;
	for(i=0; i<6; i++){
		if(maze[roomX].connects[i] == roomY) //checks to see if the room x has the id of room y in its connection list
			xHasY = 1;
		if(maze[roomY].connects[i] == roomX)//vice versa from above
			yHasX = 1;
		if(xHasY == 1 && yHasX == 1)//if both posess each other, than a connection already exists
			return 1;
	}

	return 0;//false
}

//serves to actually connect the rooms together.
void connectRoom(struct Room maze[7], int roomX, int roomY){
	int i;
	for(i=0; i<6; i++){//continues indexing through the connection list of room y, the first -1 found is replaced by the id of the room to connect
		if(maze[roomX].connects[i] == -1){
			maze[roomX].connects[i] = roomY;
			break;
		}
	}
	for(i=0; i<6; i++){
		if(maze[roomY].connects[i] == -1){//same as loop above, but connects in opposing direction
			maze[roomY].connects[i] = roomX;
			break;
		}
	}
}

//checks to see if two rooms are the same, works as bool
int isSameRoom(int roomX, int roomY){
	if(roomX == roomY){//since I use id numbers, if the numbers are the same, the rooms are the same
		return 1;//true
	}
	return 0;//true
}