#define PCG32_DEFAULT_MULTIPLIER 6364136223846793005ULL
#define PCG32_DEFAULT_INCREMENT  1442695040888963407ULL

function inline void
prng32_seed(PRNG32 *rng, u64 seed)
{
  rng->state = 0;
  prng32_nextu32(rng);
  rng->state += seed;
  prng32_nextu32(rng);
}

function inline u32
prng32_nextu32(PRNG32 *rng)
{
  u64 state = rng->state;
  rng->state = state * PCG32_DEFAULT_MULTIPLIER + PCG32_DEFAULT_INCREMENT;
  u32 value = (u32)((state ^ (state >> 18)) >> 27);
  s32 rot = state >> 59;
  return ROTR32(value, rot);
}

function inline u32
prng32_rangeu32(PRNG32 *rng, u32 low, u32 high)
{
  u32 bound = high - low;
  u64 m = (u64)prng32_nextu32(rng) * (u64)bound;
  u32 l = (u32)m;
  
  if (l < bound)
  {
    u32 t = -(s32)bound % bound;
    while (l < t)
    {
      m = (u64)prng32_nextu32(rng) * (u64)bound;
      l = (u32)m;
    }
  }
  
  return low + (u32)(m >> 32);
}

// [0, 1)
function inline f32
prng32_nextf32(PRNG32 *rng)
{
  u32 x = prng32_nextu32(rng);
  return (f32)(s32)(x >> 8) * 0x1.0p-24f;
}