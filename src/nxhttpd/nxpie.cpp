/* $Id: nxpie.cpp,v 1.1 2007-03-21 10:15:18 alk Exp $ */

#include "nxpie.h"

NxPie::NxPie()
{
	rawData = NULL;
	rawDataSize = 0;
	name.clear();
	value.clear();
	colorCount = 0;
}

NxPie::~NxPie()
{
	if (rawData != NULL)
	{
		gdFree(rawData);
	}
}

bool NxPie::setValue(string label, double value)
{
	if (name.size() < MAX_PIE_ELEMENTS)
	{
		name.push_back(label);
		this->value.push_back(value);
		return true;
	}

	return false;
}

bool NxPie::build()
{
	bool ret = false;
	gdImagePtr img;
	double arc_rad[MAX_PIE_ELEMENTS];
	double arc_dec[MAX_PIE_ELEMENTS];
	double perc[MAX_PIE_ELEMENTS];
	double total = 0;
	unsigned int i;
	int posX, posY;

	// colors
	int black;
	int background;

	img = gdImageCreate(PIE_WIDTH, PIE_HEIGHT);

	int centX = gdImageSX(img) / 2;
	int centY = gdImageSY(img) / 2;

	int d = 160;

	background = gdImageColorAllocate(img, 255, 255, 255);
	black = gdImageColorAllocate(img, 0, 0, 0);

	color[0] = gdImageColorAllocate(img,   0, 127,   0);
	color[1] = gdImageColorAllocate(img, 255, 255,   0);
	color[2] = gdImageColorAllocate(img, 249, 131,   0);
	color[3] = gdImageColorAllocate(img, 248,  63,   1);
	color[4] = gdImageColorAllocate(img, 200,   0,   0);
	colorCount = 5;

	/* excel colors
		color[0] = gdImageColorAllocate(img, 144, 151, 255);
		color[1] = gdImageColorAllocate(img, 144,  48,  96);
		color[2] = gdImageColorAllocate(img, 255, 255, 192);
		color[3] = gdImageColorAllocate(img, 207, 255, 255);
		color[4] = gdImageColorAllocate(img,  95,   0,  95);
		color[5] = gdImageColorAllocate(img, 255, 127, 127);
		color[6] = gdImageColorAllocate(img, 207, 255, 255);
		color[7] = gdImageColorAllocate(img,   0,  96, 192);
		color[8] = gdImageColorAllocate(img, 207, 200, 255);
		*/

	gdImageFill(img, 0, 0, background);
	gdImageColorTransparent(img, background);

	// count total
	for (i = 1; i <= name.size(); i++)
	{
		total += value[i - 1];
		arc_rad[i] = total * 2 * PI;
		arc_dec[i] = total * 360;
	}

	arc_rad[0] = 0;
	arc_dec[0] = 0;

	// draw boundaries
	for (i = 1; i <= name.size(); i++)
	{
		perc[i - 1] = value[i - 1] / total;

		arc_rad[i] /= total;
		arc_dec[i] /= total;

		posX = (int)ROUND(centX + (d / 2) * sin(arc_rad[i]));
		posY = (int)ROUND(centY + (d / 2) * cos(arc_rad[i]));

		gdImageLine(img, centX, centY, posX, posY, black);
		gdImageArc(img, centX, centY, d, d, (int)(arc_dec[i - 1]), (int)(arc_dec[i]), black);
	}

	int currentColor = 0;
	//int hfw = gdImageFontWidth(1);
	//int vfw = gdImageFontHeight(1);
	int fontX = gdFontGetTiny()->w;
	int fontY = gdFontGetTiny()->h;

	// draw labels and fill
	for (i = 0; i < name.size(); i++)
	{
		double arc_rad_label = arc_rad[i] + 0.5 * perc[i] * 2 * PI;

		// fill
		posX = (int)(centX + (0.8 * (d / 2) * sin(arc_rad_label)));
		posY = (int)(centY + (0.8 * (d / 2) * cos(arc_rad_label)));

		if (currentColor == colorCount)
		{
			currentColor = 0;
		}
		gdImageFillToBorder(img, posX, posY, black, color[currentColor]);
		currentColor++;

		if (perc[i] > 0)
		{
			// label
			char s[128];
			snprintf(s, 128, "%s (%.2lf%%)", name[i].c_str(), perc[i] * 100);
			s[127] = 0;

			posX = (int)(centX + (1.1 * (d / 2) * sin(arc_rad_label)));
			posY = (int)(centY + (1.1 * (d / 2) * cos(arc_rad_label)));
			if ((arc_rad_label > 0.5 * PI) && (arc_rad_label < 1.5 * PI))
			{
				posY = posY - fontY;
			}
			if (arc_rad_label > PI)
			{
				posX = posX - fontX * strlen(s);
			}
			gdImageString(img, gdFontGetTiny(), posX, posY, (unsigned char *)s, black);
		}
	}

	rawData = gdImagePngPtr(img, &rawDataSize);
	if (rawData != NULL)
	{
		ret = true;
	}

	gdImageDestroy(img);

	return ret;
}

void NxPie::free(void)
{
	if (rawData != NULL)
	{
		gdFree(rawData);
		rawData = NULL;
		rawDataSize = 0;
	}
}

void *NxPie::getRawImage(void)
{
	return rawData;
}

int NxPie::getRawImageSize(void)
{
	return rawDataSize;
}

/*
int main(int argc, char *argv[])
{
	NxPie pie;

	pie.setValue("Label 1", 10);
	pie.setValue("Label 2", 11);
	pie.setValue("Label 3", 12);
	pie.setValue("Label 4", 13);
	pie.setValue("Label 5", 14);
	pie.setValue("Label 6", 15);
	pie.setValue("Label 7", 16);
	pie.setValue("Label 8", 17);
	pie.setValue("Label 9", 18);

	pie.build();

	printf("Content-type: image/png\nContent-Lenght: %d\n\n", pie.getRawImageSize());
	fwrite(pie.getRawImage(), 1, pie.getRawImageSize(), stdout);

	pie.free();

	return 0;
}
*/

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
