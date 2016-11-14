#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

class Transform
{
 public:
  static void FDCT4(float *[4]);
  static void IDCT4(float *[4]);
  
 private:
  static float _Rot4[3];
};

#endif
