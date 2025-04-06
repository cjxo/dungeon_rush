/* date = March 14th 2025 8:31 pm */

#ifndef BASE_H
#define BASE_H

#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;
typedef s32 b32;

#define Stmnt(s) do{s}while(0)
#define function static

#if defined(DR_DEBUG)
# define AssertBreak() __debugbreak()
# define Assert(c) Stmnt( if(!(c)){AssertBreak();} )
#else
# define AssertBreak()
# define Assert(c) (void)(c)
#endif

#define InvalidDefaultCase() \
default: \
{\
Assert(0);\
} break

#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))
#define Min(a,b) (((a)<(b))?(a):(b))
#define Max(a,b) (((a)>(b))?(a):(b))

#define KB(v) (1024llu*(u64)(v))
#define MB(v) (1024llu*KB(v))
#define GB(v) (1024llu*MB(v))
#define AlignAToB(a,b) ((a)+((b)-1))&(~((b)-1))
#define MemoryClear(m,sz) memset(m,'\0',sz)
#define ClearStructP(s) MemoryClear(s,sizeof(*s))

#define M_Arena_DefaultCommit KB(128)
typedef struct
{
  u8 *base;
  u64 commit_ptr;
  u64 stack_ptr;
  u64 capacity;
} M_Arena;

#define M_Arena_PushStruct(arena,T) M_Arena_PushArray((arena),T)
#define M_Arena_PushArray(arena,T,count) (T*)m_arena_push(arena,sizeof(T)*(count))
function M_Arena *m_arena_reserve(u64 reserve_size);
function void    *m_arena_push(M_Arena *arena, u64 push_size);
function void     m_arena_pop(M_Arena *arena, u64 pop_size);

#endif //BASE_H
