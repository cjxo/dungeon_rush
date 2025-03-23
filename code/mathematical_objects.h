/* date = March 14th 2025 10:05 pm */

#ifndef MATHEMATICAL_OBJECTS_H
#define MATHEMATICAL_OBJECTS_H

typedef union
{
  struct
  {
    f32 x, y;
  };
  f32 v[2];
} v2f;

typedef union
{
  struct
  {
    f32 x, y, z;
  };
  f32 v[3];
} v3f;

typedef union
{
  struct
  {
    f32 x, y, z, w;
  };
  f32 v[4];
} v4f;

typedef union
{
  struct
  {
    f32 a00, a01, a02, a03;
    f32 a10, a11, a12, a13;
    f32 a20, a21, a22, a23;
    f32 a30, a31, a32, a33;
  };
  v4f r[4];
  f32 m[4][4];
} m44;

inline static m44 m44_make_orthographic_z01(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane);

#endif //MATHEMATICAL_OBJECTS_H
