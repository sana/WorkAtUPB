//Dascalu Laurentiu, 342C3
//Tema 2 SPRC - Sortare toleranta la defecte in MPI
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DEFAULT_TEST_SIZE    1000
#define DEFAULT_TEST_NAME    "input.dat"

/**
 * @brief Programul genereaza un fisier de test avand dimensiunea
 * data ca parametru in linia de comanda.
 * @param Numarul de int-uri generate
 * @param Numele fisierului de iesire
 */
int main(int argc, char *argv[1])
{
	int test_size = DEFAULT_TEST_SIZE, i, key;
	FILE *fout = NULL;

	if (argc == 2)
	{
		test_size = atoi(argv[1]);
		fout = fopen(DEFAULT_TEST_NAME, "wb");
	}
	else if (argc == 3)
	{
		test_size = atoi(argv[1]);
		fout = fopen(argv[2], "wb");
	}

	assert(fout != NULL);

	for (i = 0; i < test_size; i++)
	{
		key = rand() % 10000;
		fwrite(&key, sizeof(int), 1, fout);
	}

	assert(fclose(fout) == 0);
	return EXIT_SUCCESS;
}
