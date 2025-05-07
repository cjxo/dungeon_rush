/* date = March 14th 2025 10:05 pm */

#ifndef MATHEMATICAL_OBJECTS_H
#define MATHEMATICAL_OBJECTS_H

#define DegToRad(deg) (0.01745329251f*(f32)(deg))
#define RadToDeg(rad) (57.2957795131f*(f32)(rad))

inline function f32 absolute_value_f32(f32 a);
inline function b32 close_enough_zero_f32(f32 a, f32 tolerance);

typedef union
{
  struct
  {
    f32 x, y;
  };
  f32 v[2];
} v2f;

#define v2f_zero() v2f_make(0,0)
inline function v2f v2f_make(f32 x, f32 y);

typedef union
{
  struct
  {
    f32 x, y, z;
  };
  struct
  {
    v2f xy;
    f32 __unused_a;
  };
  f32 v[3];
} v3f;

inline function v3f v3f_make(f32 x, f32 y, f32 z);
inline function v3f v3f_add(v3f a, v3f b);
inline function v3f v3f_sub(v3f a, v3f b);
inline function void v3f_add_eq(v3f *a, v3f b);
inline function v3f v3f_sub_and_normalize_or_zero(v3f a, v3f b);

typedef union
{
  struct
  {
    f32 x, y, z, w;
  };
  f32 v[4];
} v4f;

#define rgba(r,g,b,a) v4f_make((f32)(r)/255.0f,(f32)(g)/255.0f,(f32)(b)/255.0f,(a))
inline function v4f v4f_make(f32 x, f32 y, f32 z, f32 w);
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

inline function m44 m44_make_orthographic_z01(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane);

#endif //MATHEMATICAL_OBJECTS_H
