#ifndef PPM_HPP_
#define PPM_HPP_

#include <string>

struct pixel
{
	int red;
	int green;
	int blue;

public:
	/*
	 * Intoarce diferenta dintre doi pixeli:
	 * 	(r1-r2)^2+(g1-g2)^2+(b1-b2)^2
	 */
	static int compare(pixel p1, pixel p2);

	pixel(int red = 0, int green = 0, int blue = 0) :
		red(red), green(green), blue(blue)
	{

	}

	void print()
	{
		printf("(%d %d %d)", red, green, blue);
	}
};

class Picture
{
private:
	pixel *image;
	int width, height, max_color;

	void reset(int h, int w);

	Picture(const Picture &);

public:
	Picture(int h, int w, int color);
	Picture(void);
	~Picture(void);
	void save(char *filename);
	void load(char *filename);
	void set(int row, int col, pixel color);
	pixel get(int row, int col);
	pixel *getp(int row, int col);
	int Color();
	int Width();
	int Height();
	pixel *pixels();
};

#endif /* PPM_HPP_ */
