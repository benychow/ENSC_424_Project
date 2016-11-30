#pragma once
typedef unsigned char byte;

typedef struct syntaxelement_dec
{
	int type;
	int value1;
	int value2;
	int len;
	int inf;
	unsigned int bitpattern;
} SyntaxElement;

typedef struct bit_stream_dec
{
	// CAVLC Decoding
	int frame_bitoffset;    //!< actual position in the codebuffer, bit-oriented, CAVLC only
	int bitstream_length;   //!< over codebuffer lnegth, byte oriented, CAVLC only
	// ErrorConcealment
	byte *streamBuffer;      //!< actual codebuffer for read bytes
	int ei_flag;            //!< error indication, 0: no error, else unspecified error
} Bitstream;

class EntropyDecode
{
public:
	EntropyDecode(void);
	~EntropyDecode(void);
	
	void decodeVLC(unsigned char *inputBuffer, int iWidth, int iHeight, float fQstep, int iQMtd);
	int ShowBits(byte buffer[], int totbitoffset, int bitcount, int numbits);
	int readSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *sym, Bitstream *currStream, char *type);
private:
};