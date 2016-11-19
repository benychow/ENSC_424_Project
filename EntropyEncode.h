#pragma once
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

private:
};