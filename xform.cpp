#include "xform.h"

//constants used in 4-point DCT:
//sqrt(2)/2, cos(3pi/8), sin(3pi/8)
float Transform::_Rot4[]={0.70710678118655f, 0.38268343236509f, 0.92387953251129f};


//Floating-point Forward 4-point DCT:
// There is a scaling of 1/sqrt(2) for 1D transform.
// After 2D transform, the scalign becomes 1/2. Apply if after the column transform.
void Transform::FDCT4(float *fpBlk[4])
{
  float f[4];

  //row transform
  for(int y = 0; y < 4; y++) {
    f[0] = fpBlk[y][0] + fpBlk[y][3];
    f[3] = fpBlk[y][0] - fpBlk[y][3];
    f[1] = fpBlk[y][1] + fpBlk[y][2];
    f[2] = fpBlk[y][1] - fpBlk[y][2];

    fpBlk[y][0] = _Rot4[0] * (f[0] + f[1]);
    fpBlk[y][2] = _Rot4[0] * (f[0] - f[1]);

    fpBlk[y][1] =  _Rot4[1] * f[2] + _Rot4[2] * f[3];
    fpBlk[y][3] = -_Rot4[2] * f[2] + _Rot4[1] * f[3];    
  }

  //column transform, including the final scaling of 1/2
  for(int x = 0; x < 4; x++) {
    f[0] = fpBlk[0][x] + fpBlk[3][x];
    f[3] = fpBlk[0][x] - fpBlk[3][x];
    f[1] = fpBlk[1][x] + fpBlk[2][x];
    f[2] = fpBlk[1][x] - fpBlk[2][x];

    fpBlk[0][x] = _Rot4[0] * (f[0] + f[1]) / 2;
    fpBlk[2][x] = _Rot4[0] * (f[0] - f[1]) / 2;

    fpBlk[1][x] =  (_Rot4[1] * f[2] + _Rot4[2] * f[3]) / 2;
    fpBlk[3][x] = (-_Rot4[2] * f[2] + _Rot4[1] * f[3]) / 2;
  }
}


//Floating-point inverse 4-point transform
void Transform::IDCT4(float *fpBlk[4])
{
  int x, y;
  float f[4];

  //column transform
  for(x = 0; x < 4; x++)
  {
    f[0] = _Rot4[0] * (fpBlk[0][x] + fpBlk[2][x]);
    f[1] = _Rot4[0] * (fpBlk[0][x] - fpBlk[2][x]);

    f[2] = _Rot4[1] * fpBlk[1][x] - _Rot4[2] * fpBlk[3][x];
    f[3] = _Rot4[2] * fpBlk[1][x] + _Rot4[1] * fpBlk[3][x];

    fpBlk[0][x] = f[0] + f[3];
    fpBlk[3][x] = f[0] - f[3];
    fpBlk[1][x] = f[1] + f[2];
    fpBlk[2][x] = f[1] - f[2];
  }
  
  //row transform, including final scaling of 1/2 
  for(y = 0; y < 4; y++)
  {
    f[0] = _Rot4[0] * (fpBlk[y][0] + fpBlk[y][2]);
    f[1] = _Rot4[0] * (fpBlk[y][0] - fpBlk[y][2]);

    f[2] = _Rot4[1] * fpBlk[y][1] - _Rot4[2] * fpBlk[y][3];
    f[3] = _Rot4[2] * fpBlk[y][1] + _Rot4[1] * fpBlk[y][3];

    fpBlk[y][0] = (f[0] + f[3]) / 2;
    fpBlk[y][3] = (f[0] - f[3]) / 2;
    fpBlk[y][1] = (f[1] + f[2]) / 2;
    fpBlk[y][2] = (f[1] - f[2]) / 2;
  }

}

