inline function f32
absolute_value_f32(f32 a)
{
  union
  {
    f32 f;
    u32 n;
  } un;
  
  un.f = a;
  un.n &= ~(0x80000000);
  
  return(un.f);
}

inline function b32
close_enough_zero_f32(f32 a, f32 tolerance)
{
  return(absolute_value_f32(a) <= tolerance);
}

inline function v3f
v3f_add(v3f a, v3f b)
{
  v3f result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  return(result);
}

inline function v3f
v3f_sub(v3f a, v3f b)
{
  v3f result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  return(result);
}

inline function void
v3f_add_eq(v3f *a, v3f b)
{
  a->x += b.x;
  a->y += b.y;
  a->z += b.z;
}

inline function f32
v3f_length(v3f a)
{
  f32 result = a.x*a.x + a.y*a.y + a.z*a.z;
  return(sqrtf(result));
}

inline function v3f
v3f_sub_and_normalize_or_zero(v3f a, v3f b)
{
  v3f result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  
  f32 length = v3f_length(result);
  if (close_enough_zero_f32(length, 0.0001f))
  {
    result.x = result.y = result.z = 0.0f;
  }
  else
  {
    result.x /= length;
    result.y /= length;
    result.z /= length;
  }
  
  return(result);
}

inline static m44
m44_make_orthographic_z01(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane)
{
  f32 rml = right - left;
  f32 tmb = top - bottom;
  
  m44 result =
  {
    2.0f / rml,        0.0f,      0.0f,       0.0f,
    0.0f,        2.0f / tmb,      0.0f,       0.0f,
    0.0f,              0.0f,      1.0f / (far_plane - near_plane),       0.0f,
    -(right + left) / rml, -(top + bottom) / tmb,  -near_plane / (far_plane - near_plane), 1.0f,
  };
  return(result);
}