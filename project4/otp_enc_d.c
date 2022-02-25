#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); }
void receiveFile(int estConnect, char** outputTo);
char* encodeString(char* ptString, char* keyString);

int main(int argc, char *argv[]){
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, i, execStatus, childExitMethod = -5;
	socklen_t sizeOfClientInfo;
	char buffer[256], *temp, *ptString, *keyString;
	struct sockaddr_in serverAddress, clientAddress;
	pid_t spawnPID = -5;
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////

	//////////////////////////////////SETTING UP SERVER////////////////////////////////////////////////////////////////
	if(argc < 2){ fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(1); }//ensures proper number of variables passed

	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); //clear server address before use
	portNumber = atoi(argv[1]); //store CMD ARG as an int var
	serverAddress.sin_family = AF_INET; //allow for general purpose sockets
	serverAddress.sin_port = htons(portNumber); //store port to connect to
	serverAddress.sin_addr.s_addr = INADDR_ANY; //allows connections from any IP address

	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); //create a new general purpose TCP socket
	if(listenSocketFD < 0) error("ERROR opening socket");

	if(bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) //associates the socket with the chosen port
		error("ERROR on binding");
	listen(listenSocketFD, 5); //allows for 5 connections to queue up
	//////////////////////////////////SETTING UP SERVER////////////////////////////////////////////////////////////////

	//////////////////////////////////SERVER LOOP//////////////////////////////////////////////////////////////////////
	while(1){

		//////////////////////////////////ACCEPTING A CLIENT//////////////////////////////////////////////////////////////////////
		sizeOfClientInfo = sizeof(clientAddress);//stores size of the client address
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);//connects to a client socket in queue
		if(establishedConnectionFD < 0) error("ERROR on accept");//throws error if conenction fails
		//////////////////////////////////ACCEPTING A CLIENT//////////////////////////////////////////////////////////////////////

		spawnPID = fork();//forking to create a child
		if(spawnPID == 0){//contains the code for the child process

			//////////////////////////////////REVEIVE HANDSHAKE////////////////////////////////////////////////////////////////
			memset(buffer, '\0', 256); //buffer must be kept clean throughout to ensure only wanted data is read
			charsRead = recv(establishedConnectionFD, buffer, 255, 0);//receives packet of buffer size, we know handshake is small enough
			if(charsRead < 0) error("ERROR reding from socket");//throws error on fail
			send(establishedConnectionFD, "VALIDATED", 9, 0);//sends a text message to the client, verifying the message was got
			//////////////////////////////////REVEIVE HANDSHAKE////////////////////////////////////////////////////////////////

			//////////////////////////////////RECEIVE FILES / SEND ENCODED FILE////////////////////////////////////////////////////////////////
			if(atoi(buffer) == 0){//if the handshake is the proper value then program continues
				
				//////////////////////////////////RECEIVE CLIENT FILES/////////////////////////////////////////////////////////////
				receiveFile(establishedConnectionFD, &ptString);//receives the plaintet from client
				receiveFile(establishedConnectionFD, &keyString);//receives key from client
				//////////////////////////////////RECEIVE CLIENT FILES/////////////////////////////////////////////////////////////

				//////////////////////////////////ENCODING/////////////////////////////////////////////////////////////////////////
				temp = encodeString(ptString, keyString);//stores new encoded string 
				memset(buffer, '\0', 256); //buffer is reset and cleared
				recv(establishedConnectionFD, buffer, 256, 0);//client asks for the string
				send(establishedConnectionFD, temp, strlen(temp), 0);//encoded string is sent to client
				//////////////////////////////////ENCODING/////////////////////////////////////////////////////////////////////////	

				//////////////////////////////////CLEANUP/////////////////////////////////////////////////////////////////////////
				free(temp);
				free(ptString);
				free(keyString);
				exit(0);
				//////////////////////////////////CLEANUP/////////////////////////////////////////////////////////////////////////

			}else{error("ERROR: WRONG CLIENT CONNECTED");}//error message sent if wrong client is used
			//////////////////////////////////RECEIVE FILES / SEND ENCODED FILE////////////////////////////////////////////////////////////////
			
			exit(0);
		}
		else{//parent is here
		 	wait();//waits for a child to finish
		 	close(establishedConnectionFD);//closes the connection that was used to send/rec data
		}
	}
	//////////////////////////////////SERVER LOOP//////////////////////////////////////////////////////////////////////

	close(listenSocketFD);//cleaning up and closing the socket that listens for clients

	return 0;
}

//Receives plaintext and key from the client
void receiveFile(int estConnect, char** outputTo){
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////
	int i = 1;//index variable
	char buffer[257];//where receive calls are stored
	char* adjustableBuffer = (char*)malloc(((256*i)+1)*sizeof(char));//adjusts size to accomodate incoming string
	char* adjustableBufferTemp = (char*)malloc(((256*i)+1)*sizeof(char));//used to store above string when resizing for new packet
	memset(adjustableBuffer, '\0', sizeof(adjustableBuffer));//set memory to null
	memset(adjustableBufferTemp, '\0', sizeof(adjustableBufferTemp));//set memory to null
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////

	//////////////////////////////////RECEIVING AND STORING PACKETS///////////////////////////////////////////////////////////////////
	do{
		memset(buffer, '\0', 257);//buffer must be cleared to receive a new packet
		recv(estConnect, buffer, 256, 0);//new packet is received
		strcpy(adjustableBufferTemp, adjustableBuffer);//contents of adj copied to temp
		free(adjustableBuffer);//adj is free'd to be resized
		adjustableBuffer = (char*)malloc(((256*i)+1)*sizeof(char));//adj is resized to accomodate new packet
		memset(adjustableBuffer, '\0', sizeof(adjustableBuffer));//memory of adj set to null
		strcpy(adjustableBuffer, adjustableBufferTemp);//temp is copied back into adj
		strcat(adjustableBuffer, buffer);//packet is cat'd to newly resized adj
		free(adjustableBufferTemp);//temp is freed
		adjustableBufferTemp = (char*)malloc(((256*i)+1)*sizeof(char));//temp is resized to match adj
		memset(adjustableBufferTemp, '\0', sizeof(adjustableBufferTemp));//memory is set
		i++;//index incremented
	}while(strstr(adjustableBuffer, "\n") == NULL);
	//////////////////////////////////RECEIVING AND STORING PACKETS///////////////////////////////////////////////////////////////////

	//////////////////////////////////STORE NEWLY FORMED STRING//////////////////////////////////////////////////////////////////////
	*outputTo = (char*)malloc(strlen(adjustableBuffer)*sizeof(char));
	memset(*outputTo, '\0', strlen(*outputTo));
	strcpy(*outputTo, adjustableBuffer);
	//////////////////////////////////STORE NEWLY FORMED STRING//////////////////////////////////////////////////////////////////////

	free(adjustableBuffer);//free memory used for adj
	free(adjustableBufferTemp);//free memory used for temp

	send(estConnect, "VALIDATED", 9, 0);//validating message is used to ensure packets aren't combined
}

//Creates a new encoded string that is returned
char* encodeString(char* ptString, char* keyString){
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////
	int i, char1, char2;//increments and stores a chars numeric value
	int wew;//fully calculated numerical value
	char* final = (char*)calloc(strlen(ptString), sizeof(char));//encoded string stored here
	char* validCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";//array of characters that are allowed to be used
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////
	
	//////////////////////////////////ENCODING STRING////////////////////////////////////////////////////////////////////////////////
	for(i=0; i<strlen(ptString); i++){//loops for size of plaintext string
		if(ptString[i] == ' ')//spaces have value manually set
			char1 = 26;
		else
			char1 = ptString[i] - 65;//65 is subtracted, so that letter can be grabbed from char array above
		if(keyString[i] == ' ')
			char2 = 26;
		else
			char2 = keyString[i] - 65;
		wew = (char1 + char2)%27;//default mod function did not work for negative numbers
		final[i] = validCharacters[wew];//puts proper character in encoded string
	} 
	final[strlen(final) -1] = '\n';//ensures that newline character is last character
	//////////////////////////////////ENCODING STRING////////////////////////////////////////////////////////////////////////////////

	return final;
}