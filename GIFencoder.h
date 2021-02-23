#ifndef _GIFENCODER_H_
#define _GIFENCODER_H_

#define MAX_COLORS 256

#include "stdio.h"

typedef struct _imageStruct {
	int width;
	int height;
	char *pixels;
	char *colors;
	int numColors;
	char minCodeSize;
} imageStruct;


typedef struct{
	char * key;
	int index;
}Dict;


//----------------------------------------------------------


imageStruct* GIFEncoder(unsigned char *data, int width, int height);
void RGB2Indexed(unsigned char *data, imageStruct* image);
int nextPower2(int n);
char numBits(int n);
void GIFEncoderWrite(imageStruct* image, char* outputFile);
void writeGIFHeader(imageStruct* image, FILE* file);
void writeImageBlockHeader(imageStruct* image, FILE* file);
void LZWCompress(FILE * file, char minCodeSize, char * pixels, int npixel);

//--------------------



Dict * initDict(char minCodSize, int * finalpos, int clearcode, int endofinfo);
void addDict(Dict * dict, int dictpos, char * new);
int findDict(Dict * dict, char * target, int dictpos);
char * intToChar(int n);
int numDigs(int n);
void writeToFile();
void emptyBuffer();
void writeCode();
void resetDict();



#endif