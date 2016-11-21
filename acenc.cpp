/*********************************************************************************
 *
 * 4-point floating-point DCT. The floating-point DCT coefficients are saved directly (4 bytes each).
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
 *
 *********************************************************************************/

#include <iostream>
#include <fstream>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "codeclib.h"
#include "EntropyEncode.h"
#include <math.h>

void usage() {
    cout << "Image encoder with DCT, quantization:" << endl;;
    cout << "Usage: acenc infile.pgm outfile qstep qmtd" << endl;
    cout << "    infile.pgm: Input PGM image." << endl;
    cout << "    outfile:    Output file. The floating-point DCT coefficients are saved directly. No compression yet." << endl;
    cout << "    qstep:      Quantization step size (can be floating-point)" << endl;
    cout << "    qmtd:       Quantization method. 0 for deadzone quantizer (default). 1 for midtread quantizer. " << endl;
	cout << "			     If qmtd is not specified, it will be treated as 0." << endl;
	cout << "	 encType;	 Type of entropy coding used. 0 is none. 1 is Huffman. 2 is CAVLC" << endl;
}



//--------------------------------------------------------
//read PGM header
//Return width and height.
//File pointer is moved to the first data after the header.
// ifstream should be passed as reference.
//--------------------------------------------------------
int ReadPGMHeader(ifstream &ifsInfile, int *piWidth, int *piHeight) 
{
    char buf[100];
	streamsize lth;
	char * pEnd;
	int tmp = 0;

    while(ifsInfile.peek()=='#')
        ifsInfile.getline(buf,100);

    ifsInfile.getline(buf,100);
    if((buf[0]!='P')||(buf[1]!='5'))
    {
        cout<<"Invalid pgm file (code 1)" <<endl;
        return -10;
    }

	lth = ifsInfile.gcount();

	if (lth == 3) { // new line after P5
		while(ifsInfile.peek()=='#')
			ifsInfile.getline(buf,100);

		ifsInfile.getline(buf,100,' ');
		*piWidth = atoi(buf);
		ifsInfile.getline(buf,100);
		*piHeight = atoi(buf);

		while(ifsInfile.peek()=='#')
			ifsInfile.getline(buf,100);

		//read max pixel value line
		ifsInfile.getline(buf,100);

	} 
	else { // P5 512 512 255 etc are in the same line, or P5 255 255 in the same line.
		*piWidth = strtol (buf + 2, &pEnd, 10);
		*piHeight = strtol (pEnd, &pEnd, 10);

		tmp = strtol (pEnd, &pEnd, 10);

		if (tmp == 0) {
			//read max pixel value line
			ifsInfile.getline(buf,100);
		}
    }

	if (!*piWidth || !*piHeight) {
		cout <<"Invalid pgm format (code 2)" << endl;
		return -20;
	}

	if (*piWidth % 16 || *piHeight % 16) {
		cout << "Image size must be multiple of 16." << endl;
		return -30;
	}

    return 0;
}


//--------------------------------------------------------
//
// main routine
//
//--------------------------------------------------------
int main(int argc, char **argv) {

	ifstream ifsInfile;
	ofstream ofsOutfile;
	int iWidth, iHeight;
	unsigned char *pcImgBuf;
	float *pDCTBuf;
	float fQstep;
	int iQMtd = 0; //default DZone quantizer
	int entType = 0; //default no entropy coding
	unsigned char *bitOutBuf;

	if (argc < 6) {
		usage();
		return -1;
	}

	//open input file
	ifsInfile.open(argv[1], ios::in | ios::binary);
	if (!ifsInfile)
	{
		cout << "Can't open file " << argv[1] << endl;
		return -2;
	}

	//open output file
	ofsOutfile.open(argv[2], ios::out | ios::binary);
	if (!ofsOutfile)
	{
		cout << "Can't open file " << argv[2] << endl;
		return -3;
	}

	fQstep = (float)atof(argv[3]);

	if (argc == 5) {
		iQMtd = atoi(argv[4]);
	}

	if (iQMtd == 0) {
		cout << "Deadzone quantizer is used." << endl;
	}
	else {
		cout << "Midtread quantizer is used." << endl;
	}

	if (argc == 6)
	{
		entType = atoi(argv[5]);
	}

    if (ReadPGMHeader(ifsInfile, &iWidth, &iHeight)) {
        return -4;
    }

    int iImageArea = iWidth * iHeight;
    pcImgBuf = new unsigned char[iImageArea];
    if (!pcImgBuf) {
        cout << "Fail to create image buffer." << endl;
        return -5;
    }

    IEncoder *pEncoder = new IEncoder(iWidth, iHeight);

    //read image
    ifsInfile.read((char *)pcImgBuf, iImageArea);
    pEncoder->SetImage(pcImgBuf);	//subtract 128

    //encode the image
    pDCTBuf = pEncoder->codeImage(fQstep, iQMtd);

	//Entropy

	if (entType == 0)
	{
		cout << "No entropy coding is used." << endl;
	}
	else if (entType == 1)
	{
		cout << "Huffman entropy coding is used." << endl;
		//calculate probabilityes for huffman
		float prob[256];
		int loc[256];
		int i;
		unsigned int *code;
		char *length;

		for (i = 0; i < 256; i++) {
			prob[i] = 0;
		}

		for (i = 0; i < iImageArea; i++)
		{
			prob[pcImgBuf[i]]++;
		}

		cout << "Probabilities:" << endl;
		for (i = 0; i < 256; i++)
		{
			prob[i] = prob[i] / iImageArea;
			cout << prob[i] << endl;
		}

		//Huffman codeword and length buffers
		code = new unsigned int[sizeof(unsigned int)];
		length = new char[sizeof(char)];

		for (i = 0; i < 256; i++)
		{
			code[i] = 0;
			length[i] = 0;
		}

		//generate Huffman codewords
		//sort(prob, loc, 256);
		//huff(prob, loc, 256, code, length);
	}
	else
	{
		cout << "CAVLC entropy coding is used" << endl;

		EntropyEncode *savage = new EntropyEncode();
		bitOutBuf = savage->encodeVLC(pDCTBuf, iWidth, iHeight);

	}

    //write output file with a simple header
    ofsOutfile.write((const char *) &iWidth, sizeof(int));
    ofsOutfile.write((const char *) &iHeight, sizeof(int));
    ofsOutfile.write((const char *) &fQstep, sizeof(float));
    ofsOutfile.write((const char *) &iQMtd, sizeof(int));

    // output DCT coeffs directly w/o compression: row by row. Matlab read col by col. Should transpose.
    ofsOutfile.write((const char *) pDCTBuf, iImageArea * sizeof(float));  

    ifsInfile.close();
    ofsOutfile.close();

    delete pEncoder;
    delete pcImgBuf;

    return 0;
}