/*********************************************************************************
 *
 * Compute the PSNR of two PGM images
 *
 * Jie Liang
 * School of Engineering Science
 * Simon Fraser University
 *
 * Feb. 2005
 *
 *********************************************************************************/

#include <iostream>
#include <fstream>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

void usage() {
    cout << "Usage: psnr img1.pgm img2.pgm" << endl;
}

//--------------------------------------------------------
//read PGM header
//Return width and height.
//File pointer is moved to the first data after the header.
// The expected PGM header format is
// P5
// 512 512
// 255
// or P5 512 512 255
// If you have a valid PGM file but not recognized by this function, please edit its header to the format above.
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
int main(int argc,char **argv) {
 
    ifstream ifs1, ifs2;
    int iWidth1, iHeight1, iWidth2, iHeight2;
    unsigned char *pcImgBuf1, *pcImgBuf2;
    double mse = 0;
    double psnr = 0;

    if(argc < 3) {
        usage();
        return -1;
    }

    //open input file
    ifs1.open(argv[1], ios::in|ios::binary);
    ifs2.open(argv[2], ios::in|ios::binary);
    if (!ifs1 || !ifs2)
    {
        cout << "Can't open input file " << endl;
        return -2;
    }

    if (ReadPGMHeader(ifs1, &iWidth1, &iHeight1)) {
        return -4;
    }

    if (ReadPGMHeader(ifs2, &iWidth2, &iHeight2)) {
        return -5;
    }

    if (iWidth1 != iWidth2 || iHeight1 != iHeight2) {
        cout << "Image size don't match." << endl;
        return -6;
    }

    //allocate buffers
    int iImageArea = iWidth1 * iHeight1;
    pcImgBuf1 = new unsigned char[iImageArea];
    pcImgBuf2 = new unsigned char[iImageArea];
    if (!pcImgBuf1 || !pcImgBuf2) {
        cout << "Fail to create image buffer." << endl;
        return -7;
    }

    //read images
    ifs1.read((char *)pcImgBuf1, iImageArea);
    ifs2.read((char *)pcImgBuf2, iImageArea);

    //HW1: computer MSE and PSNR
    for (int i = 0; i < iImageArea; i++) {
        mse += (double) (pcImgBuf1[i] - pcImgBuf2[i]) * (pcImgBuf1[i] - pcImgBuf2[i]);
    }
    mse /= iImageArea;

    psnr = 10 * log10(255.0 * 255.0 / mse);
	//----------------------------

    cout << "MSE:  " << mse << endl;
    cout << "PSNR: " << psnr << " dB" << endl;

    ifs1.close();
    ifs2.close();

    delete pcImgBuf1;
    delete pcImgBuf2;

    return 0;
}