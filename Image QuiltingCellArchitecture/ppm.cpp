/*
 * Dascalu Laurentiu, 335CA
 * Functii de manipulare a imaginilor PPM, format P3.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <malloc.h>

#include "ppm.hpp"

using namespace std;

#define MAX		128

// Buffer pentru operatii I/O
char buffer[MAX];

Picture::Picture(void)
{
	width = 0;
	height = 0;
	image = NULL;
}

Picture::Picture(int h, int w, int color)
{
	width = w;
	height = h;
	max_color = color;
	image = (pixel *) memalign(128, height * width * sizeof(pixel));
	memset(image, 0, height * width * sizeof(pixel));
}

Picture::~Picture(void)
{
	free(image);
}

void Picture::set(int row, int col, pixel color)
{
	image[row * width + col] = color;
}

pixel Picture::get(int row, int col)
{
	return image[row * width + col];
}

pixel *Picture::getp(int row, int col)
{
	return &image[row * width + col];
}

int Picture::Height(void)
{
	return height;
}

int Picture::Width(void)
{
	return width;
}

int Picture::Color()
{
	return max_color;
}

pixel* Picture::pixels(void)
{
	return image;
}

void Picture::load(char *imageName)
{
	FILE *imageFile;

	if ((imageFile = fopen(imageName, "rt")) == NULL)
	{
		perror("invalid input file\n");
		return;
	}

	// Citesc header-ul
	fgets(buffer, MAX, imageFile);

	if (strcmp(buffer, "P3\n"))
	{
		perror("invalid input file format\n");
		return;
	}

	// Citesc comentariul si il ignor
	fgets(buffer, MAX, imageFile);

	fgets(buffer, MAX, imageFile);

	width = atoi(strtok(buffer, " \n"));
	height = atoi(strtok(NULL, " \n"));
	max_color = atoi(fgets(buffer, MAX, imageFile));

	reset(height, width);

	int red;
	int green;
	int blue;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			fscanf(imageFile, "%d", &red);
			fscanf(imageFile, "%d", &green);
			fscanf(imageFile, "%d", &blue);

			image[i * width + j].red = red;
			image[i * width + j].green = green;
			image[i * width + j].blue = blue;
		}
	}

	fclose(imageFile);
}

void Picture::save(char *imageName)
{
	FILE *imageFile;

	if ((imageFile = fopen(imageName, "wt")) == NULL)
	{
		perror("invalid input file\n");
		return;
	}

	// Scriu header-ul
	fprintf(imageFile, "P3\n");

	// Scriu comentariul
	fprintf(imageFile,
			"# CREATOR: Dascalu Laurentiu's Image Quilting Application\n");

	fprintf(imageFile, "%d %d\n", width, height);
	fprintf(imageFile, "%d\n", max_color);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			fprintf(imageFile, "%d\n", image[i * width + j].red);
			fprintf(imageFile, "%d\n", image[i * width + j].green);
			fprintf(imageFile, "%d\n", image[i * width + j].blue);
		}
	}

	fclose(imageFile);
}

void Picture::reset(int height, int width)
{
	this->height = height;
	this->width = width;

	image = (pixel *) memalign(128, height * width * sizeof(pixel));
	memset(image, 0, height * width * sizeof(pixel));

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
		{
			image[i * width + j].red = 0;
			image[i * width + j].green = 0;
			image[i * width + j].blue = 0;
		}
}

int pixel::compare(pixel p1, pixel p2)
{
	return (p1.red - p2.red) * (p1.red - p2.red) + (p1.green - p2.green)
			* (p1.green - p2.green) + (p1.blue - p2.blue) * (p1.blue - p2.blue);
}
