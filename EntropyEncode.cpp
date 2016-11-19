#include "EntropyEncode.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

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

	unsigned char *signOnes = new unsigned char[2];
	unsigned char *signOneStorage = new unsigned char[(iWidth / 4) * (iHeight / 4) * 3]; //storage array for signs of 1 similar to coeffTokens3D

	int *nonZeroCoefficients = new int[16]();
	int *nonZeroCoefficientsStorage = new int[(iWidth / 4) * (iHeight / 4) * 16];
	int coeffCounter2 = 0;

	int *totalZeros = new int[(iWidth / 4) * (iHeight / 4)];
	int coeffCounter3 = 0;

	int *zeroLeft = new int[(iWidth / 4) * (iHeight / 4) * 16]();
	int *runBefore = new int[(iWidth / 4) * (iHeight / 4) * 16]();

	//for each 4x4 block, call a zigzag
	//starting top left corner of each 4x4
	int xStart1 = 0;
	int xStart2 = 0;

	SyntaxElement  se;
	Bitstream outputStream;
	outputStream.write_flag = 0;

	for (int i = 0; i < (iHeight / 4); i++)
	{
		xStart1 = i * iHeight * 4;
		for (int j = 0; j < (iWidth / 4); j++)
		{
			xStart2 = xStart1 + j * 4; //Starting location for top left of each 4x4 dct block
			//scannedBlock = zScan(pDCTBuf, xStart2, iWidth);
			scannedBlock[0] = 0;;
			scannedBlock[1] = 3;
			scannedBlock[2] = 0;
			scannedBlock[3] = 1;
			scannedBlock[4] = -1;
			scannedBlock[5] = -1;
			scannedBlock[6] = 0;
			scannedBlock[7] = 1;
			scannedBlock[8] = 0;
			scannedBlock[9] = 0;
			scannedBlock[10] = 0;
			scannedBlock[11] = 0;
			scannedBlock[12] = 0;
			scannedBlock[13] = 0;
			scannedBlock[14] = 0;
			scannedBlock[15] = 0;

			//count coeff tokens
			coeffTokens = countCoeffToken(scannedBlock);
			//store the coeff tokens in 3d array
			coeffTokens3D[coeffCounter] = xStart2;
			coeffTokens3D[coeffCounter + 1] = coeffTokens[0];
			coeffTokens3D[coeffCounter + 2] = coeffTokens[1];
			
			//Encode the signs of trailing 1s in reverse order, 0 for positive, 1 for negative
			signOnes = signTrailOnes(scannedBlock);
			//store the signs in a 3d array
			signOneStorage[coeffCounter] = signOnes[0];
			signOneStorage[coeffCounter + 1] = signOnes[1];
			signOneStorage[coeffCounter + 2] = signOnes[2];

			coeffCounter += 3;

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

			coeffCounter2 += 16;

			coeffCounter3++;
		}
	}

	//Adaptive coding of numtrail
	//Obtain the average TC
	for (int i = 0; i < ((iWidth / 4) * (iHeight / 4) * 3); i = i + 3)
	{
		//pass in the  starting location and TC into function to return the average N
		averageN = adaptiveNumtrail(i, coeffTokens3D, iWidth,  iHeight);
	}

	se.value1 = coeffTokens3D[1];
	se.value2 = coeffTokens3D[2];
	se.len = 0;

	writeSyntaxElement_NumCoeffTrailingOnes(&se, &outputStream);




	cout << "FUCK U" << endl;

	return bitStream;
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

	bitstream->write_flag = 1;

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

	for (int i = 15; i >= 0; i--)
	{
		if (scannedArray[i] != 0) //if the value is a non-zero coefficient
		{
			if (abs(scannedArray[i]) == 1 && oneCounter > 0)
			{
				//ignore this and subtract oneCounter
				oneCounter--;
			}
			else
			{
				tempArray[arrayCounter] = scannedArray[i]; 
				arrayCounter++;
			}
		}
	}

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
		}
	}

	for (int k = 0; k < 15; k++)
	{
		if (j > 0)
		{
			orderedArray[k] = tempArray[j - 1];
			j--;
		}
	}


	return orderedArray;
}

unsigned char * EntropyEncode::signTrailOnes(float *scannedArray)
{
	unsigned char tempArray[] = { 2, 2, 2 }; 
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
		//top row cases, not adaptive needed, send back VLC0 case
		averageN = 0;
		return averageN;
	}
	else if (xStart % leftColHeight == 0)
	{
		//left most case, no adaptive needed, send back VLC0 case
		averageN = 0;
		return averageN;
	}
	else
	{
		leftN = coeffTokens3D[xStart - 2];
		topN = coeffTokens3D[xStart - leftColHeight + 1];

		averageN = (leftN + topN) / 2;
		return averageN;
	}
}

float * EntropyEncode::zScan(float *pDCTBuf, int xStart, int iWidth)
{
	float *scannedArray;
	scannedArray = new float[16];

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
	int *coeffTokens;
	coeffTokens = new int[2]; //0 stores TC, 1 stores T1s
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