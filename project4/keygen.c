#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]){
	srand(time(NULL));

	int i;
	char* validCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int numCharacters;

	char* key;
	int tempRand;
	FILE *fp;

	if(argc == 1){
		perror("Invalid Arguments\n");
		exit(1);
	}
	else{
		numCharacters = atoi(argv[1]);
		//printf("%d\n", numCharacters);
		key = (char*)malloc(numCharacters * sizeof(char));
		memset(key, '\0', numCharacters*sizeof(char));
		//printf("%s\n", key);

		for(i=0; i<numCharacters; i++){
			tempRand = rand()%27;
			key[i] = validCharacters[tempRand];
		}

		key[numCharacters] = '\n';
		printf("%s", key);

		// fp = fopen("key", "w+");
		// fputs(key, fp);
		// fclose(fp);
	}

	return 0;
}
