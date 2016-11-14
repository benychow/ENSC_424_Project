#include <iostream>
#include <fstream>
using namespace std;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "codeclib.h"
#include "xform.h"

//-----------------------------------
// ICodec class
//-----------------------------------
ICodec::ICodec(unsigned w, unsigned h)
	:_w(w), _h(h)
{
	unsigned y;

	m_iNonzeros = 0;  //Num of nonzero quantized DCT coeffs.

					  //Image buffer: the (y, x)-th pixel of the image can be accessed  by _d[y][x].
	_d = new float *[_h];
	_d[0] = new float[_w * _h];
	for (y = 1; y < _h; y++) {
		_d[y] = _d[y - 1] + _w;
	}

}

ICodec::~ICodec(void)
{
	delete _d[0];
	delete _d;

}


//Copy image to the internal buffer 
void ICodec::SetImage(
	unsigned char *pcBuf)
{
	unsigned len = _w * _h, i;
	for (i = 0; i < len; i++)
		_d[0][i] = (float)(pcBuf[i] - 128);
}

//Read internal image to external buffer, clip to [0, 255]
void ICodec::GetImage(
	unsigned char *pcBuf)
{
	int tmp;
	unsigned len = _w * _h, i;

	for (i = 0; i < len; i++) {

		//add 128, and round to integer
		tmp = (int)(_d[0][i] + 128 + 0.5);

		//clipping to [0, 255]
		pcBuf[i] = (unsigned char)(tmp < 0 ? 0 : (tmp > 255 ? 255 : tmp));
	}
}

//Deadzone quantization: sign(x) floor(|x| / qstep)
void ICodec::QuantDZone(float *fpBlk[], int iBlkSize, float fQStep)
{
	int iIdx = 0;   //index after quantization
	int iSign = 0;  //sign of the DCT coeff
	float fAbs = 0; //abs value of the DCT coeff

	for (int i = 0; i < iBlkSize; i++) {
		for (int j = 0; j < iBlkSize; j++) {

			//----------------------
			//hw2 solution:
			if (fpBlk[i][j] >= 0) {
				iSign = 1;
				fAbs = fpBlk[i][j];
			}
			else {
				iSign = -1;
				fAbs = -fpBlk[i][j];
			}

			iIdx = (int)(fAbs / fQStep);
			fpBlk[i][j] = (float)(iSign * iIdx);
			//----------------------

			//collect num of nonzero DCT coeffs after quantization
			if (iIdx > 0) {
				m_iNonzeros++;
			}
		}
	}
}


//  inverse deadzone quantization
void ICodec::DequantDZone(float *fpBlk[], int iBlkSize, float fQStep)
{
	for (int i = 0; i < iBlkSize; i++) {
		for (int j = 0; j < iBlkSize; j++) {
			if (fpBlk[i][j] > 0) {
				fpBlk[i][j] = (fpBlk[i][j] + 0.5f) * fQStep;
			}
			else if (fpBlk[i][j] < 0) {
				fpBlk[i][j] = (fpBlk[i][j] - 0.5f) * fQStep;
			}
		}
	}
}


//mid-tread quantization: sign(x) floor(|x| / qstep + 0.5)
void ICodec::QuantMidtread(float *fpBlk[], int iBlkSize, float fQStep)
{
	int iIdx = 0;   //index after quantization
	int iSign = 0;  //sign of the DCT coeff
	float fAbs = 0; //abs value of the DCT coeff

	for (int i = 0; i < iBlkSize; i++) {
		for (int j = 0; j < iBlkSize; j++) {

			//----------------------
			//hw2 solution:
			if (fpBlk[i][j] >= 0) {
				iSign = 1;
				fAbs = fpBlk[i][j];
			}
			else {
				iSign = -1;
				fAbs = -fpBlk[i][j];
			}

			iIdx = (int)(fAbs / fQStep + 0.5);
			fpBlk[i][j] = (float)(iSign * iIdx);
			//----------------------

			//collect num of nonzero DCT coeffs after quantization
			if (iIdx > 0) {
				m_iNonzeros++;
			}
		}
	}
}

//mid-tread inverse midtread quantization
void ICodec::DequantMidtread(float *fpBlk[], int iBlkSize, float fQStep)
{
	for (int i = 0; i < iBlkSize; i++) {
		for (int j = 0; j < iBlkSize; j++) {
			fpBlk[i][j] = fpBlk[i][j] * fQStep;
		}
	}
}

//-----------------------------------
// IEncoder class
//-----------------------------------

IEncoder::IEncoder(unsigned w, unsigned h)
	:ICodec(w, h)
{
}

IEncoder::~IEncoder(void)
{
}


// Main function to encode an image
float * IEncoder::codeImage(float fQstep, int iQMtd)
{
	unsigned x, y;

	for (y = 0; y < _h; y += BLOCKSIZE) {
		for (x = 0; x < _w; x += BLOCKSIZE) {
			codeBlock(y, x, fQstep, iQMtd);
		}
	}

	//display the first block
	cout << "after transform and quantization" << endl;
	cout << _d[0][0] << ", " << _d[0][1] << ", " << _d[0][2] << ", " << _d[0][3] << endl;
	cout << _d[1][0] << ", " << _d[1][1] << ", " << _d[1][2] << ", " << _d[1][3] << endl;
	cout << _d[2][0] << ", " << _d[2][1] << ", " << _d[2][2] << ", " << _d[2][3] << endl;
	cout << _d[3][0] << ", " << _d[3][1] << ", " << _d[3][2] << ", " << _d[3][3] << endl;

	return _d[0];
}

//Encode an image block
void IEncoder::codeBlock(
	unsigned y,     // y index of upper-left corner of the block
	unsigned x,     // x index of upper-left corner of the block
	float fQstep,   // quantization step
	int iQMtd)		// quantization method
{
	unsigned i;

	//update m_fBlkBuf so that it points to the current block
	for (i = 0; i < BLOCKSIZE; i++) {
		m_fBlkBuf[i] = _d[y + i] + x;
	}

	//Forward DCT
	Transform::FDCT4(m_fBlkBuf);

	//Quantization
	if (!iQMtd) {
		// iQMtd=0: deadzone quantizer
		QuantDZone(m_fBlkBuf, BLOCKSIZE, fQstep);
	}
	else {
		// Midtread quantizer
		QuantMidtread(m_fBlkBuf, BLOCKSIZE, fQstep);
	}

}


//-----------------------------------
// IDecoder class
//-----------------------------------
IDecoder::IDecoder(unsigned w, unsigned h)
	:ICodec(w, h)
{
}

IDecoder::~IDecoder(void)
{
}

// Main function to decode an image
void IDecoder::decodeImage(
	float *b,		//input buffer
	float fQstep,   //quantization step size,
	int iQMtd)		//quantization method
{
	unsigned x, y;

	//hack: initialize internal buffer
	_d[0] = b;
	for (y = 1; y < _h; y++) {
		_d[y] = _d[y - 1] + _w;
	}

	for (y = 0; y < _h; y += BLOCKSIZE) {
		for (x = 0; x < _w; x += BLOCKSIZE) {
			decodeBlock(y, x, fQstep, iQMtd);
		}
	}

}


//Decode an image block
void IDecoder::decodeBlock(
	unsigned y,     //y index of upper-left corner of the block
	unsigned x,     //x index of upper-left corner of the block
	float fQstep,   //quantization step size,
	int iQMtd)		//quantization method
{
	unsigned i;

	//update m_fBlkBuf so that it points to the current block
	for (i = 0; i < BLOCKSIZE; i++) {
		m_fBlkBuf[i] = _d[y + i] + x;
	}

	//Dequantization
	if (!iQMtd) {
		DequantDZone(m_fBlkBuf, BLOCKSIZE, fQstep);
	}
	else {
		DequantMidtread(m_fBlkBuf, BLOCKSIZE, fQstep);
	}

	//IDCT
	Transform::IDCT4(m_fBlkBuf);
}
