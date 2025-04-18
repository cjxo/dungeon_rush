function M_Arena *
m_arena_reserve(u64 reserve_size)
{
  M_Arena *result = 0;
  reserve_size = AlignAToB(reserve_size, 16);
  void *block = VirtualAlloc(0, reserve_size, MEM_RESERVE, PAGE_NOACCESS);
  
  if (block)
  {
    u64 new_commit_ptr = AlignAToB(sizeof(M_Arena), M_Arena_DefaultCommit);
    u64 new_commit_ptr_clamped = Min(new_commit_ptr, reserve_size);
    
    VirtualAlloc(block, new_commit_ptr_clamped, MEM_COMMIT, PAGE_READWRITE);
    result = block;
    result->base = block;
    result->commit_ptr = new_commit_ptr_clamped;
    result->stack_ptr = sizeof(M_Arena);
    result->capacity = reserve_size;
  }
  
  return(result);
}

function void *
m_arena_push(M_Arena *arena, u64 push_size)
{
  Assert(arena);
  void *result_block = 0;
  push_size = AlignAToB(push_size, 16);
  u64 desired_stack_ptr = arena->stack_ptr + push_size;
  u64 desired_commit_ptr = arena->commit_ptr;
  if (desired_stack_ptr <= arena->capacity)
  {
    if (desired_stack_ptr >= arena->commit_ptr)
    {
      u64 new_commit_ptr = AlignAToB(desired_stack_ptr, M_Arena_DefaultCommit);
      u64 new_commit_ptr_clamped = Min(new_commit_ptr, arena->capacity);
      
      if (new_commit_ptr_clamped > arena->commit_ptr)
      {
        VirtualAlloc(arena->base + arena->commit_ptr,
                     new_commit_ptr_clamped - arena->commit_ptr,
                     MEM_COMMIT, PAGE_READWRITE);
        desired_commit_ptr = new_commit_ptr_clamped;
      }
    }
    
    if (desired_commit_ptr > desired_stack_ptr)
    {
      result_block = arena->base + arena->stack_ptr;
      arena->stack_ptr = desired_stack_ptr;
      arena->commit_ptr = desired_commit_ptr;
    }
  }
  
  Assert(result_block);
  return(result_block);
}

function void
m_arena_pop(M_Arena *arena, u64 pop_size)
{
  pop_size = AlignAToB(pop_size, 16);
  Assert(pop_size <= (arena->stack_ptr + sizeof(M_Arena)));
  arena->stack_ptr -= pop_size;
  
  u64 new_commit_ptr = AlignAToB(arena->stack_ptr, M_Arena_DefaultCommit);
  if (new_commit_ptr < arena->commit_ptr)
  {
    VirtualFree(arena->base + new_commit_ptr, arena->commit_ptr - new_commit_ptr, MEM_DECOMMIT);
    arena->commit_ptr = new_commit_ptr;
  }
}

inline function Temporary_Memory
begin_temporary_memory(M_Arena *arena)
{
  Assert(!!arena);
  Temporary_Memory result;
  result.arena = arena;
  result.start_stack_ptr = arena->stack_ptr;
  return(result);
}

inline function void
end_temporary_memory(Temporary_Memory temp)
{
  Assert(!!temp.arena);
  Assert(temp.arena->stack_ptr >= temp.start_stack_ptr);
  m_arena_pop(temp.arena, temp.arena->stack_ptr - temp.start_stack_ptr);
}

#define MaxTransientArena() 4
global_variable thread_variable M_Arena *g_transient_arena[MaxTransientArena()];

function M_Arena *
get_transient_arena(M_Arena **conflict, u64 count)
{
  if (!g_transient_arena[0])
  {
    for (u64 arena_idx = 0; arena_idx < MaxTransientArena(); ++arena_idx)
    {
      g_transient_arena[arena_idx] = m_arena_reserve(MB(8));
    }
  }
  
  M_Arena *result = 0;
  for (u64 arena_idx = 0; arena_idx < MaxTransientArena(); ++arena_idx)
  {
    b32 accept_selection = 1;
    
    for (u64 conflict_idx = 0; conflict_idx < count; ++conflict_idx)
    {
      if (g_transient_arena[arena_idx] == conflict[conflict_idx])
      {
        accept_selection = 0;
        break;
      }
    }
    
    if (accept_selection)
    {
      result = g_transient_arena[arena_idx];
      break;
    }
  }
  
  Assert(result);
  return(result);
}

function String_U8_Const
str8_format_va(M_Arena *arena, String_U8_Const str, va_list args0)
{
  va_list args1;
  va_copy(args1, args0);
  
  s32 total_chars = vsnprintf(0, 0, (char *)str.s, args1);
  String_U8_Const result = {0};
  if (total_chars)
  {
    result.count = (u64)total_chars;
    result.cap = result.count;
    result.s = M_Arena_PushArray(arena, u8, result.cap + 1);
    vsnprintf((char *)result.s, result.count + 1, (char *)str.s, args1);
    
    result.s[result.count] = 0;
  }
  
  va_end(args1);
  return(result);
}