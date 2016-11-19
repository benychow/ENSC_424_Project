#pragma once
typedef unsigned char byte;

typedef struct syntaxelement_enc
{
	int type;
	int value1;
	int value2;
	int len;
	int inf;
	unsigned int bitpattern;
} SyntaxElement;

typedef struct bit_stream_enc
{
	int buffer_size;
	int byte_pos;
	int bits_to_go;

	int stored_byte_pos;
	int stored_bits_to_go;
	int byte_pos_skip;
	int bits_to_go_skip;
	int write_flag; //bitstream contains data and needs to be written

	byte byte_buf;
	byte stored_byte_buf;
	byte byte_buf_skip;
	byte *streamBuffer; //actual buffer
} Bitstream;

typedef struct datapartition_enc
{
	Bitstream *bitstream;
} DataPartition;

class EntropyEncode
{
public:
	EntropyEncode(void);
	~EntropyEncode(void);

	unsigned char *encodeVLC(float *pDCTBuf, int iWidth, int iHeight);
	float *zScan(float *, int xStart, int iWidth);
	int * countCoeffToken(float *scannedBlock);
	int adaptiveNumtrail(int xStart, int *coeffTokens3D, int iWidth, int iHeight);
	unsigned char *signTrailOnes(float *scannedArray);
	int * reverseLevels(float *scannedArray);
	int countZeros(float *scannedArray);
	void codingOfRuns(int countZeros, int *zeroLeft, int *runBefore, float *scannedArray, int coeffCounter2);
	int writeSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *se, Bitstream *bitstream);
	int symbol2vlc(SyntaxElement *sym);
	void writeUVLC2buffer(SyntaxElement *se, Bitstream *currStream);



private:
};