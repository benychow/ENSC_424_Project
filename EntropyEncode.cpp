#include "EntropyEncode.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define MAXRTPPAYLOADLEN  (65536 - 40)  

static inline int imin(int a, int b)
{
	return ((a) < (b) ? (a) : (b));
}

EntropyEncode::EntropyEncode(void)
{
}

EntropyEncode::~EntropyEncode(void)
{
}

unsigned char * EntropyEncode::encodeVLC(float *pDCTBuf, int iWidth, int iHeight)
{
	unsigned char *bitStream;	//will be the output bitstream
	bitStream = new unsigned char[2];
	int *coeffTokens = new int[2];
	float *scannedBlock;
	scannedBlock = new float[16];

	int *coeffTokens3D = new int[(iWidth / 4) * (iHeight / 4) * 3];
	int coeffCounter = 0;

	int averageN;

	int *signOnes = new int[2];
	int *signOnesStorage = new int[(iWidth / 4) * (iHeight / 4) * 2];
	int coeffCounter4 = 0;

	int *nonZeroCoefficients = new int[16]();
	int *nonZeroCoefficientsStorage = new int[(iWidth / 4) * (iHeight / 4) * 16];
	int coeffCounter2 = 0;

	int *totalZeros = new int[(iWidth / 4) * (iHeight / 4)];
	int coeffCounter3 = 0;

	int *zeroLeft = new int[(iWidth / 4) * (iHeight / 4) * 16]();
	int *runBefore = new int[(iWidth / 4) * (iHeight / 4) * 16]();

	int level_two_or_higher, vlcnum;
	static const int incVlc[] = { 0, 3, 6, 12, 24, 48, 32768 };  // maximum vlc = 6

	int run, zerosleft, numcoef;

	//for each 4x4 block, call a zigzag
	//starting top left corner of each 4x4
	int xStart1 = 0;
	int xStart2 = 0;

	SyntaxElement  se;
	Bitstream outputStream;
	//initialize bitstream parameters
	outputStream.byte_buf = 0;
	outputStream.bits_to_go = 8;
	outputStream.byte_pos = 0;
	outputStream.streamBuffer = new byte[MAXRTPPAYLOADLEN];
	outputStream.write_flag = 0;

	for (int i = 0; i < (iHeight / 4); i++)
	{
		xStart1 = i * iHeight * 4;
		for (int j = 0; j < (iWidth / 4); j++)
		{
			xStart2 = xStart1 + j * 4; //Starting location for top left of each 4x4 dct block
<<<<<<< HEAD

			scannedBlock = zScan(pDCTBuf, xStart2, iWidth);
			
			/*
			scannedBlock[0] = -2;
			scannedBlock[1] = 4;
			scannedBlock[2] = 3;
			scannedBlock[3] = -3;
			scannedBlock[4] = 0;
			scannedBlock[5] = 0;
			scannedBlock[6] = -1;
			scannedBlock[7] = 0;
=======
			scannedBlock = zScan(pDCTBuf, xStart2, iWidth);
			
			/*
<<<<<<< HEAD
			scannedBlock[0] = 0;
=======
			scannedBlock[0] = 0;;
>>>>>>> parent of 8e4adce... Reverted to last working copy
			scannedBlock[1] = 3;
			scannedBlock[2] = 0;
			scannedBlock[3] = 1;
			scannedBlock[4] = -1;
			scannedBlock[5] = -1;
			scannedBlock[6] = 0;
			scannedBlock[7] = 1;
>>>>>>> origin/master
			scannedBlock[8] = 0;
			scannedBlock[9] = 0;
			scannedBlock[10] = 0;
			scannedBlock[11] = 0;
			scannedBlock[12] = 0;
			scannedBlock[13] = 0;
			scannedBlock[14] = 0;
			scannedBlock[15] = 0;
			*/
<<<<<<< HEAD
			
=======
<<<<<<< HEAD
			
			
=======
>>>>>>> parent of 8e4adce... Reverted to last working copy

>>>>>>> origin/master
			//count coeff tokens
			coeffTokens = countCoeffToken(scannedBlock);
			//store the coeff tokens in 3d array
			coeffTokens3D[coeffCounter] = xStart2;
			coeffTokens3D[coeffCounter + 1] = coeffTokens[0];
			coeffTokens3D[coeffCounter + 2] = coeffTokens[1];
			
			//Encode the signs of trailing 1s in reverse order, 0 for positive, 1 for negative
			signOnes = signTrailOnes(scannedBlock);
			signOnesStorage[coeffCounter4] = signOnes[0]; //codeowrd
			signOnesStorage[coeffCounter4 + 1] = signOnes[1]; //length
			

			//scan for remaining non-zero coefficients not part of num trail
			nonZeroCoefficients = reverseLevels(scannedBlock);
			for (int k = 0; k < 16; k++)
			{
				nonZeroCoefficientsStorage[coeffCounter2 + k] = nonZeroCoefficients[k];

			}

			//store total zeros
			totalZeros[coeffCounter3] = countZeros(scannedBlock);
			//this coding of run function is missing the special case, intended to be adding in the encoding part
			//i.e. if the number of coefficients if 0, then the last zeroLeft and runBefore will not be encoded
			codingOfRuns(countZeros(scannedBlock), zeroLeft, runBefore, scannedBlock, coeffCounter2);

			coeffCounter += 3;
			coeffCounter2 += 16;
			coeffCounter3++;
			coeffCounter4 += 2;
		}
	}

	coeffCounter = 0; // + 3
	coeffCounter2 = 0; // +2
	coeffCounter3 = 0; // +16

	for (int z = 0; z < (iWidth / 4) * (iHeight / 4); z++)
	{
		//Adaptive coding of numtrail by p assing in the starting location and TC array
		//returns average N
		averageN = adaptiveNumtrail(coeffCounter, coeffTokens3D, iWidth, iHeight);

		//step 1 coeff num and trail 1 encode
		se.value1 = coeffTokens3D[coeffCounter + 1];
		se.value2 = coeffTokens3D[coeffCounter + 2];
		se.len = averageN;

		writeSyntaxElement_NumCoeffTrailingOnes(&se, &outputStream);

		if (coeffTokens3D[coeffCounter + 1])
		{
			//step 2 sign of trail 1 encode
			se.value1 = signOnesStorage[coeffCounter2];
			se.value2 = signOnesStorage[coeffCounter2 + 1];

			writeSyntaxElement_VLC(&se, &outputStream);

			//step 3 encode the remaining non-zero coeffs in reverse order
			//decide which vlc to use
			level_two_or_higher = (coeffTokens3D[coeffCounter + 1] > 3 && coeffTokens3D[coeffCounter + 2] == 3) ? 0 : 1;
			vlcnum = (coeffTokens3D[coeffCounter + 1] > 10 && coeffTokens3D[coeffCounter + 2] < 3) ? 1 : 0;

			for (int k = 15; k >= 0; k--)
			{
				if (nonZeroCoefficientsStorage[coeffCounter3 + k] != 0)
				{
					se.value1 = nonZeroCoefficientsStorage[coeffCounter3 + k];
					// encode level
					if (vlcnum == 0)
					{
						writeSyntaxElement_level_VLC1(&se, &outputStream);
					}
					else
					{
						writeSyntaxElement_level_VLCN(&se, vlcnum, &outputStream);
					}

					//update VLC table

					if (abs(nonZeroCoefficientsStorage[coeffCounter3 + k]) > incVlc[vlcnum])
					{
						vlcnum++;
					}
				}
			}

			//step 4 coding of total zeros
			if (coeffTokens3D[coeffCounter + 1] < 16)
			{
				se.value1 = totalZeros[z];

				vlcnum = coeffTokens3D[coeffCounter + 1] - 1;

				se.len = vlcnum;

				writeSyntaxElement_TotalZeros(&se, &outputStream);
			}

			//step 5 encoding run before each coefficient
			zerosleft = totalZeros[z];
			numcoef = coeffTokens3D[coeffCounter + 1];
			for (int l = 0; l < 16; l++)
			{
				run = runBefore[l];

				se.value1 = run;

				// for last coeff, run is remaining totzeros
				// when zerosleft is zero, remaining coeffs have 0 run
				if ((!zerosleft) || (coeffTokens3D[coeffCounter + 1] <= 1))
					break;
				if (numcoef > 1 && zerosleft)
				{
					vlcnum = imin(zerosleft - 1, 6);
					se.len = vlcnum;

					writeSyntaxElement_Run(&se, &outputStream);

					zerosleft -= run;
					numcoef--;
				}
			}
		}
		coeffCounter += 3;
		coeffCounter2 += 2;
		coeffCounter3 += 16;
	}

	writeVlcByteAlign(&outputStream);

<<<<<<< HEAD
	*sizeVLCBuf = outputStream.byte_pos;

<<<<<<< HEAD
=======
	delete[] coeffTokens;
	delete[] coeffTokens3D;
	delete[] signOnes;
	delete[] signOnesStorage;
	delete[] nonZeroCoefficients;
	delete[] nonZeroCoefficientsStorage;
	delete[] totalZeros;
	delete[] zeroLeft;
	delete[] runBefore;

>>>>>>> origin/master
	return outputStream.streamBuffer;
=======
	cout << "END TEST" << endl;

	return bitStream;
>>>>>>> parent of 8e4adce... Reverted to last working copy
}

// pads the rest of the bitstream with 0 for the current byte
void EntropyEncode::writeVlcByteAlign(Bitstream *currStream)
{
	if (currStream->bits_to_go < 8)
	{
		currStream->byte_buf = (byte)((currStream->byte_buf << currStream->bits_to_go) | (0xff >> (8 - currStream->bits_to_go)));
		currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
		currStream->bits_to_go = 8;
	}
}

int EntropyEncode::writeSyntaxElement_Run(SyntaxElement *se, Bitstream *bitstream)
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

	int vlcnum = se->len;

	// se->value1 : run
	se->len = lentab[vlcnum][se->value1];
	se->inf = codtab[vlcnum][se->value1];

	if (se->len == 0)
	{
		cout << "ERROR: (run) not valid" << endl;
		exit(-1);
	}

	symbol2vlc(se);

	writeUVLC2buffer(se, bitstream);

	return (se->len);
}

int EntropyEncode::writeSyntaxElement_TotalZeros(SyntaxElement *se, Bitstream *bitstream)
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
	int vlcnum = se->len;

	// se->value1 : TotalZeros
	se->len = lentab[vlcnum][se->value1];
	se->inf = codtab[vlcnum][se->value1];

	if (se->len == 0)
	{
		cout << "ERROR: (TotalZeros) not valid" << endl;
		exit(-1);
	}

	symbol2vlc(se);

	writeUVLC2buffer(se, bitstream);

	return (se->len);
}

int EntropyEncode::writeSyntaxElement_level_VLCN(SyntaxElement *se, int vlc, Bitstream *bitstream)
{
	int level = se->value1;
	int sign = (level < 0 ? 1 : 0);
	int levabs = abs(level) - 1;

	int shift = vlc - 1;
	int escape = (15 << shift);

	if (levabs < escape)
	{
		int sufmask = ~((0xffffffff) << shift);
		int suffix = (levabs)& sufmask;

		se->len = ((levabs) >> shift) + 1 + vlc;
		se->inf = (2 << shift) | (suffix << 1) | sign;
	}
	else
	{
		int iMask = 4096;
		int levabsesc = levabs - escape + 2048;
		int numPrefix = 0;

		if ((levabsesc) >= 4096)
		{
			numPrefix++;
			while ((levabsesc) >= (4096 << numPrefix))
			{
				numPrefix++;
			}
		}

		iMask <<= numPrefix;
		se->inf = iMask | ((levabsesc << 1) - iMask) | sign;

		se->len = 28 + (numPrefix << 1);
	}

	symbol2vlc(se);

	writeUVLC2buffer(se, bitstream);

	return (se->len);
}

int EntropyEncode::writeSyntaxElement_level_VLC1(SyntaxElement *se, Bitstream *bitstream)
{
	int level = se->value1;
	int sign = (level < 0 ? 1 : 0);
	int levabs = abs(level);

	if (levabs < 8)
	{
		se->len = levabs * 2 + sign - 1;
		se->inf = 1;
	}
	else if (levabs < 16)
	{
		// escape code1
		se->len = 19;
		se->inf = 16 | ((levabs << 1) - 16) | sign;
	}
	else
	{
		int iMask = 4096, numPrefix = 0;
		int  levabsm16 = levabs + 2032;

		// escape code2
		if ((levabsm16) >= 4096)
		{
			numPrefix++;
			while ((levabsm16) >= (4096 << numPrefix))
			{
				numPrefix++;
			}
		}

		iMask <<= numPrefix;
		se->inf = iMask | ((levabsm16 << 1) - iMask) | sign;

		se->len = 28 + (numPrefix << 1);
	}

	symbol2vlc(se);

	writeUVLC2buffer(se, bitstream);

	return (se->len);
}

int EntropyEncode::writeSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *se, Bitstream *bitstream)
{
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

	int vlcnum = se->len;

	// se->value1 : numcoeff
	// se->value2 : numtrailingones

	if (vlcnum == 3) //FLC case
	{
		se->len = 6;
		if (se->value1 > 0)
		{
			se->inf = ((se->value1 - 1) << 2) | se->value2;
		}
		else
		{
			se->inf = 3;
		}
	}
	else
	{
		se->len = lentab[vlcnum][se->value2][se->value1];
		se->inf = codtab[vlcnum][se->value2][se->value1];
	}

	if (se->len == 0)
	{
		cout << "ERROR: (numcoeff, trailingones) not valid: vlc = " << vlcnum <<
			" (" << se->value1 << ", " << se->value2 << ")" << endl;
	}

	symbol2vlc(se);

	writeUVLC2buffer(se, bitstream);

	//bitstream->write_flag = 1;

	return (se->len);
}

int EntropyEncode::writeSyntaxElement_VLC(SyntaxElement *se, Bitstream *bitstream)
{
	se->inf = se->value1;
	se->len = se->value2;
	symbol2vlc(se);

	writeUVLC2buffer(se, bitstream);

	//bitstream->write_flag = 1;

	return (se->len);
}

void EntropyEncode::writeUVLC2buffer(SyntaxElement *se, Bitstream *currStream)
{
	unsigned int mask = 1 << (se->len - 1);
	byte *byte_buf = &currStream->byte_buf;
	int *bits_to_go = &currStream->bits_to_go;
	int i;

	// Add the new bits to the bitstream.
	// Write out a byte if it is full
	if (se->len < 33)
	{
		for (i = 0; i < se->len; i++)
		{
			*byte_buf <<= 1;

			if (se->bitpattern & mask)
				*byte_buf |= 1;

			mask >>= 1;

			if ((--(*bits_to_go)) == 0)
			{
				*bits_to_go = 8;
				currStream->streamBuffer[currStream->byte_pos++] = *byte_buf;
				*byte_buf = 0;
			}
		}
	}
	else
	{
		// zeros
		for (i = 0; i < (se->len - 32); i++)
		{
			*byte_buf <<= 1;

			if ((--(*bits_to_go)) == 0)
			{
				*bits_to_go = 8;
				currStream->streamBuffer[currStream->byte_pos++] = *byte_buf;
				*byte_buf = 0;
			}
		}
		// actual info
		mask = (unsigned int)1 << 31;
		for (i = 0; i < 32; i++)
		{
			*byte_buf <<= 1;

			if (se->bitpattern & mask)
				*byte_buf |= 1;

			mask >>= 1;

			if ((--(*bits_to_go)) == 0)
			{
				*bits_to_go = 8;
				currStream->streamBuffer[currStream->byte_pos++] = *byte_buf;
				*byte_buf = 0;
			}
		}
	}
}

int EntropyEncode::symbol2vlc(SyntaxElement *sym)
{
	int info_len = sym->len;

	//convert info into a bitpattern
	sym->bitpattern = 0;

	//vlc coding
	while (--info_len >= 0)
	{
		sym->bitpattern <<= 1;
		sym->bitpattern |= (0x01 & (sym->inf >> info_len));
	}

	return 0;
}

void EntropyEncode::codingOfRuns(int countZeros, int *zeroLeft, int *runBefore, float *scannedArray, int coeffCounter2)
{
	int zeroCounter = countZeros;
	int runCounter = 0;
	int futureCounter = 1;
	int returnIndexRunCounter = 0;
	int returnIndexZeroCounter = 0;
	bool relevant = false;

	zeroLeft[coeffCounter2 + returnIndexZeroCounter] = countZeros;
	returnIndexZeroCounter++;

	for (int i = 15; i >= 0; i--)
	{
		if (scannedArray[i] != 0)
		{
			relevant = true;
			while (relevant)
			{
				if (scannedArray[i - futureCounter] == 0)
				{
					runCounter++;
					futureCounter++;
				}
				else if (scannedArray[i - futureCounter] != 0)
				{
					relevant = false;
					futureCounter = 1;

					runBefore[coeffCounter2 + returnIndexRunCounter] = runCounter;
					returnIndexRunCounter++;

					zeroCounter -= runCounter;

					zeroLeft[coeffCounter2 + returnIndexZeroCounter] = zeroCounter;
					returnIndexZeroCounter++;

					runCounter = 0;
				}
			}
		}



	}
}

int EntropyEncode::countZeros(float *scannedArray)
{
	int totalZeros = 0;
	bool relevant = false;

	for (int i = 15; i >= 0; i--)
	{
		if (scannedArray[i] != 0)
		{
			relevant = true;
		}
		else if (scannedArray[i] == 0 && relevant)
		{
			totalZeros++;
		}
	}

	return totalZeros;
}

int * EntropyEncode::reverseLevels(float *scannedArray)
{
	int tempArray[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int orderedArray[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int oneCounter = 3; //if the one is part of the trailing 1s, do not include
	int arrayCounter = 0;

	int t1Counter = 0;
	int t1Location = 0;
	int coefLocation = 0;

	for (int e = 0; e < 16; e++)
	{
		tempArray[e] = scannedArray[e];
	}
<<<<<<< HEAD
	//if no 1s abort
	if (oneCounter == 3)
	{
		return tempArray;
	}
	else
	{
		//special case
		bool lastNonZero = false;
		int j = 0;
		while (!lastNonZero)
		{
			if (tempArray[j] == 0)
			{
				lastNonZero = true;
			}
			else
			{
				j++;
=======

	//find special case first
	//first count # of trailing ones
	for (int i = 0; i < 16; i++)
	{
		if (abs(tempArray[i]) == 1)
			t1Counter++;
	}

	//if # trailing 1 is less than 3, next non-zero coeff must be >1 or < -1
	if (t1Counter < 3)
	{
		//find where t he  1 is located
		for (int j = 0; j < 16; j++)
		{
			if (abs(tempArray[j]) == 1)
			{
				t1Location = j;
				j = 16;
			}
		}

		//location of coeff before t1Location
		for (int k = t1Location; k >= 0; k--)
		{
			if (tempArray[k] != 0 && abs(tempArray[k]) != 1)
			{
				if (tempArray[k] > 0)
				{
					tempArray[k] = tempArray[k] - 1;
				}
				else if (tempArray[k] < 0)
				{
					tempArray[k] = tempArray[k] + 1;
				}
>>>>>>> origin/master
			}
		}

<<<<<<< HEAD
		for (int k = 0; k < 15; k++)
		{
			if (j > 0)
			{
				orderedArray[k] = tempArray[j - 1];
				j--;
=======
	for (int i = 15; i >= 0; i--)
	{
		if (tempArray[i] != 0) //if the value is a non-zero coefficient
		{
			if (abs(tempArray[i]) == 1 && oneCounter > 0)
			{
				//ignore this and subtract oneCounter
				oneCounter--;
			}
			else
			{
				orderedArray[arrayCounter] = tempArray[i];
				arrayCounter++;
>>>>>>> origin/master
			}
		}
	}

	return orderedArray;
}

int * EntropyEncode::signTrailOnes(float *scannedArray)
{
	int tempArray[] = { 2, 2, 2 }; 
	int oneCounter = 0;

	for (int i = 15; i >= 0; i--)
	{
		if (scannedArray[i] != 0) //scanning non-zero coefficients
		{
			if (oneCounter < 3)
			{
				if (scannedArray[i] == 1)
				{
					//if scanned in reverse order zig zag is 1, 0 for positive
					tempArray[oneCounter] = 0;
					oneCounter++;
				}
				else if (scannedArray[i] == -1)
				{
					tempArray[oneCounter] = 1;
					oneCounter++;
				}
				else if (abs(scannedArray[i]) != 1) //special case
				{
					if (scannedArray[i] > 0 && tempArray[0] != 2)
					{
						//greater than 0 coded as level - 1
						scannedArray[i] = scannedArray[i] - 1;
						oneCounter = 3;
					}
					else if (scannedArray[i] < 0 && tempArray[0] != 2)
					{
						scannedArray[i] = scannedArray[i] + 1;
						oneCounter = 3;
					}
				}
			}
		}
	}
	//reading a 2 will mean EOF

	int len = 0;
	int code = 0;

	if (tempArray[0] != 2)
	{
		len++;
		if (tempArray[0] == 1)
		{
			code += 4;
		}
	}

	if (tempArray[1] != 2)
	{
		len++;
		if (tempArray[1] == 1)
		{
			code += 2;
		}
	}

	if (tempArray[2] != 2)
	{
		len++;
		if (tempArray[2] == 1)
		{
			code += 1;
		}
	}

	tempArray[0] = code;
	tempArray[1] = len;

	return tempArray;

}

int EntropyEncode::adaptiveNumtrail(int xStart, int *coeffTokens3D, int iWidth, int iHeight)
{
	int averageN;
	int topN;
	int leftN;

	int topRowLength = (iWidth / 4) * 3 - 3;
	int leftColHeight = (iHeight / 4) * 3;

	if (xStart <= topRowLength)
	{
		//top row cases, adaptive is value on left
		//if first block, use vlcnum = 0
		if (xStart == 0)
		{
			averageN = 0;
			return averageN;
		}
		else
		{
			//averageN is left N
			averageN = coeffTokens3D[xStart - 2];
		}

	}
	else if (xStart % leftColHeight == 0)
	{
		//left most case, adaptive is value on top
		averageN = coeffTokens3D[xStart - leftColHeight + 1];
	}
	else
	{
		leftN = coeffTokens3D[xStart - 2];
		topN = coeffTokens3D[xStart - leftColHeight + 1];

		averageN = (leftN + topN) / 2;
	}

	if (averageN >= 0 && averageN < 2)
	{
		//use table 0
		averageN = 0;
		return averageN;
	}
	else if (averageN >= 2 && averageN < 4)
	{
		//use table 1 
		averageN = 1;
		return averageN;
	}
	else if (averageN >= 4 && averageN < 8)
	{
		//use table 2
		averageN = 2;
		return averageN;
	}
	else if (averageN >= 8)
	{
		//use FLC
		averageN = 3;
		return averageN;
	}
	else
	{
		cout << "ERROR in calculating average N" << endl;
		return 1;
	}
}

float * EntropyEncode::zScan(float *pDCTBuf, int xStart, int iWidth)
{
	float scannedArray[16];

	//perform zig zag scan
	scannedArray[0] = pDCTBuf[xStart];
	scannedArray[1] = pDCTBuf[xStart + 1];
	scannedArray[2] = pDCTBuf[xStart + iWidth];
	scannedArray[3] = pDCTBuf[xStart + (iWidth * 2)];
	scannedArray[4] = pDCTBuf[xStart + (iWidth + 1)];
	scannedArray[5] = pDCTBuf[xStart + 2];
	scannedArray[6] = pDCTBuf[xStart + 3];
	scannedArray[7] = pDCTBuf[xStart + (iWidth + 2)];
	scannedArray[8] = pDCTBuf[xStart + (iWidth * 2) + 1];
	scannedArray[9] = pDCTBuf[xStart + (iWidth * 3)];
	scannedArray[10] = pDCTBuf[xStart + (iWidth * 3) + 1];
	scannedArray[11] = pDCTBuf[xStart + (iWidth * 2) + 2];
	scannedArray[12] = pDCTBuf[xStart + (iWidth + 3)];
	scannedArray[13] = pDCTBuf[xStart + (iWidth * 2)];
	scannedArray[14] = pDCTBuf[xStart + (iWidth * 3) + 2];
	scannedArray[15] = pDCTBuf[xStart + (iWidth * 3) + 3];

	return scannedArray;
}

int * EntropyEncode::countCoeffToken(float *scannedBlock)
{
	int coeffTokens[2];
	//0 stores TC, 1 stores T1s
	int coeffCounter = 0;
	int oneCounter = 0;

	for (int i = 0; i < 16; i++)
	{
		if (abs(scannedBlock[i]) > 0)
		{
			coeffCounter++;

			if (abs(scannedBlock[i]) == 1.0)
			{
				oneCounter++;
			}
		}
	}

	if (oneCounter > 3)
	{
		oneCounter = 3;
	}
	
	coeffTokens[0] = coeffCounter;
	coeffTokens[1] = oneCounter;

	return coeffTokens;
}