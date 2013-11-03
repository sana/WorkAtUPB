#!/bin/bash
make clean
make

# Zoom = 2
# Numar bucati height = 3
# Numar bucati width = 8
# Banda de suprapunere height = 0.2
# Banda de suprapunere width = 0.2
# Numar de canditati 4 per SPU
./ppu input/S29.ppm output/S29.ppm 3 3 8 0.2 0.2 4
./ppu input/gmesh.ppm output/gmesh.ppm 3 3 8 0.2 0.2 4

