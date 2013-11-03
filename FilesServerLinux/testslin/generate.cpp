//genereaza date de testare
//argumente: numarul de bytes care trebuie generati
//outputul e scris si la stdout si la stderr

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_LENGTH		(64 * 1024)
#define min(a, b)		((a) < (b) ? (a) : (b))

int main(int argc, char* argv[]){
	int noBytes;
	if (argc != 2)
		return 1;

	sscanf(argv[1], "%i", &noBytes);
	char buffer[BUFFER_LENGTH];

	srand(time(NULL) + noBytes);
	while (noBytes > 0){
		int no = min(BUFFER_LENGTH, noBytes);

		for (int i = 0; i < no; i++)
			buffer[i] = rand() % 256;
		fwrite(buffer, no, 1, stdout);
		fwrite(buffer, no, 1, stderr);
		noBytes -= no;
	}
	
	return 0;
}
