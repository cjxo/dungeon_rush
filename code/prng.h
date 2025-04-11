/* date = April 11th 2025 7:03 pm */

#ifndef PRNG_H
#define PRNG_H

typedef struct
{
  u64 state;
} PRNG32;

function inline void prng32_seed(PRNG32 *rng, u64 seed);
function inline u32 prng32_nextu32(PRNG32 *rng);
function inline u32 prng32_rangeu32(PRNG32 *rng, u32 low, u32 high);
// [0, 1)
function inline f32 prng32_nextf32(PRNG32 *rng);

#endif //PRNG_H
