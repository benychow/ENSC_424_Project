#include "EntropyDecode.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define MAXRTPPAYLOADLEN (65536 - 40)

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

void EntropyDecode::decodeVLC(unsigned char *inputBuffer, int iWidth, int iHeight, float fQstep, int iQMtd)
{
	//input buffer contains our relevant bitstream
	//for now ignore quantization
	SyntaxElement currSE;
	Bitstream currStream; 

	currStream.streamBuffer = inputBuffer;
	char type[15];

	currStream.frame_bitoffset = 0;
	currStream.bitstream_length = 43009; //for this example specifically, will be passed into func later


	//predict neighbour non-zero coefficients
	//for now assume 0 because first block
	int nnz = 0;

	currSE.value1 = (nnz < 2) ? 0 : ((nnz < 4) ? 1 : ((nnz < 8) ? 2 : 3));
	readSyntaxElement_NumCoeffTrailingOnes(&currSE, &currStream, type);

	int numcoeff, numtrailingones;

	numcoeff = currSE.value1;
	numtrailingones = currSE.value2;

	cout << "WOW AMASING" << endl;
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