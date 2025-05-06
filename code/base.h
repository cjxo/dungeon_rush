/* date = March 14th 2025 8:31 pm */

#ifndef BASE_H
#define BASE_H

#include <stdint.h>
#include <string.h>
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
#define global_variable static
#define thread_variable __declspec(thread)

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

#define HeyDeveloperPleaseImplementMeSoon() Assert(0)

#define ForLoopU64(idx_name,limit) for (u64 idx_name = 0; idx_name < (limit); ++(idx_name))

#if defined(_MSC_VER)
# define ROTR32(r,c) _rotr((r), (c))
#else
# define ROTR32(r,c) (((r)>>(c))|((r)<<(-(c)&31)))
#endif

#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))
#define Min(a,b) (((a)<(b))?(a):(b))
#define Max(a,b) (((a)>(b))?(a):(b))

#define KB(v) (1024llu*(u64)(v))
#define MB(v) (1024llu*KB(v))
#define GB(v) (1024llu*MB(v))
#define AlignAToB(a,b) ((a)+((b)-1))&(~((b)-1))
#define MemoryClear(m,sz) memset(m,'\0',sz)
#define ClearStructP(s) MemoryClear(s,sizeof(*s))
#define Swap(T,a,b) Stmnt(T temp = a; a = b; b = temp;)
#define MemoryCopy(dest,src,sz) memcpy(dest,src,sz)

#define InvalidIndexU64 0xFFFFFFFFFFFFFFFFllu

#define SLLPushFrontN(head,n,next) (((n)->next=(head)),(head)=(n))

#define M_Arena_DefaultCommit KB(128)
typedef struct
{
  u8 *base;
  u64 commit_ptr;
  u64 stack_ptr;
  u64 capacity;
} M_Arena;

#define M_Arena_PushStruct(arena,T) M_Arena_PushArray((arena),T,1)
#define M_Arena_PushArray(arena,T,count) (T*)m_arena_push(arena,sizeof(T)*(count))
function M_Arena     *m_arena_reserve(u64 reserve_size);
function void        *m_arena_push(M_Arena *arena, u64 push_size);
function void         m_arena_pop(M_Arena *arena, u64 pop_size);
inline function void  m_arena_clear(M_Arena *arena);

typedef struct
{
  M_Arena *arena;
  u64 start_stack_ptr;
} Temporary_Memory;
inline function Temporary_Memory begin_temporary_memory(M_Arena *arena);
inline function void end_temporary_memory(Temporary_Memory temp);

function M_Arena *get_transient_arena(M_Arena **conflict, u64 count);

#define str8(s) (String_U8_Const){(u8*)(s),(sizeof(s)-1),(sizeof(s)-1)}
typedef struct
{
  u8 *s;
  u64 cap;
  u64 count;
} String_U8_Const;
typedef String_U8_Const String_U8;

function String_U8_Const str8_format_va(M_Arena *arena, String_U8_Const str, va_list args0);
function String_U8_Const str8_format(M_Arena *arena, String_U8_Const string, ...);
function u64             str8_calculate_hash(String_U8_Const str, u64 base);
function u64             str8_find_first_string(String_U8_Const str, String_U8_Const to_find, u64 offset_from_beginning_of_source);
function b32             str8_equal_strings(String_U8_Const a, String_U8_Const b);
function String_U8       str8_copy(M_Arena *arena, String_U8_Const str);

#endif //BASE_H
