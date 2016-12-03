// EntropyDecode.cpp
// Source file for EntropyDecode.h
// File defines functions declared in header
// File contains references from H.264 Reference Software
// Author: Benny Chou
// Created: Nov. 2016

#include "EntropyDecode.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define MAXRTPPAYLOADLEN (65536 - 40)

void fillArrayZeros(int *inputArray)
{
	for (int k = 0; k < 16; k++)
	{
		inputArray[k] = 0;
	}
}

static inline int imin(int a, int b)
{
	return ((a) < (b)) ? (a) : (b);
}

int GetBits(byte buffer[], int totbitoffset, int *info, int bitcount, int numbits)
{
	if ((totbitoffset + numbits) > bitcount)
	{
		return -1;
	}
	else
	{
		int bitoffset = 7 - (totbitoffset & 0x07); //bit from start of byte
		int byteoffset = (totbitoffset >> 3); //byte from start of buffer
		int bitcounter = numbits;
		byte *curbyte = &(buffer[byteoffset]);
		int inf = 0;

		while (numbits--)
		{
			inf <<= 1;
			inf |= ((*curbyte) >> (bitoffset--)) & 0x01;
			if (bitoffset == -1)
			{
				//Move onto next byte to get all of numbits
				curbyte++;
				bitoffset = 7;
			}
		}
		*info = inf;

		return bitcounter;
	}
}

static inline int ShowBitsThres(int inf, int numbits)
{
	return  ((inf) >> ((sizeof(byte) * 24) - (numbits)));
}

//code from bitstream 2d tables
static  int code_from_bitstream_2d(SyntaxElement *sym, Bitstream *currStream, const byte *lentab, const byte *codtab, int tabwidth, int tabheight, int *code)
{
	int i, j;
	const byte *len = &lentab[0], *cod = &codtab[0];

	int *frame_bitoffset = &currStream->frame_bitoffset;
	byte *buf = &currStream->streamBuffer[*frame_bitoffset >> 3];

	//Apply bitoffset to three bytes (maximum that may be traversed by ShowBitsThres)
	unsigned int inf = ((*buf) << 16) + (*(buf + 1) << 8) + *(buf + 2); //Even at the end of a stream we will still be pulling out of allocated memory as alloc is done by MAX_CODED_FRAME_SIZE
	inf <<= (*frame_bitoffset & 0x07);                                  //Offset is constant so apply before extracting different numbers of bits
	inf &= 0xFFFFFF;                                                   //Arithmetic shift so wipe any sign which may be extended inside ShowBitsThres

																	   // this VLC decoding method is not optimized for speed
	for (j = 0; j < tabheight; j++)
	{
		for (i = 0; i < tabwidth; i++)
		{
			if ((*len == 0) || (ShowBitsThres(inf, (int)*len) != *cod))
			{
				++len;
				++cod;
			}
			else
			{
				sym->len = *len;
				*frame_bitoffset += *len; // move bitstream pointer
				*code = *cod;
				sym->value1 = i;
				sym->value2 = j;
				return 0;                 // found code and return 
			}
		}
	}
	return -1;  // failed to find code
}

EntropyDecode::EntropyDecode(void)
{
}

EntropyDecode::~EntropyDecode(void)
{
}

void EntropyDecode::decodeVLC(float *outputBuffer, unsigned char *inputBuffer, int iWidth, int iHeight, float fQstep, int iQMtd)
{
	//input buffer contains our relevant bitstream
	//for now ignore quantization
	SyntaxElement currSE;
	Bitstream currStream;

	currStream.streamBuffer = inputBuffer;
	char type[15];

	currStream.frame_bitoffset = 0;
	currStream.bitstream_length = 43009; //for this example specifically, will be passed into func later

	int numcoeff, numtrailingones, numones, ntr, code, level_two_or_higher;
	int vlcnum, abslevel, totzeros, zerosleft;
	int *levarr = new int[16];
	int *runarr = new int[16];
	int *recArray = new int[16];
	static const int incVlc[] = { 0, 3, 6, 12, 24, 48, 32768 };    // maximum vlc = 6

	fillArrayZeros(levarr);
	fillArrayZeros(runarr);
	fillArrayZeros(recArray);

	int *coeffStorage = new int[(iWidth * iHeight) / 16];

	//for all macroblocks 
	for (int position = 0; position < ((iWidth * iHeight) / 16); position++)
	{
		fillArrayZeros(levarr);
		fillArrayZeros(runarr);
		fillArrayZeros(recArray);

		//predict neighbour non-zero coefficients
		int nnz = predict_nnz(position, iWidth, iHeight, coeffStorage);

		if (position == 0)
		{
			currSE.value1 = 0;
		}
		else
		{
			currSE.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
		}

		readSyntaxElement_NumCoeffTrailingOnes(&currSE, &currStream, type);

		//decode numcoeff and num trailing ones
		numcoeff = currSE.value1;
		numtrailingones = currSE.value2;

		//store numcoeff for other predictions
		coeffStorage[position] = numcoeff;

		//decode trailing one signs
		numones = numtrailingones;

		if (numcoeff)
		{
			if (numtrailingones)
			{
				currSE.len = numtrailingones;

				readSyntaxElement_FLC(&currSE, &currStream);

				code = currSE.inf;
				ntr = numtrailingones; //ntr = numtrailingones

		//place trailing ones into temp array levarr
				for (int j = numcoeff - 1; j > numcoeff - 1 - numtrailingones; j--)
				{
					ntr--;
					levarr[j] = (code >> ntr) & 1 ? -1 : 1;
				}
			}

			//decode levels
			if (numcoeff > 3 && numtrailingones == 3)
			{
				level_two_or_higher = 0;
			}
			else
			{
				level_two_or_higher = 1;
			}

			if (numcoeff > 10 && numtrailingones < 3)
			{
				vlcnum = 1;
			}
			else
			{
				vlcnum = 0;
			}

			for (int k = numcoeff - 1 - numtrailingones; k >= 0; k--)
			{
				if (vlcnum == 0)
				{
					readSynTaxElement_level_VLC0(&currSE, &currStream);
				}
				else
				{
					readSyntaxElement_Level_VLCN(&currSE, vlcnum, &currStream);
				}

				if (level_two_or_higher)
				{
					currSE.inf += (currSE.inf > 0) ? 1 : -1;
					level_two_or_higher = 0;
				}

				levarr[k] = currSE.inf;
				abslevel = abs(levarr[k]);

				if (abslevel == 1)
				{
					++numones;
				}

				// update VLC table
				if (abslevel > incVlc[vlcnum])
				{
					++vlcnum;
				}

				if (k == numcoeff - 1 - numtrailingones && abslevel > 3)
				{
					vlcnum = 2;
				}
			}

			if (numcoeff < 16)
			{
				//decode total run
				vlcnum = numcoeff - 1;
				currSE.value1 = vlcnum;

				//decode to zeros
				readSyntaxElement_TotalZeros(&currSE, &currStream);

				totzeros = currSE.value1;
			}
			else
			{
				totzeros = 0;
			}

			// decode run before each coefficient
			zerosleft = totzeros;
			int i = numcoeff - 1;

			if (zerosleft > 0 && i > 0)
			{
				do
				{
					// select vlc for runbefore
					vlcnum = imin(zerosleft - 1, 6);

					currSE.value1 = vlcnum;

					readSyntaxElement_Run(&currSE, &currStream);
					runarr[i] = currSE.value1;

					zerosleft -= runarr[i];
					i--;
				} while (zerosleft != 0 && i != 0);
			}
			runarr[i] = zerosleft;
		}
		reconstructScannedArray(recArray, levarr, runarr);
		//pass reconstructed array back to output buffer but as float type
		sendBack(position, recArray, outputBuffer, iWidth, iHeight);
	}
}

//copies the reconstructed and array back into the output buffer at specific position
//needs to place in reverse zig zag order
void EntropyDecode::sendBack(int position, int *recArray, float *outputBuffer, int iWidth, int iHeight)
{
	int heightLoc = 0;
	int widthLoc = 0;
	int topLeftCorner = 0;

	if (position == 0)
	{
		heightLoc = 0;
	}
	else
	{
		heightLoc = (iWidth / 4) % position;//height of macroblock
	}

	topLeftCorner = position * 4 + heightLoc * (iWidth * 4); //top left corner of macroblock is related to x (position) and y (heightLoc)

	//top row
	outputBuffer[topLeftCorner] = (float)recArray[0];
	outputBuffer[topLeftCorner + 1] = (float)recArray[1];
	outputBuffer[topLeftCorner + 2] = (float)recArray[5];
	outputBuffer[topLeftCorner + 3] = (float)recArray[6];

	//second row
	outputBuffer[topLeftCorner + iWidth] = (float)recArray[2];
	outputBuffer[topLeftCorner + iWidth + 1] = (float)recArray[4];
	outputBuffer[topLeftCorner + iWidth + 2] = (float)recArray[7];
	outputBuffer[topLeftCorner + iWidth + 3] = (float)recArray[12];

	//third row
	outputBuffer[topLeftCorner + (iWidth * 2)] = (float)recArray[3];
	outputBuffer[topLeftCorner + (iWidth * 2) + 1] = (float)recArray[8];
	outputBuffer[topLeftCorner + (iWidth * 2) + 2] = (float)recArray[11];
	outputBuffer[topLeftCorner + (iWidth * 2) + 3] = (float)recArray[13];

	//fourth row
	outputBuffer[topLeftCorner + (iWidth * 3)] = (float)recArray[9];
	outputBuffer[topLeftCorner + (iWidth * 3) + 1] = (float)recArray[10];
	outputBuffer[topLeftCorner + (iWidth * 3) + 2] = (float)recArray[14];
	outputBuffer[topLeftCorner + (iWidth * 3) + 3] = (float)recArray[15];
}

//predicts the neighbouring total non-zero coefficients
int EntropyDecode::predict_nnz(int position, int iWidth, int iHeight, int *coefficientStorage)
{
	int nnz;
	int nnzTop;
	int nnzLeft;
	//edge cases will only have 1 neighbour or no neighbour if first case
	//top left corner case, no neighbours either side
	if (position == 0)
	{
		//vlc 0 will be used
		nnz = 0;
		return nnz;
	}
	//top row case
	else if (position < (iWidth / 4))
	{
		nnz = coefficientStorage[position - 1]; //left neighbour
		return nnz;
	}
	//left edge case
	else if (position % (iWidth / 4) == 0)
	{
		nnz = coefficientStorage[position - (iWidth / 4)]; //top neighbour
		return nnz;
	}
	else
	{
		nnzLeft = coefficientStorage[position - 1];
		nnzTop = coefficientStorage[position - (iWidth / 4)];

		nnz = (nnzLeft + nnzTop) / 2;
		return nnz;
	}
}

//takes in the levarray and runarray and reconstructs the zig-zag scanned array
void EntropyDecode::reconstructScannedArray(int *outputArray, int *levArray, int *runArray)
{
	int pos = 0;
	int runs;
	int levCounter = 0;
	int runCounter = 0;

	while (pos < 16)
	{
		//check if any runs
		//if run, pad with 0 for run value
		if (runArray[runCounter] > 0)
		{
			int runs = runArray[runCounter];

			runCounter++;

			while (runs > 0)
			{
				outputArray[pos] = 0;
				runs--;
				pos++;
			}

			outputArray[pos] = levArray[levCounter];
			pos++;
			levCounter++;
		}
		else //if no runs, the value at the return array should be from level array
		{
			outputArray[pos] = levArray[levCounter];
			pos++;
			levCounter++;
			runCounter++;
		}
	}
}

int EntropyDecode::readSyntaxElement_Run(SyntaxElement *sym, Bitstream *currStream)
{
	static const byte lentab[15][16] =
	{
		{ 1,1 },
		{ 1,2,2 },
		{ 2,2,2,2 },
		{ 2,2,2,3,3 },
		{ 2,2,3,3,3,3 },
		{ 2,3,3,3,3,3,3 },
		{ 3,3,3,3,3,3,3,4,5,6,7,8,9,10,11 },
	};

	static const byte codtab[15][16] =
	{
		{ 1,0 },
		{ 1,1,0 },
		{ 3,2,1,0 },
		{ 3,2,1,1,0 },
		{ 3,2,3,2,1,0 },
		{ 3,0,1,3,2,5,4 },
		{ 7,6,5,4,3,2,1,1,1,1,1,1,1,1,1 },
	};
	int code;
	int vlcnum = sym->value1;
	int retval = code_from_bitstream_2d(sym, currStream, &lentab[vlcnum][0], &codtab[vlcnum][0], 16, 1, &code);

	if (retval)
	{
		printf("ERROR: failed to find Run\n");
		exit(-1);
	}

	return retval;
}



int EntropyDecode::readSyntaxElement_TotalZeros(SyntaxElement *sym, Bitstream *currStream)
{
	static const byte lentab[15][16] =
	{

		{ 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9 },
		{ 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6 },
		{ 4,3,3,3,4,4,3,3,4,5,5,6,5,6 },
		{ 5,3,4,4,3,3,3,4,3,4,5,5,5 },
		{ 4,4,4,3,3,3,3,3,4,5,4,5 },
		{ 6,5,3,3,3,3,3,3,4,3,6 },
		{ 6,5,3,3,3,2,3,4,3,6 },
		{ 6,4,5,3,2,2,3,3,6 },
		{ 6,6,4,2,2,3,2,5 },
		{ 5,5,3,2,2,2,4 },
		{ 4,4,3,3,1,3 },
		{ 4,4,2,1,3 },
		{ 3,3,1,2 },
		{ 2,2,1 },
		{ 1,1 },
	};

	static const byte codtab[15][16] =
	{
		{ 1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1 },
		{ 7,6,5,4,3,5,4,3,2,3,2,3,2,1,0 },
		{ 5,7,6,5,4,3,4,3,2,3,2,1,1,0 },
		{ 3,7,5,4,6,5,4,3,3,2,2,1,0 },
		{ 5,4,3,7,6,5,4,3,2,1,1,0 },
		{ 1,1,7,6,5,4,3,2,1,1,0 },
		{ 1,1,5,4,3,3,2,1,1,0 },
		{ 1,1,1,3,3,2,2,1,0 },
		{ 1,0,1,3,2,1,1,1, },
		{ 1,0,1,3,2,1,1, },
		{ 0,1,1,2,1,3 },
		{ 0,1,1,1,1 },
		{ 0,1,1,1 },
		{ 0,1,1 },
		{ 0,1 },
	};

	int code;
	int vlcnum = sym->value1;
	int retval = code_from_bitstream_2d(sym, currStream, &lentab[vlcnum][0], &codtab[vlcnum][0], 16, 1, &code);

	if (retval)
	{
		printf("ERROR: failed to find Total Zeros !cdc\n");
		exit(-1);
	}

	return  retval;
}

int EntropyDecode::readSyntaxElement_Level_VLCN(SyntaxElement *sym, int vlc, Bitstream *currStream)
{
	int frame_bitoffset = currStream->frame_bitoffset;
	int BitStreamLengthInBytes = currStream->bitstream_length;
	int BitstreamLengthInBits = (BitStreamLengthInBytes << 3) + 7;
	byte *buf = currStream->streamBuffer;

	int levabs, sign;
	int len = 1;
	int code = 1, sb;

	int shift = vlc - 1;

	//read pre zeros
	while (!ShowBits(buf, frame_bitoffset++, BitstreamLengthInBits, 1))
	{
		len++;
	}
	
	frame_bitoffset -= len;

	if (len < 16)
	{
		levabs = ((len - 1) << shift) + 1;

		// read (vlc-1) bits -> suffix
		if (shift)
		{
			sb = ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, shift);
			code = (code << (shift)) | sb;
			levabs += sb;
			len += (shift);
		}

		// read 1 bit -> sign
		sign = ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, 1);
		code = (code << 1) | sign;
		len++;
	}
	else // escape
	{
		int addbit = len - 5;
		int offset = (1 << addbit) + (15 << shift) - 2047;

		sb = ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, addbit);
		code = (code << addbit) | sb;
		len += addbit;

		levabs = sb + offset;

		// read 1 bit -> sign
		sign = ShowBits(buf, frame_bitoffset + len, BitstreamLengthInBits, 1);

		code = (code << 1) | sign;

		len++;
	}

	sym->inf = (sign) ? -levabs : levabs;
	sym->len = len;

	currStream->frame_bitoffset = frame_bitoffset + len;

	return 0;
}

int EntropyDecode::readSynTaxElement_level_VLC0(SyntaxElement *sym, Bitstream *currStream)
{
	int frame_bitoffset = currStream->frame_bitoffset;
	int BitstreamLengthInBytes = currStream->bitstream_length;
	int BitstreamLengthInBits = (BitstreamLengthInBytes << 3) + 7;
	byte *buf = currStream->streamBuffer;
	int len = 1, sign = 0, level = 0, code = 1;

	while (!ShowBits(buf, frame_bitoffset++, BitstreamLengthInBits, 1))
	{
		len++;
	}
	if (len < 15)
	{
		sign = (len - 1) & 1;
		level = ((len - 1) >> 1) + 1;
	}
	else if (len == 15)
	{
		//escape code
		code <<= 4;
		code |= ShowBits(buf, frame_bitoffset, BitstreamLengthInBits, 4);
		len += 4;
		frame_bitoffset += 4;
		sign = (code & 0x01);
		level = ((code >> 1) & 0x07) + 8;
	}
	else if (len >= 16)
	{
		//escape code
		int addbit = (len - 16);
		int offset = (2048 << addbit) - 2032;
		len -= 4;
		code = ShowBits(buf, frame_bitoffset, BitstreamLengthInBits, len);
		sign = (code & 0x01);
		frame_bitoffset += len;
		level = (code >> 1) + offset;

		code |= (1 << (len)); // for display only
		len += addbit + 16;
	}

	sym->inf = (sign) ? -level : level;
	sym->len = len;

	currStream->frame_bitoffset = frame_bitoffset;
	return 0;
}

int EntropyDecode::readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream)
{
	int BitstreamLengthInBits = (currStream->bitstream_length << 3) + 7;

	if((GetBits(currStream->streamBuffer, currStream->frame_bitoffset, &(sym->inf), BitstreamLengthInBits, sym->len)) < 0)
	{ 
		return -1;
	}
	sym->value1 = sym->inf;
	currStream->frame_bitoffset += sym->len; //move bitstream pointer

	return 1;
}

//read  bits from the bitstream buffer
int EntropyDecode::ShowBits(byte buffer[], int totbitoffset, int bitcount, int numbits)
{
	if ((totbitoffset + numbits) > bitcount)
	{
		return -1;	//error
	}
	else
	{
		int bitoffset = 7 - (totbitoffset & 0x07); // bit  from start of byte
		int byteoffset = (totbitoffset >> 3); // byte from start of buffer
		byte *curbyte = &(buffer[byteoffset]);
		int inf = 0;

		while (numbits--)
		{
			inf <<= 1;
			inf |= ((*curbyte) >> (bitoffset--)) & 0x01;

			if (bitoffset == -1)
			{
				//Move onto next byte to get all numbits
				curbyte++;
				bitoffset = 7;
			}
		}
		//return absolute offset in bit from start of frame
		return inf; 
	}
}

int EntropyDecode::readSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *sym, Bitstream *currStream, char *type)
{
	int frame_bitoffset = currStream->frame_bitoffset;
	int BitstreamLengthInBytes = currStream->bitstream_length;
	int BitstreamLengthinBits = (BitstreamLengthInBytes << 3) + 7;
	byte *buf = currStream->streamBuffer;

	static const byte lentab[3][4][17] =
	{
		{   // 0702
			{ 1, 6, 8, 9,10,11,13,13,13,14,14,15,15,16,16,16,16 },
			{ 0, 2, 6, 8, 9,10,11,13,13,14,14,15,15,15,16,16,16 },
			{ 0, 0, 3, 7, 8, 9,10,11,13,13,14,14,15,15,16,16,16 },
			{ 0, 0, 0, 5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16 },
		},
		{
			{ 2, 6, 6, 7, 8, 8, 9,11,11,12,12,12,13,13,13,14,14 },
			{ 0, 2, 5, 6, 6, 7, 8, 9,11,11,12,12,13,13,14,14,14 },
			{ 0, 0, 3, 6, 6, 7, 8, 9,11,11,12,12,13,13,13,14,14 },
			{ 0, 0, 0, 4, 4, 5, 6, 6, 7, 9,11,11,12,13,13,13,14 },
		},
		{
			{ 4, 6, 6, 6, 7, 7, 7, 7, 8, 8, 9, 9, 9,10,10,10,10 },
			{ 0, 4, 5, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 9,10,10,10 },
			{ 0, 0, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,10 },
			{ 0, 0, 0, 4, 4, 4, 4, 4, 5, 6, 7, 8, 8, 9,10,10,10 },
		},
	};

	static const byte codtab[3][4][17] =
	{
		{
			{ 1, 5, 7, 7, 7, 7,15,11, 8,15,11,15,11,15,11, 7,4 },
			{ 0, 1, 4, 6, 6, 6, 6,14,10,14,10,14,10, 1,14,10,6 },
			{ 0, 0, 1, 5, 5, 5, 5, 5,13, 9,13, 9,13, 9,13, 9,5 },
			{ 0, 0, 0, 3, 3, 4, 4, 4, 4, 4,12,12, 8,12, 8,12,8 },
		},
		{
			{ 3,11, 7, 7, 7, 4, 7,15,11,15,11, 8,15,11, 7, 9,7 },
			{ 0, 2, 7,10, 6, 6, 6, 6,14,10,14,10,14,10,11, 8,6 },
			{ 0, 0, 3, 9, 5, 5, 5, 5,13, 9,13, 9,13, 9, 6,10,5 },
			{ 0, 0, 0, 5, 4, 6, 8, 4, 4, 4,12, 8,12,12, 8, 1,4 },
		},
		{
			{ 15,15,11, 8,15,11, 9, 8,15,11,15,11, 8,13, 9, 5,1 },
			{ 0,14,15,12,10, 8,14,10,14,14,10,14,10, 7,12, 8,4 },
			{ 0, 0,13,14,11, 9,13, 9,13,10,13, 9,13, 9,11, 7,3 },
			{ 0, 0, 0,12,11,10, 9, 8,13,12,12,12, 8,12,10, 6,2 },
		},
	};

	int retval = 0, code;
	int vlcnum = sym->value1;
	// vlcnum is the index of Table used to code coeff_token
	// vlcnum==3 means (8<=nC) which uses 6bit FLC

	if (vlcnum == 3)
	{
		// read 6 bit FLC
		//code = ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, 6);
		code = ShowBits(buf, frame_bitoffset, BitstreamLengthinBits, 6);
		currStream->frame_bitoffset += 6;
		sym->value2 = (code & 3);
		sym->value1 = (code >> 2);

		if (!sym->value1 && sym->value2 == 3)
		{
			// #c = 0, #t1 = 3 => #c = 0
			sym->value2 = 0;
		}
		else
		{
			sym->value1++;
		}
	}
	else
	{
		//retval = code_from_bitstream_2d
		retval = code_from_bitstream_2d(sym, currStream, lentab[vlcnum][0], codtab[vlcnum][0], 17, 4, &code);
		if (retval)
		{
			cout << "ERROR: failed to find numcoeff/trailingones" << endl;
			exit(-1);
		}
	}
	return retval; 
}