/*********************************************************************************
 *
 * Decoder of 4-point floating-point DCT. The floating-point DCT coefficients are saved directly (4 bytes each).
 * No compression is applied yet.
 * 
 * The codec was originally written by Dr. Chengjie Tu.
 *
 * Modified by Jie Liang
 * School of Engineering Science
 * Simon Fraser University
 *
 * Ver 1, Feb. 2005
 * Ver 2, Sept. 2012
 * Ver 3: Sept. 2013
 *
 * Modified by Benny CHou	-	Nov. 2016
 * -implemented CAVLC decoding
 *********************************************************************************/
#include <iostream>
#include <fstream>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "codeclib.h"

#include "EntropyDecode.h"

void usage() {
    cout << "Image decoder with inverse quantizer, inverse DCT:" << endl;;
    cout << "Usage: acdec infile outfile.pgm" << endl;
    cout << "    infile:      Input file." << endl;
    cout << "    outfile.pgm: Output PGM file." << endl;
}

int main(int argc,char **argv) {
 
    ifstream ifsInfile;
    ofstream ofsOutfile;
    int iWidth, iHeight;
    unsigned char *pcImgBuf;
    float *pDCTBuf;
	unsigned char *pDCTBuf2;
    float fQstep;
	int iQMtd;

    if(argc < 3) {
        usage();
        return -1;
    }

    //open input file
    ifsInfile.open(argv[1], ios::in|ios::binary);
    if(!ifsInfile)
    {
        cout << "Can't open file " << argv[1] << endl;
        return -2;
    }

    //open output file
    ofsOutfile.open(argv[2], ios::out|ios::binary);
    if(!ofsOutfile)
    {
        cout << "Can't open file " << argv[2] << endl;
        return -3;
    }

    //read width, height
	ifsInfile.read((char *)&iWidth, sizeof(int));
	ifsInfile.read((char *)&iHeight, sizeof(int));
	ifsInfile.read((char *)&fQstep, sizeof(float));
	ifsInfile.read((char *)&iQMtd, sizeof(int));

	if (iQMtd == 0) {
		cout << "Deadzone quantizer is used." << endl;
	} else {
		cout << "Midtread quantizer is used." << endl;
	}

    int iImageArea = iWidth * iHeight;
    pcImgBuf = new unsigned char[iImageArea];
    if (!pcImgBuf) {
        cout << "Fail to create image buffer." << endl;
        return -5;
    }

    IDecoder *pDecoder = new IDecoder(iWidth, iHeight);

    pDCTBuf = new float[iImageArea];
	pDCTBuf2 = new unsigned char[43009];
    if (!pDCTBuf) {
        cout << "Fail to create DCT buffer." << endl;
        return -7;
    }
	ifsInfile.read((char *)pDCTBuf2, 43009);
    //ifsInfile.read((char *)pDCTBuf, iImageArea * sizeof(float));

	EntropyDecode *test = new EntropyDecode();
	test->decodeVLC(pDCTBuf, pDCTBuf2, iWidth, iHeight, fQstep, iQMtd);

	/* INCOMPLETE CODE*/
	// decodeVLC should return pDCTBuf with the relevant decoded coefficients arranged row by row
	// pDCTBuf is then passed back into decodeImage and GetImage from base codec to re-obtain dequantized and transformed data
	// Also missing function to detect whether decoding is necessary based on input .out file

    // Decode image
    //pDecoder->decodeImage(pDCTBuf, fQstep, iQMtd);

    // Copy to output buf, add 128
    //pDecoder->GetImage(pcImgBuf);
    
    //Output PGM header
    //ofsOutfile << "P5\n" << iWidth << " " << iHeight << endl;
    //ofsOutfile << "255" << endl;
    //ofsOutfile.write((const char *)pcImgBuf, iImageArea);
    //ifsInfile.close();
    //ofsOutfile.close();

    delete pDecoder;
    delete pcImgBuf;
    //delete pDCTBuf; //buf has been freed with pDecoder.

    return 0;
}