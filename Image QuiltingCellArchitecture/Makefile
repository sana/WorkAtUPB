# Dascalu Laurentiu, 335CA; Tema4 Cell Image Quilting
CFLAGS = -O3

################
# Subdirectories
################
DIRS := spu

################
# Target
################
PROGRAM_ppu := ppu


################
# Local Defines
################

IMPORTS          := spu/lib_spu.a -lspe2 -lpthread

################
# make.footer
################

# make.footer is in the top of the SDK
ifdef CELL_TOP
    include $(CELL_TOP)/buildutils/make.footer
else
    include ../../../../buildutils/make.footer
endif
