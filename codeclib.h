#ifndef _ICODEC_H_
#define _ICODEC_H_

#define MAX_CONTEXT_NUM     256
#define BLOCKSIZE           4

class ICodec
{
 public:
  ICodec(unsigned,unsigned);
  ~ICodec(void);

  void SetImage(unsigned char *);  //Copy image to the internal buffer and subtract 128. 
  void GetImage(unsigned char *);  //Read internal image to external buffer, add 128, clip to [0, 255]

  void QuantDZone(float *[], int iBlkSize, float fQStep);
  void DequantDZone(float *[], int iBlkSize, float fQStep);

  void QuantMidtread(float *[], int iBlkSize, float fQStep);
  void DequantMidtread(float *[], int iBlkSize, float fQStep);

 protected:
  unsigned _w, _h;
  float **_d;
  float *m_fBlkBuf[BLOCKSIZE]; 
  int m_iNonzeros;
};

class IEncoder:public ICodec
{
 public:
  IEncoder(unsigned,unsigned);
  ~IEncoder(void);

  float * codeImage(float fQstep, int iQMtd);

  void codeBlock(unsigned y, unsigned x, float fQstep, int iQMtd);

 private:
};

class IDecoder:public ICodec
{
 public:
  IDecoder(unsigned,unsigned);
  ~IDecoder(void);

  void decodeImage(float *b, float fQstep, int iQMtd);
  void decodeBlock(unsigned y, unsigned x, float fQstep, int iQMtd);

 private:
};

#endif
