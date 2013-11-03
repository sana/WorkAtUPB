#include "quilt.hpp"
#include <libspe2.h>
#include <malloc.h>

//#define CELL

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#ifndef MAX
#define MAX(A, B) ((A) < (B) ? (B) : (A))
#endif

#ifndef MAX_SPU_THREADS
#define MAX_SPU_THREADS   8
#endif

static volatile int SPE_WORK = 0;
extern int spu_threads;
extern spe_context_ptr_t spes[MAX_SPU_THREADS];
extern spe_event_unit_t pevents[MAX_SPU_THREADS], event_received;
unsigned int adr_value[1] __attribute__((aligned(128)));

static void __min(int **E, int x, int y, int &min, int &min_id)
{
	min = E[x][0];
	min_id = 0;

	for (int j = 1; j < y; j++)
	{
		if (min > E[x][j])
		{
			min = E[x][j];
			min_id = j;
		}
	}
}

static void min_line(int **E, int line, int width, int &min, int &min_id)
{
	__min(E, line, width, min, min_id);
}

void min_column(int **E, int column, int height, int &min, int &min_id)
{
	__min(E, column, height, min, min_id);
}

static int orizontal_dif(Picture *pout, int off_candidat, int off_add,
		int dim_fasie_width, int dim_bucata_width, int height);

static void orizontal_netezire(Picture *pout, int id, int in_width, int height,
		int dim_fasie_width, int dim_bucata_width, int off_candidat_min);

static void orizontal_merge(Picture *pout, int height, int dim_fasie_width,
		int in_width, int id, int dim_bucata_width, int off_candidat_min);

void quilt_orizontal(Picture *pout, int dim_bucata_height,
		int dim_bucata_width, int nbucati_height, int nbucati_width,
		int dim_fasie_height, int dim_fasie_width, int npatches,
		int ncandidati, int in_width)
{
	int height = pout->Height();

	int off_candidat = -1, off_candidat_min = -1;
	int min_dif = -1, curr_dif = -1;

#ifndef CELL
	for (int i = 0; i < npatches; i++)
	{
		min_dif = 1 << 30;
		// Pentru fiecare petic nou adaugat, exista ncandidati petice
		// ce candideaza pentru ocuparea pozitiei
		for (int j = 0; j < ncandidati; j++)
		{
			off_candidat = rand() % (in_width - dim_fasie_width);

			curr_dif = orizontal_dif(pout, off_candidat, in_width + (i - 1)
					* dim_bucata_width, dim_fasie_width, dim_bucata_width,
					height);

			if (min_dif> curr_dif)
			{
				min_dif = curr_dif;
				off_candidat_min = off_candidat;
			}
		}

		orizontal_netezire(pout, i, in_width, height, dim_fasie_width,
				dim_bucata_width, off_candidat_min);
	}
#else
	for (int i = 0; i < npatches; i++)
	{
		min_dif = 1 << 30;
		// Pentru fiecare petic nou adaugat, exista ncandidati petice
		// ce candideaza pentru ocuparea pozitiei
		for (int j = 0; j < ncandidati; j++)
		{
			off_candidat = rand() % (in_width - dim_fasie_width);

			curr_dif = orizontal_dif(pout, off_candidat, in_width + (i - 1)
					* dim_bucata_width, dim_fasie_width, dim_bucata_width,
					height);
		}

		// Astept de la toate raspunsul
		for (int i = 0; i < spu_threads; i++)
		{
			adr_value[0] = (unsigned int) 2;
			if (spe_in_mbox_write(spes[i], adr_value, 1,
					SPE_MBOX_ANY_NONBLOCKING) < 0)
			{
				perror("ERROR, writing messages to spe failed\n");
			}
		}

		unsigned int buffer_candidat_off, buffer_candidat_val,
				off_candidat_min = -1, val_candidat_min = 1 << 30;
		for (i = 0; i < NO_SPU; ++i)
		{
			nevents = spe_event_wait(event_handler, &event_received, 1, -1);
			if (event_received.events & SPE_EVENT_OUT_INTR_MBOX)
			{
				while (spe_out_intr_mbox_status(event_received.spe) < 1)
					;
				spe_out_intr_mbox_read(event_received.spe,
						&buffer_candidat_val, 1, SPE_MBOX_ANY_BLOCKING);

				while (spe_out_intr_mbox_status(event_received.spe) < 1)
					;
				spe_out_intr_mbox_read(event_received.spe,
						&buffer_candidat_off, 1, SPE_MBOX_ANY_BLOCKING);
			}

			if (buffer_candidat_val < val_candidat_min)
			{
				val_candidat_min = buffer_candidat_val;
				off_candidat_min = buffer_candidat_off;
			}

			orizontal_netezire(pout, i, in_width, height, dim_fasie_width,
					dim_bucata_width, off_candidat_min);
		}
	}
#endif
}

static int orizontal_dif(Picture *pout, int off_candidat, int off_add,
		int dim_fasie_width, int dim_bucata_width, int height)
{
	int diff = 0;
#ifndef CELL
	for (int i = 0; i < height; i++)
	{
		/*
		 * Pixelii de pe fasie se proiecteaza in diferenta.
		 */
		for (int j = 0; j < dim_fasie_width; j++)
		diff += pixel::compare(pout->get(i, off_candidat + j
						+ dim_bucata_width), pout->get(i, off_add - dim_fasie_width
						+ j));
	}
#else
	// Trimit un pachet prin care anunt ca vreau sa fac diff
	// intre doua fasii
	adr_value[0] = 1;
	if (spe_in_mbox_write(spes[SPE_WORK], adr_value, 1,
			SPE_MBOX_ANY_NONBLOCKING) < 0)
	{
		perror("ERROR, writing messages to spe failed\n");
	}

	pixel *packet1 = (pixel *) memalign(128, height * dim_fasie_width
			* sizeof(pixel));
	pixel *packet2 = (pixel *) memalign(128, height * dim_fasie_width
			* sizeof(pixel));

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < dim_fasie_width; j++)
		{
			packet1[i * dim_fasie_width + j] = pout->get(i, off_candidat + j
					+ dim_bucata_width);
			packet2[i * dim_fasie_width + j] = pout->get(i, off_add
					- dim_fasie_width + j);
		}
	}

	adr_value[0] = (unsigned int) height;
	if (spe_in_mbox_write(spes[SPE_WORK], adr_value, 1,
			SPE_MBOX_ANY_NONBLOCKING) < 0)
	{
		perror("ERROR, writing messages to spe failed\n");
	}

	adr_value[0] = (unsigned int) dim_fasie_width;
	if (spe_in_mbox_write(spes[SPE_WORK], adr_value, 1,
			SPE_MBOX_ANY_NONBLOCKING) < 0)
	{
		perror("ERROR, writing messages to spe failed\n");
	}

	adr_value[0] = (unsigned int) packet1;
	if (spe_in_mbox_write(spes[SPE_WORK], adr_value, 1,
			SPE_MBOX_ANY_NONBLOCKING) < 0)
	{
		perror("ERROR, writing messages to spe failed\n");
	}

	adr_value[0] = (unsigned int) packet2;
	if (spe_in_mbox_write(spes[SPE_WORK], adr_value, 1,
			SPE_MBOX_ANY_NONBLOCKING) < 0)
	{
		perror("ERROR, writing messages to spe failed\n");
	}

	SPE_WORK = (SPE_WORK + 1) % spu_threads;

	free(packet1);
	free(packet2);

#endif
	return diff;
}

static void orizontal_netezire(Picture *pout, int id, int in_width, int height,
		int dim_fasie_width, int dim_bucata_width, int off_candidat_min)
{
	for (int j = 0; j < dim_bucata_width; j++)
	{
		for (int i = 0; i < height; i++)
		{
#ifdef DEBUG
			pout->set(i,
					j + in_width + id * dim_bucata_width + dim_fasie_width,
					pixel(0, 255, 0));
#else
			pout->set(i, j + in_width + id * dim_bucata_width, pout->get(i, j
					+ off_candidat_min));
#endif
		}
	}

	orizontal_merge(pout, height, dim_fasie_width, in_width, id,
			dim_bucata_width, off_candidat_min);
}

static void orizontal_merge(Picture *pout, int height, int dim_fasie_width,
		int in_width, int id, int dim_bucata_width, int off_candidat_min)
{

#ifdef DEBUG
	for (int j = 0; j < dim_fasie_width; j++)
	{
		for (int i = 0; i < height; i++)
		{
			pout->set(i, j + in_width + id * dim_bucata_width, pixel(255, 0, 0));
		}
	}
	return;
#endif
	/*
	 * Cele doua fasii sunt descrise de:
	 * 		in_width + id * dim_bucata_width - dim_fasie_width
	 * 		off_candidat_min + dim_bucata_width - dim_fasie_width
	 * Calculez eroarea minima de suprapunere intre cele doua fasii dupa urmatoarea
	 * dinamica: E(i, j) = e(i, j) + min{E(i - 1, j - 1), E(i - 1, j), E(i - 1, j + 1)}
	 */
	int **e, **E;
	int candidat1 = in_width + id * dim_bucata_width - dim_bucata_width;
	int candidat2 = off_candidat_min;

	e = (int **) malloc(dim_fasie_width * sizeof(int *));
	E = (int **) malloc(dim_fasie_width * sizeof(int *));
	for (int i = 0; i < dim_fasie_width; i++)
	{
		e[i] = (int *) malloc(height * sizeof(int));
		E[i] = (int *) malloc(height * sizeof(int));
	}

	for (int i = 0; i < dim_fasie_width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			e[i][j] = pixel::compare(pout->get(i, j + candidat1), pout->get(i,
					j + candidat2));
		}
	}

	for (int j = 0; j < height; j++)
		E[0][j] = e[0][j];

	for (int i = 1; i < dim_fasie_width; i++)
	{

		for (int j = 0; j < height; j++)
		{
			if (j == 0)
				E[i][0] = MIN(E[i - 1][0], E[i - 1][1]);
			else if (j == height - 1)
				E[i][height - 1] = MIN(E[i - 1][height - 2], E[i - 1][height
						- 1]);
			else
				E[i][j] = MIN(E[i-1][j-1], MIN(E[i-1][j], E[i-1][j+1]));
		}
	}

	int col_id = dim_fasie_width - 1, min = -1, min_id = -1;
	min_column(E, col_id, height, min, min_id);

	while (1)
	{
		for (int j = 0; j < min_id; j++)
		{
#ifdef DEBUG
			pout->set(col_id + in_win_width + id * dim_bucata_width, j,
					pixel(255, 0, 0));
#else
			pout->set(j, col_id + in_width + id * dim_bucata_width, pout->get(
					j, col_id + off_candidat_min + dim_bucata_width
							- dim_fasie_width));
#endif
		}

		for (int j = min_id; j < height; j++)
		{
#ifdef DEBUG
			pout->set(j, col_id + in_width + id * dim_bucata_width,
					pixel(0, 255, 0));
#else
			pout->set(j, col_id + in_width + id * dim_bucata_width, pout->get(
					j, col_id + in_width + id * dim_bucata_width
							- dim_fasie_width));
#endif
		}

		if (col_id <= 0)
			break;

		col_id--;

		if (min_id == 0)
		{
			min = MIN(E[col_id][0], E[col_id][1]);
			if (min == E[col_id][0])
				min_id = 0;
			else
				min_id = 1;
		}
		else if (min_id == height - 1)
		{
			min = MIN(E[col_id][height - 2], E[col_id][height - 1]);
			if (min == E[col_id][height - 2])
				min_id = height - 2;
			else
				min_id = height - 1;
		}
		else
		{
			min = MIN(E[col_id][min_id - 1],
					MIN(E[col_id][min_id], E[col_id][min_id + 1]));

			if (min == E[col_id][min_id - 1])
				min_id = min_id - 1;
			else if (min == E[col_id][min_id + 1])
				min_id = min_id + 1;
		}
	}

	for (int i = 0; i < dim_fasie_width; i++)
	{
		free(e[i]);
		free(E[i]);
	}

	free(e);
	free(E);
}

static int vertical_dif(Picture *pout, int off_candidat, int off_add,
		int dim_fasie_height, int dim_bucata_height, int width);

static void vertical_netezire(Picture *pout, int id, int in_height, int width,
		int dim_fasie_height, int dim_bucata_height, int off_candidat_min);

static void vertical_merge(Picture *pout, int width, int dim_fasie_height,
		int in_height, int id, int dim_bucata_height, int off_candidat_min);

void quilt_vertical(Picture *pout, int dim_bucata_width, int dim_bucata_height,
		int nbucati_width, int nbucati_height, int dim_fasie_width,
		int dim_fasie_height, int npatches, int ncandidati, int in_height)
{
	int width = pout->Width();

	int off_candidat = -1, off_candidat_min = -1;
	int min_dif = -1, curr_dif = -1;

	for (int i = 0; i < npatches; i++)
	{
		min_dif = 1 << 30;
		// Pentru fiecare petic nou adaugat, exista ncandidati petice
		// ce candideaza pentru ocuparea pozitiei
		for (int j = 0; j < ncandidati; j++)
		{
			off_candidat = rand() % (in_height - dim_fasie_height);

			curr_dif = vertical_dif(pout, off_candidat, in_height + (i - 1)
					* dim_bucata_height, dim_fasie_height, dim_bucata_height,
					width);

			if (min_dif > curr_dif)
			{
				min_dif = curr_dif;
				off_candidat_min = off_candidat;
			}
		}

		vertical_netezire(pout, i, in_height, width, dim_fasie_height,
				dim_bucata_height, off_candidat_min);
	}
}

static int vertical_dif(Picture *pout, int off_candidat, int off_add,
		int dim_fasie_height, int dim_bucata_height, int width)
{
	int diff = 0;

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < dim_fasie_height; j++)
		{
			diff += pixel::compare(pout->get(off_candidat + j, i), pout->get(
					off_add - dim_fasie_height + j, i));
		}
	}

	return diff;
}

static void vertical_netezire(Picture *pout, int id, int in_height, int width,
		int dim_fasie_height, int dim_bucata_height, int off_candidat_min)
{
	for (int i = 0; i < dim_bucata_height; i++)
	{
		for (int j = 0; j < width; j++)
		{
#ifdef DEBUG
			pout->set(i + in_height + id * dim_bucata_height, j, pixel(0, 255,
							0));
#else
			pout->set(i + in_height + id * dim_bucata_height, j, pout->get(i
					+ off_candidat_min, j));
#endif
		}
	}

	vertical_merge(pout, width, dim_fasie_height, in_height, id,
			dim_bucata_height, off_candidat_min);
}

static void vertical_merge(Picture *pout, int width, int dim_fasie_height,
		int in_height, int id, int dim_bucata_height, int off_candidat_min)
{
#ifdef DEBUG
	for (int i = 0; i < dim_fasie_height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pout->set(i + in_height + id * dim_bucata_height, j, pixel(255, 0,
							0));
		}
	}
	return;
#endif
	/*
	 * Cele doua fasii sunt descrise de:
	 * 		in_height + id * dim_bucata_height - dim_fasie_height
	 * 		off_candidat_min + dim_bucata_height - dim_fasie_height
	 * Calculez eroarea minima de suprapunere intre cele doua fasii dupa urmatoarea
	 * dinamica: E(i, j) = e(i, j) + min{E(i - 1, j - 1), E(i - 1, j), E(i - 1, j + 1)}
	 */
	int **e, **E;
	int candidat1 = in_height + id * dim_bucata_height - dim_bucata_height;
	int candidat2 = off_candidat_min;

	e = (int **) malloc(dim_fasie_height * sizeof(int *));
	E = (int **) malloc(dim_fasie_height * sizeof(int *));
	for (int i = 0; i < dim_fasie_height; i++)
	{
		e[i] = (int *) malloc(width * sizeof(int));
		E[i] = (int *) malloc(width * sizeof(int));
	}

	for (int i = 0; i < dim_fasie_height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			e[i][j] = pixel::compare(pout->get(i, j + candidat1), pout->get(i,
					j + candidat2));
		}
	}

	for (int j = 0; j < width; j++)
		E[0][j] = e[0][j];

	for (int i = 1; i < dim_fasie_height; i++)
	{

		for (int j = 0; j < width; j++)
		{
			if (j == 0)
				E[i][0] = MIN(E[i - 1][0], E[i - 1][1]);
			else if (j == width - 1)
				E[i][width - 1] = MIN(E[i - 1][width - 2], E[i - 1][width
						- 1]);
			else
				E[i][j] = MIN(E[i-1][j-1], MIN(E[i-1][j], E[i-1][j+1]));
		}
	}

	int line_id = dim_fasie_height - 1, min, min_id;
	min_line(E, line_id, width, min, min_id);

	while (1)
	{
		for (int j = 0; j < min_id; j++)
		{
#ifdef DEBUG
			pout->set(line_id + in_height + id * dim_bucata_height, j, pixel(
							255, 0, 0));
#else
			pout->set(line_id + in_height + id * dim_bucata_height, j,
					pout->get(line_id + off_candidat_min + dim_bucata_height
							- dim_fasie_height, j));
#endif
		}

		for (int j = min_id; j < width; j++)
		{
#ifdef DEBUG
			pout->set(line_id + in_height + id * dim_bucata_height, j, pixel(0,
							255, 0));
#else
			pout->set(line_id + in_height + id * dim_bucata_height, j,
					pout->get(line_id + in_height + id * dim_bucata_height
							- dim_fasie_height, j));
#endif
		}

		if (line_id <= 0)
			break;

		line_id--;

		if (min_id == 0)
		{
			min = MIN(E[line_id][0], E[line_id][1]);
			if (min == E[line_id][0])
				min_id = 0;
			else
				min_id = 1;
		}
		else if (min_id == width - 1)
		{
			min = MIN(E[line_id][width - 2], E[line_id][width - 1]);
			if (min == E[line_id][width - 2])
				min_id = width - 2;
			else
				min_id = width - 1;
		}
		else
		{
			min = MIN(E[line_id][min_id - 1],
					MIN(E[line_id][min_id], E[line_id][min_id + 1]));

			if (min == E[line_id][min_id - 1])
				min_id = min_id - 1;
			else if (min == E[line_id][min_id + 1])
				min_id = min_id + 1;
		}
	}
}
