#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libspe2.h>
#include <pthread.h>
#include <assert.h>
#include "ppm.hpp"
#include "quilt.hpp"

extern spe_program_handle_t spu;
int spu_threads;
static void filter_image(Picture *pout);

#define MAX_SPU_THREADS   8

typedef struct
{
	// contextul de executie
	spe_context_ptr_t spe;

	// event-uri
	spe_event_unit_t pevents;

	// identificatorul thread-ului
	int id;

	// dimensiunea patch-ului/fasiei
	int patch_size;
} thread_arg_t;

// Contexte SPE
spe_context_ptr_t spes[MAX_SPU_THREADS];

// Thread-uri + argumente
pthread_t threads[MAX_SPU_THREADS];
thread_arg_t arg[MAX_SPU_THREADS];

// Event-uri
spe_event_unit_t pevents[MAX_SPU_THREADS], event_received;
spe_event_handler_ptr_t event_handler;

void *ppu_pthread_function(void *arg)
{
	thread_arg_t *targ = (thread_arg_t *) arg;
	unsigned int entry = SPE_DEFAULT_ENTRY;
	spe_stop_info_t stop_info;

	if (spe_context_run(targ->spe, &entry, 0, (int *) targ->patch_size,
			(int *) targ->id, &stop_info) < 0)
	{
		perror("Failed running context");
		exit(1);
	}

	pthread_exit(NULL);
}

void compute_parameters(Picture *&pin, Picture *&pout, char *fin, char *fout,
		int &in_height, int &in_width, int nbucati_height, int nbucati_width,
		double bheight, double bwidth, int zoom, int &dim_bucata_height,
		int &dim_bucata_width, int &dim_fasie_height, int &dim_fasie_width)
{
	// Deschid fisierul de intrare
	pin = new Picture();
	pin->load(fin);

	in_height = pin->Height();
	in_width = pin->Width();

	int out_height = in_height * zoom;
	int out_width = in_width * zoom;

	// Width
	dim_bucata_width = (out_width - in_width) / nbucati_width;
	out_width = dim_bucata_width * nbucati_width + in_width;

	// Height
	dim_bucata_height = (out_height - in_height) / nbucati_height;
	out_height = dim_bucata_height * nbucati_height + in_height;

#ifdef DEBUG
	printf("Dim patch width, Dim patch height %d %d\n",
			dim_bucata_width, dim_bucata_height);

	printf("Out width => %d\n", out_width);
	printf("%d %d\n", 0, in_width);
	for (int i = 0; i < nbucati_width; i++)
	{
		printf("%d=>%d\n", in_width + i * dim_bucata_width, in_width + (i + 1)
				* dim_bucata_width);
	}

	printf("Out height => %d\n", out_height);
	printf("%d %d\n", 0, in_height);
	for (int i = 0; i < nbucati_height; i++)
	{
		printf("%d=>%d\n", in_height + i * dim_bucata_height, in_height + (i
						+ 1) * dim_bucata_height);
	}
#endif

	dim_fasie_height = (int) (bheight * dim_bucata_height);
	dim_fasie_width = (int) (bwidth * dim_bucata_width);

#ifdef DEBUG
	printf("Blocuri height %d, width %d\n", nbucati_height, nbucati_width);
	printf("Bloc height %d, width %d\n", dim_bucata_height, dim_bucata_width);
	printf("Fasie height %d, width %d\n", dim_fasie_height, dim_fasie_width);
#endif

	pout = new Picture(out_height, out_width, pin->Color());
}

/*
 * @usage:
 *
 * ./program
 * 		fis_in - fisier de intrare
 * 		fis_out - fisier de iesire
 * 		zoom - de cate ori imaginea de iesire va fi mai mare decat imagine de intrare
 * 		nr_bucati_dim1 - numarul de petice pe una din dimensiuni
 * 		nr_bucati_dim2 - numarul de bucati pe cealalta dimensiune
 * 		banda de suprapunere_dim1 - fractiunile din dimensiunile pe dim1 si dim2 ale peticelor
 * 		banda de suprapunere_dim2
 * 		nr_candidati
 */
int main(int argc, char **argv)
{
	assert(argc == 9);

	char *fin = strdup(argv[1]);
	char *fout = strdup(argv[2]);
	int zoom = atoi(argv[3]);
	int nbucati_height = atoi(argv[4]);
	int nbucati_width = atoi(argv[5]);
	double bheight = atof(argv[6]);
	double bwidth = atof(argv[7]);
	int ncandidati = atoi(argv[8]);
	Picture *pin, *pout;

	int in_height, in_width;
	int dim_bucata_height, dim_bucata_width;
	int dim_fasie_height, dim_fasie_width;

	// Parsez fisierul de intrare si ajustez parametrii in functie de datele de input
	compute_parameters(pin, pout, fin, fout, in_height, in_width,
			nbucati_height, nbucati_width, bheight, bwidth, zoom,
			dim_bucata_height, dim_bucata_width, dim_fasie_height,
			dim_fasie_width);

	/* creez handler-ul de evenimente */
	event_handler = spe_event_handler_create();

	/*
	 * Determine the number of SPE threads to create.
	 */
	spu_threads = spe_cpu_info_get(SPE_COUNT_USABLE_SPES, -1);

	if (spu_threads > MAX_SPU_THREADS)
		spu_threads = MAX_SPU_THREADS;

	/*
	 * Create several SPE-threads to execute 'spu'.
	 */
	for (int i = 0; i < spu_threads; i++)
	{
		/* Create context */
		if ((spes[i] = spe_context_create(SPE_EVENTS_ENABLE, NULL)) == NULL)
		{
			perror("Failed creating context");
			exit(1);
		}

		/* Load program into context */
		if (spe_program_load(spes[i], &spu))
		{
			perror("Failed loading program");
			exit(1);
		}

		pevents[i].events = SPE_EVENT_OUT_INTR_MBOX;
		pevents[i].spe = spes[i];
		spe_event_handler_register(event_handler, &pevents[i]);

		arg[i].spe = spes[i];
		arg[i].id = i;
		arg[i].pevents = pevents[i];

		/* Create thread for each SPE context */
		if (pthread_create(&threads[i], NULL, &ppu_pthread_function, &arg[i]))
		{
			perror("Failed creating thread");
			exit(1);
		}
	}

	for (int i = 0; i < in_height; i++)
		for (int j = 0; j < in_width; j++)
			pout->set(i, j, pin->get(i, j));

	/*
	 * Image quilting-ul este format din quilting pe orizontala
	 * si pe verticala
	 */

	ncandidati *= 50;

	quilt_vertical(pout, dim_bucata_width, dim_bucata_height, nbucati_width,
			nbucati_height, dim_fasie_width, dim_fasie_height, nbucati_height,
			ncandidati, in_height);

	quilt_orizontal(pout, dim_bucata_height, dim_bucata_width, nbucati_height,
			nbucati_width, dim_fasie_height, dim_fasie_width, nbucati_width,
			ncandidati, in_width);

	// La sfarsit se aplica un filtru pentru uniformizarea imaginii
	filter_image(pout);

	pout->save(fout);

	unsigned int val[1] __attribute__((aligned(128)));
	// Trimit mesaj de QUIT
	val[0] = 0;

	for (int i = 0; i < spu_threads; i++)
	{
		if (spe_in_mbox_write(spes[i], val, 1, SPE_MBOX_ANY_NONBLOCKING) < 0)
		{
			perror("ERROR, writing messages to spe failed\n");
		}
	}

	/* Wait for SPU-thread to complete execution.  */
	for (int i = 0; i < spu_threads; i++)
	{
		if (pthread_join(threads[i], NULL))
		{
			perror("Failed pthread_join");
			exit(1);
		}

		spe_event_handler_deregister(event_handler, &pevents[i]);

		/* Destroy context */
		if (spe_context_destroy(spes[i]) != 0)
		{
			perror("Failed destroying context");
			exit(1);
		}
	}

	spe_event_handler_destroy(event_handler);

	printf("\nThe program has successfully executed.\n");

	return 0;
}

static void apply_filter(pixel *p[9])
{
	int red = 0, green = 0, blue = 0;

	for (int i = 0; i < 9; i++)
	{
		red += p[i]->red;
		green += p[i]->green;
		blue += p[i]->blue;
	}

	p[5]->red = (red + 3 * p[5]->red) / 12;
	p[5]->green = (green + 3 * p[5]->green) / 12;
	p[5]->blue = (blue + 3 * p[5]->blue) / 12;
}

static void filter_image(Picture *pout)
{
	int width = pout->Width();
	int height = pout->Height();
	pixel *pixels[9];

	for (int i = 3; i < height - 3; i++)
	{

		for (int j = 3; j < width - 3; j++)
		{
			for (int ii = 0; ii <= 2; ii++)
				for (int jj = 0; jj <= 2; jj++)
					pixels[ii * 3 + jj] = pout->getp(i - ii - 1, j - jj - 1);

			apply_filter(pixels);
		}
	}

}
