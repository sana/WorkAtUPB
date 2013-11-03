#include <stdio.h>
#include <spu_mfcio.h>
#include <vector>
#include "../ppm.hpp"

static int pixel_compare(pixel p1, pixel p2)
{
	return (p1.red - p2.red) * (p1.red - p2.red) + (p1.green - p2.green)
			* (p1.green - p2.green) + (p1.blue - p2.blue) * (p1.blue - p2.blue);
}

unsigned int addr __attribute__((aligned(128)));
unsigned int packet1 __attribute__((aligned(128)));
unsigned int packet2 __attribute__((aligned(128)));
unsigned int packet_aux1 __attribute__((aligned(128)));
unsigned int packet_aux2 __attribute__((aligned(128)));

pixel values1[1024] __attribute__((aligned(128)));
pixel values2[1024] __attribute__((aligned(128)));

pixel values_aux1[1024] __attribute__((aligned(128)));
pixel values_aux2[1024] __attribute__((aligned(128)));

static std::vector<int> diff_vect;

void calculate_diff(pixel values1[1024], pixel values2[1024], int height,
		int dim_fasie_width)
{
	int diff = 0;

	for (int i = 0; i < height; i++)
	{
		/*
		 * Pixelii de pe fasie se proiecteaza in diferenta.
		 */
		for (int j = 0; j < dim_fasie_width; j++)
			diff += pixel_compare(values1[i * dim_fasie_width + j], values2[i
					* dim_fasie_width + j]);
	}
	diff_vect.push_back(diff);
}

int main(unsigned long long speid, unsigned long long __size,
		unsigned long long id)
{
	int size, fasie_size;

	while (1)
	{
		while (spu_stat_out_intr_mbox() == 0)
			;
		addr = spu_read_in_mbox();

		if (addr == 0)
			break;

		while (spu_stat_out_intr_mbox() == 0)
			;
		size = spu_read_in_mbox();

		while (spu_stat_out_intr_mbox() == 0)
			;
		fasie_size = spu_read_in_mbox();

		while (spu_stat_out_intr_mbox() == 0)
			;
		packet1 = spu_read_in_mbox();

		while (spu_stat_out_intr_mbox() == 0)
			;
		packet2 = spu_read_in_mbox();

		mfc_get(&values1, packet1, size * fasie_size * sizeof(pixel), 31, 0, 0);
		mfc_get(&values2, packet2, size * fasie_size * sizeof(pixel), 31, 0, 0);
		mfc_write_tag_mask(1 << 31);
		mfc_read_tag_status_all();

		// Inainte sa m-apuc de calculul diferentei,
		// incep transferul altui bloc de date
		while (spu_stat_out_intr_mbox() == 0)
			;
		addr = spu_read_in_mbox();

		if (addr == 2)
		{
			int min_diff = diff_vect[0];

			for (unsigned int i = 1; i < diff_vect.size(); i++)
				if (diff_vect[i] < min_diff)
					min_diff = diff_vect[i];

			while (spu_stat_out_intr_mbox() == 0)
				;
			spu_write_out_intr_mbox(min_diff);
		}
		else
		{

			while (spu_stat_out_intr_mbox() == 0)
				;
			size = spu_read_in_mbox();

			while (spu_stat_out_intr_mbox() == 0)
				;
			fasie_size = spu_read_in_mbox();

			while (spu_stat_out_intr_mbox() == 0)
				;
			packet_aux1 = spu_read_in_mbox();

			while (spu_stat_out_intr_mbox() == 0)
				;
			packet_aux2 = spu_read_in_mbox();

			mfc_get(&values_aux1, packet_aux1, sizeof(float), 31, 0, 0);
			mfc_get(&values_aux2, packet_aux2, sizeof(float), 31, 0, 0);
			mfc_write_tag_mask(1 << 31);

			calculate_diff(values1, values2, size, fasie_size);

			mfc_read_tag_status_all();

			calculate_diff(values_aux1, values_aux2, size, fasie_size);
		}
	}
}
