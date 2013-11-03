#ifndef QUILT_HPP_
#define QUILT_HPP_

#include "ppm.hpp"

// Operatiile pentru Quilting-ul orizontal
void quilt_orizontal(Picture *pout, int dim_bucata_height,
		int dim_bucata_width, int nbucati_height, int nbucati_width,
		int dim_fasie_height, int dim_fasie_width, int npatches,
		int ncandidati, int in_width);

// Operatiile pentru Quilting-ul vertical
void quilt_vertical(Picture *pout, int dim_bucata_width, int dim_bucata_height,
		int nbucati_width, int nbucati_height, int dim_fasie_width,
		int dim_fasie_height, int npatches, int ncandidati, int in_height);

#endif /* QUILT_HPP_ */
