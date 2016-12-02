#include "EntropyDecode.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define MAXRTPPAYLOADLEN (65536 - 40)

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

	int numcoeff, numtrailingones, numones, ntr, code, level_two_or_higher;
	int vlcnum, abslevel;
	int levarr[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	static const int incVlc[] = { 0, 3, 6, 12, 24, 48, 32768 };    // maximum vlc = 6

	//decode numcoeff and num trailing ones
	numcoeff = currSE.value1;
	numtrailingones = currSE.value2;

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
		}
	}

	//place trailing ones into temp array levarr
	for (int j = numcoeff - 1; j > numcoeff - 1 - numtrailingones; j--)
	{
		ntr--;
		levarr[j] = (code >> ntr) & 1 ? -1 : 1;
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

		if (numcoeff < 16)
		{
			//decode total run
			vlcnum = numcoeff - 1;
			currSE.value1 = vlcnum;

			//decode to zeros
		}
	}
	
	cout << "WOW AMASING" << endl;
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