#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg){ perror(msg); exit(0); }
int readInFiles(char *plaintextTitle, char *keyTitle, FILE *plaintext, FILE *key); //returns size of plaintext
void receiveEncrypted(int estConnect);

int main(int argc, char *argv[]){
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////
	int socketFD, portNumber, charsWritten, charsRead, ptSize;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256], *ptBuffer, *keyBuffer, *encBuffer;
	size_t ptLineBuffSize = 0, keyLineBuffSize = 0;
	FILE *plaintextFP, *keyFP;
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////

	//////////////////////////////////SETTING UP SERVER///////////////////////////////////////////////////////////////////////////////
	if(argc < 4){fprintf(stderr, "USAGE %s plaintext key port\n", argv[0]); exit(0);}//checks to see if proper num of args used

	ptSize = readInFiles(argv[1], argv[2], plaintextFP, keyFP);//checks to see if files are valid

	memset((char*)&serverAddress, '\0', sizeof(serverAddress));//sets memory to null
	portNumber = atoi(argv[3]);//gets port number from args, converts to int
	serverAddress.sin_family = AF_INET;//allows for different connections
	serverAddress.sin_port = htons(portNumber);//stores port number with server address struct
	serverHostInfo = gethostbyname("localhost");//connects to host on localhost
	if(serverHostInfo == NULL){ fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }//if host is unavilable error message sent
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);//copies memory info

	socketFD = socket(AF_INET, SOCK_STREAM, 0);//makes a socket to connect to
	if(socketFD < 0) error("CLIENT: ERROR opening socket");//error sent if there is something wrong when opening the socket

	if(connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)//connects to newly made socket
		printf("CLIENT: ERROR connecting");
	//////////////////////////////////SETTING UP SERVER///////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////HANDSHAKE///////////////////////////////////////////////////////////////////////////////////////
	memset(buffer, '\0', sizeof(buffer));//buffer is cleared
	buffer[0] = '0';//a zero value is sent for handshake on enc
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);//handshake is sent to server
	if(charsWritten < 0) error("CLIENT: ERROR writing to socket");//error is socket cannot be written to
	if(charsWritten < strlen(buffer) ) printf("CLIENT: WARNING: Not all data written to socket!\n");//error if not everything sent
	//////////////////////////////////HANDSHAKE///////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////READING IN FROM FILES///////////////////////////////////////////////////////////////////////////
	plaintextFP = fopen(argv[1], "r");//opens file for plaintext
	getline(&ptBuffer, &ptLineBuffSize, plaintextFP);//reads line from file into local string
	fclose(plaintextFP);//closes file
	keyFP = fopen(argv[2], "r");//opens key file
	getline(&keyBuffer, &keyLineBuffSize, keyFP);//reads line from file into local string
	fclose(keyFP);//closes file
	//////////////////////////////////READING IN FROM FILES///////////////////////////////////////////////////////////////////////////

	//////////////////////////////////SEND FILES TO SERVER////////////////////////////////////////////////////////////////////////////
	memset(buffer, '\0',  sizeof(buffer));//clears buffer
	recv(socketFD, buffer, sizeof(buffer)-1, 0);//receives request for strings from files
	send(socketFD, ptBuffer, strlen(ptBuffer), 0);//sends plaintext
	memset(buffer, '\0',  sizeof(buffer));//clears buffer
	recv(socketFD, buffer, sizeof(buffer)-1, 0);//receives confirmation from server
	send(socketFD, keyBuffer, strlen(keyBuffer), 0);//sends key to server
	memset(buffer, '\0',  sizeof(buffer));//clears buffer
	recv(socketFD, buffer, sizeof(buffer)-1, 0);//receives confirmation from server
	//////////////////////////////////SEND FILES TO SERVER////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////RECEIVE ENCRYPTED FILE//////////////////////////////////////////////////////////////////////////
	send(socketFD, "I WANNA STRING", 14, 0);//sends request for encrypted data to server
	receiveEncrypted(socketFD);//receives the encrypted string
	//////////////////////////////////RECEIVE ENCRYPTED FILE//////////////////////////////////////////////////////////////////////////

	close(socketFD);

	return 0;
}

//checks to see if files are valid to use
int readInFiles(char* plaintextTitle, char* keyTitle, FILE* plaintext, FILE* key){
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////
	char* fileReadingBuffer = NULL;//holds a string temporarily
	size_t lineBuffSize = 0; //stores size of line buffer
	int ptSize, keySize, i;//size of both files being read
	//////////////////////////////////DECLARING / INITIALIZING VARIABLES//////////////////////////////////////////////////////////////

	//////////////////////////////////SEARCH FOR INVALID CHARACTERS///////////////////////////////////////////////////////////////////
	plaintext = fopen(plaintextTitle, "r");//opens the plaintext file to read
	key = fopen(keyTitle, "r");//opens key to read
	getline(&fileReadingBuffer, &lineBuffSize, plaintext);//stores plaintext in string locally
	ptSize = strlen(fileReadingBuffer);//calculates size of string
	for(i=0; i<ptSize-1; i++){//loops through string
		if((fileReadingBuffer[i]!=' ') && (fileReadingBuffer[i]>'Z' || fileReadingBuffer[i]<'A')){//check to see if in range or not space
			error("ERROR: Invalid character is present in plaintext");//sends error message on invalid character
			fclose(plaintext);//files are closed and program exited
			fclose(key);
			exit(1);
		}
	}
	//////////////////////////////////SEARCH FOR INVALID CHARACTERS///////////////////////////////////////////////////////////////////

	//////////////////////////////////COMPARE STRING SIZES////////////////////////////////////////////////////////////////////////////
	getline(&fileReadingBuffer, &lineBuffSize, key);//reads in key string
	keySize = strlen(fileReadingBuffer);//calculate size of key
	if(keySize < ptSize){//if key is shorter than plaintext then program fails
		error("ERROR: Key is not at least as long as plaintext");
		fclose(plaintext);
		fclose(key);
		exit(1);
	}
	//////////////////////////////////COMPARE STRING SIZES////////////////////////////////////////////////////////////////////////////

	fclose(plaintext);
	fclose(key);
	return ptSize;
}

//receives the encrypted file from the server
void receiveEncrypted(int estConnect){
	char buffer[257];
	
	//////////////////////////////////RECEIVING ENCRYPTED STRING//////////////////////////////////////////////////////////////////////
	do{
		memset(buffer, '\0', 257);//clear buffer to read
		recv(estConnect, buffer, 256, 0);//receive a packet
		printf(buffer);//print out received packet
	}while(strstr(buffer, "\n") == NULL);//if newline character is found end
	//////////////////////////////////RECEIVING ENCRYPTED STRING//////////////////////////////////////////////////////////////////////
}
