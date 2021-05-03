#include "rle8.h"

#include "rleX_extreme_common.h"

#define RLE8_EXTREME_MULTI_SIZE_OF_SYMBOL_HEADER (1 + 1 + 1)
#define RLE8_EXTREME_MULTI_MAX_SIZE_OF_SYMBOL_HEADER (1 + 1 + 4 + 1 + 4)
#define RLE8_EXTREME_MULTI_MIN_RANGE_SHORT (6)
#define RLE8_EXTREME_MULTI_MIN_RANGE_LONG (9)

#define RLE8_EXTREME_SINGLE_SIZE_OF_SYMBOL_HEADER (1 + 1)
#define RLE8_EXTREME_SINGLE_MAX_SIZE_OF_SYMBOL_HEADER (1 + 4 + 1 + 4)
#define RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT (4)
#define RLE8_EXTREME_SINGLE_MIN_RANGE_LONG (8)

#define RLE8_EXTREME_MODE_MULTI 0
#define RLE8_EXTREME_MODE_SINGLE 1

#ifdef _MSC_VER
#pragma pack(1)
typedef struct
#else
typedef struct
__attribute__((packed))
#endif
{
  uint32_t uncompressedLength, compressedLength;
  uint8_t mode;
} rle_extreme_t;

#ifdef _MSC_VER
#pragma pack(1)
typedef struct
#else
typedef struct
__attribute__((packed))
#endif
{
  uint8_t symbol;
  uint8_t count; // + RLE8_EXTREME_MULTI_MIN_RANGE_SHORT
  uint8_t offset;
  uint32_t offsetIfNull;
} rle8_extreme_multi_symbol_debug_t;

#ifdef _MSC_VER
#pragma pack(1)
typedef struct
#else
typedef struct
__attribute__((packed))
#endif
{
  uint8_t count; // + RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT
  uint8_t offset;
  uint32_t offsetIfNull;
} rle8_extreme_single_symbol_debug_t;

int64_t rle8_extreme_compress_multi_sse2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, IN OUT uint8_t *pSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE);
int64_t rle8_extreme_compress_multi_avx2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, IN OUT uint8_t *pSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE);

uint8_t rle8_extreme_single_compress_get_approx_optimal_symbol_sse2(IN const uint8_t *pIn, const size_t inSize);
uint8_t rle8_extreme_single_compress_get_approx_optimal_symbol_avx2(IN const uint8_t *pIn, const size_t inSize);
int64_t rle8_extreme_compress_single_sse2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, const uint8_t maxFreqSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE);
int64_t rle8_extreme_compress_single_avx2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, const uint8_t maxFreqSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE);

void rle8_extreme_decompress_multi_sse(IN const uint8_t *pInStart, OUT uint8_t *pOut);
void rle8_extreme_decompress_multi_avx(IN const uint8_t *pInStart, OUT uint8_t *pOut);
void rle8_extreme_decompress_multi_avx512f(IN const uint8_t *pInStart, OUT uint8_t *pOut);
void rle8_extreme_decompress_single_sse(IN const uint8_t *pInStart, OUT uint8_t *pOut, const uint8_t symbol);
void rle8_extreme_decompress_single_avx(IN const uint8_t *pInStart, OUT uint8_t *pOut, const uint8_t symbol);
void rle8_extreme_decompress_single_avx512f(IN const uint8_t *pInStart, OUT uint8_t *pOut, const uint8_t symbol);

//////////////////////////////////////////////////////////////////////////

uint32_t rle8_extreme_compress_bounds(const uint32_t inSize)
{
  if (inSize > (1 << 30))
    return 0;

  return inSize + (max(RLE8_EXTREME_MULTI_MAX_SIZE_OF_SYMBOL_HEADER, RLE8_EXTREME_SINGLE_MAX_SIZE_OF_SYMBOL_HEADER) + 64) * 2 + sizeof(rle_extreme_t) + 1;
}

uint32_t rle8_extreme_decompress_additional_size()
{
  return 128; // just to be on the safe side.
}

uint32_t rle8_extreme_multi_compress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || inSize == 0 || pOut == NULL || outSize < rle8_extreme_compress_bounds(inSize))
    return 0;

  rle_extreme_t header;
  header.uncompressedLength = inSize;
  header.mode = RLE8_EXTREME_MODE_MULTI;

  memcpy(pOut, &header, sizeof(rle_extreme_t));

  size_t index = sizeof(rle_extreme_t);
  int64_t i = 0;
  int64_t lastRLE = 0;

  int64_t count = 0;
  uint8_t symbol = ~(*pIn);

  _DetectCPUFeatures();

  if (avx2Supported)
    i = rle8_extreme_compress_multi_avx2(pIn, inSize, pOut, &index, &symbol, &count, &lastRLE);
  else
    i = rle8_extreme_compress_multi_sse2(pIn, inSize, pOut, &index, &symbol, &count, &lastRLE);

  for (; i < inSize; i++)
  {
    if (pIn[i] == symbol)
    {
      count++;
    }
    else
    {
      if (count >= RLE8_EXTREME_MULTI_MIN_RANGE_SHORT)
      {
        pOut[index] = symbol;
        index++;

        const int64_t storedCount = count - RLE8_EXTREME_MULTI_MIN_RANGE_SHORT + 1;

        if (storedCount <= 255)
        {
          pOut[index] = (uint8_t)storedCount;
          index++;
        }
        else
        {
          pOut[index] = 0;
          index++;
          *(uint32_t *) &(pOut[index]) = (uint32_t)storedCount;
          index += sizeof(uint32_t);
        }

        const int64_t range = i - lastRLE - count + 1;

        if (range > 255)
        {
          pOut[index] = 0;
          index++;
          *((uint32_t *)& pOut[index]) = (uint32_t)range;
          index += sizeof(uint32_t);
        }
        else
        {
          pOut[index] = (uint8_t)range;
          index++;
        }

        const size_t copySize = i - count - lastRLE;

        memcpy(pOut + index, pIn + lastRLE, copySize);
        index += copySize;

        lastRLE = i;
      }

      symbol = pIn[i];
      count = 1;
    }
  }

  // Copy / Encode remaining bytes.
  {
    const int64_t range = i - lastRLE - count + 1;

    if (count >= RLE8_EXTREME_MULTI_MIN_RANGE_SHORT)
    {
      pOut[index] = symbol;
      index++;

      const int64_t storedCount = count - RLE8_EXTREME_MULTI_MIN_RANGE_SHORT + 1;

      if (storedCount <= 255)
      {
        pOut[index] = (uint8_t)storedCount;
        index++;
      }
      else
      {
        pOut[index] = 0;
        index++;
        *(uint32_t *) &(pOut[index]) = (uint32_t)storedCount;
        index += sizeof(uint32_t);
      }

      if (range > 255)
      {
        pOut[index] = 0;
        index++;
        *((uint32_t *)& pOut[index]) = (uint32_t)range;
        index += sizeof(uint32_t);
      }
      else
      {
        pOut[index] = (uint8_t)range;
        index++;
      }

      const size_t copySize = i - count - lastRLE;

      memcpy(pOut + index, pIn + lastRLE, copySize);
      index += copySize;

      lastRLE = i;

      pOut[index] = 0;
      index++;
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);

      lastRLE = i;
    }
    else
    {
      pOut[index] = 0;
      index++;
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = (uint32_t)range;
      index += sizeof(uint32_t);

      const size_t copySize = i - lastRLE;

      memcpy(pOut + index, pIn + lastRLE, copySize);
      index += copySize;
    }
  }

  // Store compressed length.
  ((rle_extreme_t *)pOut)->compressedLength = (uint32_t)index;

  return (uint32_t)index;
}

uint32_t rle8_extreme_single_compress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || inSize == 0 || pOut == NULL || outSize < rle8_extreme_compress_bounds(inSize))
    return 0;

  rle_extreme_t header;
  header.uncompressedLength = inSize;
  header.mode = RLE8_EXTREME_MODE_SINGLE;

  memcpy(pOut, &header, sizeof(rle_extreme_t));

  _DetectCPUFeatures();
  
  uint8_t maxFreqSymbol;

  // The AVX2 variant appears to be slower, so we're just always calling the SSE2 version.
  //if (avx2Supported)
  //  maxFreqSymbol = rle8_extreme_single_compress_get_approx_optimal_symbol_avx2(pIn, inSize);
  //else
    maxFreqSymbol = rle8_extreme_single_compress_get_approx_optimal_symbol_sse2(pIn, inSize);

  size_t index = sizeof(rle_extreme_t);
  
  pOut[index] = maxFreqSymbol;
  index++;
  
  int64_t i = 0;
  int64_t lastRLE = 0;

  int64_t count = 0;

  // The AVX2 variant appears to be slower, so we're just always calling the SSE2 version.
  //if (avx2Supported)
  //  i = rle8_extreme_compress_single_avx2(pIn, inSize, pOut, &index, maxFreqSymbol, &count, &lastRLE);
  //else
    i = rle8_extreme_compress_single_sse2(pIn, inSize, pOut, &index, maxFreqSymbol, &count, &lastRLE);

  for (; i < inSize; i++)
  {
    if (pIn[i] == maxFreqSymbol)
    {
      count++;
    }
    else
    {
      {
        const int64_t range = i - lastRLE - count + 1;

        if (range <= 255 && count >= RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT)
        {
          const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;
        
          if (storedCount <= 255)
          {
            pOut[index] = (uint8_t)storedCount;
            index++;
          }
          else
          {
            pOut[index] = 0;
            index++;
            *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
            index += sizeof(uint32_t);
          }
        
          pOut[index] = (uint8_t)range;
          index++;

          const size_t copySize = i - count - lastRLE;

          memcpy(pOut + index, pIn + lastRLE, copySize);
          index += copySize;

          lastRLE = i;
        }
        else if (count >= RLE8_EXTREME_SINGLE_MIN_RANGE_LONG)
        {
          const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

          if (storedCount <= 255)
          {
            pOut[index] = (uint8_t)storedCount;
            index++;
          }
          else
          {
            pOut[index] = 0;
            index++;
            *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
            index += sizeof(uint32_t);
          }

          pOut[index] = 0;
          index++;
          *((uint32_t *)&pOut[index]) = (uint32_t)range;
          index += sizeof(uint32_t);

          const size_t copySize = i - count - lastRLE;

          memcpy(pOut + index, pIn + lastRLE, copySize);
          index += copySize;

          lastRLE = i;
        }
      }

      count = (pIn[i] == maxFreqSymbol);
    }
  }

  {
    const int64_t range = i - lastRLE - count + 1;

    if (range <= 255 && count >= RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT)
    {
      const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

      if (storedCount <= 255)
      {
        pOut[index] = (uint8_t)storedCount;
        index++;
      }
      else
      {
        pOut[index] = 0;
        index++;
        *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
        index += sizeof(uint32_t);
      }

      pOut[index] = (uint8_t)range;
      index++;

      const size_t copySize = i - count - lastRLE;

      memcpy(pOut + index, pIn + lastRLE, copySize);
      index += copySize;

      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);

      lastRLE = i;
    }
    else if (count >= RLE8_EXTREME_SINGLE_MIN_RANGE_LONG)
    {
      const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

      if (storedCount <= 255)
      {
        pOut[index] = (uint8_t)storedCount;
        index++;
      }
      else
      {
        pOut[index] = 0;
        index++;
        *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
        index += sizeof(uint32_t);
      }

      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = (uint32_t)range;
      index += sizeof(uint32_t);

      const size_t copySize = i - count - lastRLE;

      memcpy(pOut + index, pIn + lastRLE, copySize);
      index += copySize;

      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);

      lastRLE = i;
    }
    else
    {
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = 0;
      index += sizeof(uint32_t);
      pOut[index] = 0;
      index++;
      *((uint32_t *)&pOut[index]) = (uint32_t)(range + count);
      index += sizeof(uint32_t);

      const size_t copySize = i - lastRLE;

      memcpy(pOut + index, pIn + lastRLE, copySize);
      index += copySize;
    }
  }

  // Store compressed length.
  ((rle_extreme_t *)pOut)->compressedLength = (uint32_t)index;

  return (uint32_t)index;
}

uint32_t rle8_extreme_decompress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || pOut == NULL || inSize == 0 || outSize == 0)
    return 0;

  const size_t expectedInSize = ((rle_extreme_t *)pIn)->compressedLength;
  const size_t expectedOutSize = ((rle_extreme_t *)pIn)->uncompressedLength;
  const uint8_t mode = ((rle_extreme_t *)pIn)->mode;

  if (expectedOutSize > outSize || expectedInSize > inSize)
    return 0;

  size_t index = sizeof(rle_extreme_t);

  _DetectCPUFeatures();

  switch (mode)
  {
  case RLE8_EXTREME_MODE_MULTI:
  {
    pIn += index;

    if (avx512FSupported)
      rle8_extreme_decompress_multi_avx512f(pIn, pOut);
    else if (avxSupported)
      rle8_extreme_decompress_multi_avx(pIn, pOut);
    else
      rle8_extreme_decompress_multi_sse(pIn, pOut);
    
    break;
  }

  case RLE8_EXTREME_MODE_SINGLE:
  {
    const uint8_t symbol = pIn[index];
    index++;

    pIn += index;

    if (avx512FSupported)
      rle8_extreme_decompress_single_avx512f(pIn, pOut, symbol);
    else if (avxSupported)
      rle8_extreme_decompress_single_avx(pIn, pOut, symbol);
    else
      rle8_extreme_decompress_single_sse(pIn, pOut, symbol);

    break;
  }

  default:
    return 0;
  }

  return (uint32_t)expectedOutSize;
}

//////////////////////////////////////////////////////////////////////////

int64_t rle8_extreme_compress_multi_sse2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, IN OUT uint8_t *pSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE)
{
  const int64_t endInSize128 = inSize - sizeof(__m128i);
  int64_t i = 0;
  size_t index = *pOutIndex;
  uint8_t symbol = *pSymbol;
  __m128i symbol128 = _mm_set1_epi8(symbol);
  int64_t count = 0;
  int64_t lastRLE = 0;

  while (i < endInSize128)
  {
    const uint32_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(symbol128, _mm_loadu_si128((const __m128i *)&(pIn[i]))));

    if (0xFFFF == mask)
    {
      count += sizeof(symbol128);
      i += sizeof(symbol128);
    }
    else
    {
      if (mask != 0 || count > 1)
      {
#ifdef _MSC_VER
        unsigned long _zero;
        _BitScanForward64(&_zero, ~mask);
#else
        const uint64_t _zero = __builtin_ctzl(~mask);
#endif

        count += _zero;
        i += _zero;

        if (count >= RLE8_EXTREME_MULTI_MIN_RANGE_SHORT)
        {
          pOut[index] = symbol;
          index++;

          const int64_t storedCount = count - RLE8_EXTREME_MULTI_MIN_RANGE_SHORT + 1;

          if (storedCount <= 255)
          {
            pOut[index] = (uint8_t)storedCount;
            index++;
          }
          else
          {
            pOut[index] = 0;
            index++;
            *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
            index += sizeof(uint32_t);
          }

          const int64_t range = i - lastRLE - count + 1;

          if (range > 255)
          {
            pOut[index] = 0;
            index++;
            *((uint32_t *)&pOut[index]) = (uint32_t)range;
            index += sizeof(uint32_t);
          }
          else
          {
            pOut[index] = (uint8_t)range;
            index++;
          }

          const size_t copySize = i - count - lastRLE;

          memcpy(pOut + index, pIn + lastRLE, copySize);
          index += copySize;

          lastRLE = i;
        }
      }

      while (i < endInSize128)
      {
        const __m128i current = _mm_loadu_si128((const __m128i *)(&pIn[i]));
        const __m128i next = _mm_bsrli_si128(current, 1);
        const int32_t cmp = 0x7FFF & _mm_movemask_epi8(_mm_cmpeq_epi8(current, next));

        if (cmp == 0)
        {
          i += sizeof(symbol128) - 1;
        }
        else
        {
#ifdef _MSC_VER
          unsigned long _zero;
          _BitScanForward64(&_zero, cmp);
#else
          const uint64_t _zero = __builtin_ctzl(cmp);
#endif

          i += _zero;
          break;
        }
      }

      symbol = pIn[i];
      symbol128 = _mm_set1_epi8(symbol);
      count = 1;
      i++;
    }
  }

  *pOutIndex = index;
  *pSymbol = symbol;
  *pCount = count;
  *pLastRLE = lastRLE;

  return i;
}

#ifndef _MSC_VER
__attribute__((target("avx2")))
#endif
int64_t rle8_extreme_compress_multi_avx2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, IN OUT uint8_t *pSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE)
{
  const int64_t endInSize256 = inSize - sizeof(__m256i);
  int64_t i = 0;
  size_t index = *pOutIndex;
  uint8_t symbol = *pSymbol;
  __m256i symbol256 = _mm256_set1_epi8(symbol);
  int64_t count = 0;
  int64_t lastRLE = 0;

  while (i < endInSize256)
  {
    const uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(symbol256, _mm256_loadu_si256((const __m256i *)&(pIn[i]))));

    if (0xFFFFFFFF == mask)
    {
      count += sizeof(symbol256);
      i += sizeof(symbol256);
    }
    else
    {
      if (mask != 0 || count > 1)
      {
#ifdef _MSC_VER
        unsigned long _zero;
        _BitScanForward64(&_zero, ~mask);
#else
        const uint64_t _zero = __builtin_ctzl(~mask);
#endif

        count += _zero;
        i += _zero;

        if (count >= RLE8_EXTREME_MULTI_MIN_RANGE_SHORT)
        {
          pOut[index] = symbol;
          index++;

          const int64_t storedCount = count - RLE8_EXTREME_MULTI_MIN_RANGE_SHORT + 1;

          if (storedCount <= 255)
          {
            pOut[index] = (uint8_t)storedCount;
            index++;
          }
          else
          {
            pOut[index] = 0;
            index++;
            *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
            index += sizeof(uint32_t);
          }

          const int64_t range = i - lastRLE - count + 1;

          if (range > 255)
          {
            pOut[index] = 0;
            index++;
            *((uint32_t *)&pOut[index]) = (uint32_t)range;
            index += sizeof(uint32_t);
          }
          else
          {
            pOut[index] = (uint8_t)range;
            index++;
          }

          const size_t copySize = i - count - lastRLE;

          memcpy(pOut + index, pIn + lastRLE, copySize);
          index += copySize;

          lastRLE = i;
        }
      }

      while (i < endInSize256)
      {
        const __m256i current = _mm256_loadu_si256((const __m256i *)(&pIn[i]));
        const __m256i next = _mm256_loadu_si256((const __m256i *)(&pIn[i + 1]));
        const int32_t cmp = 0x7FFFFFFF & _mm256_movemask_epi8(_mm256_cmpeq_epi8(current, next));

        if (cmp == 0)
        {
          i += sizeof(symbol256) - 1;
        }
        else
        {
#ifdef _MSC_VER
          unsigned long _zero;
          _BitScanForward64(&_zero, cmp);
#else
          const uint64_t _zero = __builtin_ctzl(cmp);
#endif

          i += _zero;
          break;
        }
      }

      symbol = pIn[i];
      symbol256 = _mm256_set1_epi8(symbol);
      count = 1;
      i++;
    }
  }

  *pOutIndex = index;
  *pSymbol = symbol;
  *pCount = count;
  *pLastRLE = lastRLE;

  return i;
}

uint8_t rle8_extreme_single_compress_get_approx_optimal_symbol_sse2(IN const uint8_t *pIn, const size_t inSize)
{
  uint32_t prob[256];
  uint32_t pcount[256];
  bool consumed[256];

  memset(prob, 0, sizeof(prob));
  memset(pcount, 0, sizeof(pcount));
  memset(consumed, 0, sizeof(consumed));

  uint8_t lastSymbol = 0;
  uint32_t count = 0;

  if (pIn[0] != lastSymbol)
    pcount[lastSymbol] = (uint32_t)-1;

  int64_t inIndex = 0;
  lastSymbol = ~pIn[0];
  __m128i lastSymbol128 = _mm_set1_epi8(lastSymbol);
  const int64_t endInSize128 = inSize - sizeof(lastSymbol128);

  // This is far from optimal, but a lot faster than what we had previously. If you prefer accuracy, use what the normal rle8 uses.
  for (; inIndex < endInSize128; inIndex++)
  {
    const uint32_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(lastSymbol128, _mm_loadu_si128((const __m128i *)&(pIn[inIndex]))));

    if (0xFFFF == mask)
    {
      count += sizeof(lastSymbol128) - 1;
      inIndex += sizeof(lastSymbol128) - 1;
    }
    else
    {
      if (mask != 0 || count > 1)
      {
#ifdef _MSC_VER
        unsigned long _zero;
        _BitScanForward64(&_zero, ~mask);
#else
        const uint64_t _zero = __builtin_ctzl(~mask);
#endif

        count += _zero;
        inIndex += _zero;

        prob[lastSymbol] += count;
        pcount[lastSymbol]++;
      }

      while (inIndex < endInSize128)
      {
        const __m128i current = _mm_loadu_si128((const __m128i *)(&pIn[inIndex]));
        const __m128i next = _mm_bsrli_si128(current, 1);
        const int32_t cmp = 0x7FFF & _mm_movemask_epi8(_mm_cmpeq_epi8(current, next));

        if (cmp == 0)
        {
          inIndex += sizeof(lastSymbol128) - 1;
        }
        else
        {
#ifdef _MSC_VER
          unsigned long _zero;
          _BitScanForward64(&_zero, cmp);
#else
          const uint64_t _zero = __builtin_ctzl(cmp);
#endif

          inIndex += _zero;
          break;
        }
      }

      count = 1;
      lastSymbol = pIn[inIndex];
      lastSymbol128 = _mm_set1_epi8(lastSymbol);
    }
  }

  prob[lastSymbol] += count;
  pcount[lastSymbol]++;

  size_t maxBytesSaved = 0;
  size_t maxBytesSavedIndexIndex = 0;

  for (size_t i = 0; i < 256; i++)
  {
    if (pcount[i] > 0 && prob[i] / pcount[i] > 2)
    {
      const size_t saved = prob[i] - (pcount[i] * 2);

      if (saved > maxBytesSaved)
      {
        maxBytesSaved = saved;
        maxBytesSavedIndexIndex = i;
      }
    }
  }

  return (uint8_t)maxBytesSavedIndexIndex;
}

// This appears to be slower than the SSE2 variant, so it's not being used currently.
#ifndef _MSC_VER
__attribute__((target("avx2")))
#endif
uint8_t rle8_extreme_single_compress_get_approx_optimal_symbol_avx2(IN const uint8_t *pIn, const size_t inSize)
{
  uint32_t prob[256];
  uint32_t pcount[256];
  bool consumed[256];

  memset(prob, 0, sizeof(prob));
  memset(pcount, 0, sizeof(pcount));
  memset(consumed, 0, sizeof(consumed));

  uint8_t lastSymbol = 0;
  uint32_t count = 0;

  if (pIn[0] != lastSymbol)
    pcount[lastSymbol] = (uint32_t)-1;

  int64_t inIndex = 0;
  lastSymbol = ~pIn[0];
  __m256i lastSymbol256 = _mm256_set1_epi8(lastSymbol);
  const int64_t endInSize256 = inSize - sizeof(lastSymbol256);

  // This is far from optimal, but a lot faster than what we had previously. If you prefer accuracy, use what the normal rle8 uses.
  for (; inIndex < endInSize256; inIndex++)
  {
    const uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(lastSymbol256, _mm256_loadu_si256((const __m256i *)&(pIn[inIndex]))));

    if (0xFFFFFFFF == mask)
    {
      count += sizeof(lastSymbol256) - 1;
      inIndex += sizeof(lastSymbol256) - 1;
    }
    else
    {
      if (mask != 0 || count > 1)
      {
#ifdef _MSC_VER
        unsigned long _zero;
        _BitScanForward64(&_zero, ~mask);
#else
        const uint64_t _zero = __builtin_ctzl(~mask);
#endif

        count += _zero;
        inIndex += _zero;

        prob[lastSymbol] += count;
        pcount[lastSymbol]++;
      }

      while (inIndex < endInSize256)
      {
        const __m256i current = _mm256_loadu_si256((const __m256i *)(&pIn[inIndex]));
        const __m256i next = _mm256_loadu_si256((const __m256i *)(&pIn[inIndex + 1]));
        const int32_t cmp = 0x7FFFFFFF & _mm256_movemask_epi8(_mm256_cmpeq_epi8(current, next));

        if (cmp == 0)
        {
          inIndex += sizeof(lastSymbol256) - 1;
        }
        else
        {
#ifdef _MSC_VER
          unsigned long _zero;
          _BitScanForward64(&_zero, cmp);
#else
          const uint64_t _zero = __builtin_ctzl(cmp);
#endif

          inIndex += _zero;
          break;
        }
      }

      count = 1;
      lastSymbol = pIn[inIndex];
      lastSymbol256 = _mm256_set1_epi8(lastSymbol);
    }
  }

  prob[lastSymbol] += count;
  pcount[lastSymbol]++;

  size_t maxBytesSaved = 0;
  size_t maxBytesSavedIndexIndex = 0;

  for (size_t i = 0; i < 256; i++)
  {
    if (pcount[i] > 0 && prob[i] / pcount[i] > 2)
    {
      const size_t saved = prob[i] - (pcount[i] * 2);

      if (saved > maxBytesSaved)
      {
        maxBytesSaved = saved;
        maxBytesSavedIndexIndex = i;
      }
    }
  }

  return (uint8_t)maxBytesSavedIndexIndex;
}

int64_t rle8_extreme_compress_single_sse2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, const uint8_t maxFreqSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE)
{
  int64_t i = 0;
  size_t index = *pOutIndex;
  int64_t count = 0;
  int64_t lastRLE = 0;
  size_t wastedChances = 0;
  size_t firstWastedChanceIndex = 0;

  const __m128i symbol128 = _mm_set1_epi8(maxFreqSymbol);
  const int64_t endInSize128 = inSize - sizeof(symbol128);

  for (; i < endInSize128; i++)
  {
    const uint32_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(symbol128, _mm_loadu_si128((const __m128i *) & (pIn[i]))));

    if (0xFFFF == mask)
    {
      count += sizeof(symbol128);
      i += sizeof(symbol128) - 1;
    }
    else
    {
      if (mask != 0 || count > 1)
      {
#ifdef _MSC_VER
        unsigned long _zero;
        _BitScanForward64(&_zero, ~mask);
#else
        const uint64_t _zero = __builtin_ctzl(~mask);
#endif

        count += _zero;
        i += _zero;

        const int64_t range = i - lastRLE - count + 1;

        if (count >= RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT)
        {
          if (range <= 255)
          {
            const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

            if (storedCount <= 255)
            {
              pOut[index] = (uint8_t)storedCount;
              index++;
            }
            else
            {
              pOut[index] = 0;
              index++;
              *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
              index += sizeof(uint32_t);
            }

            pOut[index] = (uint8_t)range;
            index++;

            const size_t copySize = i - count - lastRLE;

            memcpy(pOut + index, pIn + lastRLE, copySize);
            index += copySize;

            lastRLE = i;
            wastedChances = 0;
          }
          else if (count >= RLE8_EXTREME_SINGLE_MIN_RANGE_LONG)
          {
            const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

            if (storedCount <= 255)
            {
              pOut[index] = (uint8_t)storedCount;
              index++;
            }
            else
            {
              pOut[index] = 0;
              index++;
              *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
              index += sizeof(uint32_t);
            }

            pOut[index] = 0;
            index++;
            *((uint32_t *)&pOut[index]) = (uint32_t)range;
            index += sizeof(uint32_t);

            const size_t copySize = i - count - lastRLE;

            memcpy(pOut + index, pIn + lastRLE, copySize);
            index += copySize;

            lastRLE = i;
            wastedChances = 0;
          }
          else
          {
            wastedChances++;

            if (wastedChances == 1 || i - firstWastedChanceIndex > 255)
            {
              firstWastedChanceIndex = i - count;
              wastedChances = 1;
            }
            else if (wastedChances > 2)
            {
              i = firstWastedChanceIndex;
              wastedChances = 0;
              count = 0;

              for (; i < endInSize128; i++)
                if (pIn[i] == maxFreqSymbol)
                  ++count;
                else
                  break;

              const int64_t wastedRange = i - lastRLE - count + 1;
              const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

              pOut[index] = (uint8_t)storedCount;
              index++;

              pOut[index] = 0;
              index++;
              *((uint32_t *)&pOut[index]) = (uint32_t)wastedRange;
              index += sizeof(uint32_t);

              const size_t copySize = i - count - lastRLE;

              memcpy(pOut + index, pIn + lastRLE, copySize);
              index += copySize;

              lastRLE = i;
            }
          }
        }
      }

      count = 0;

      while (i < endInSize128)
      {
        const int32_t cmp = _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((const __m128i *)(&pIn[i])), symbol128));

        if (cmp == 0 || ((cmp & 0x8000) == 0 && __popcnt((uint32_t)cmp) < RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT))
        {
          i += sizeof(symbol128);
        }
        else
        {
#ifdef _MSC_VER
          unsigned long _zero;
          _BitScanForward64(&_zero, cmp);
#else
          const uint64_t _zero = __builtin_ctzl(cmp);
#endif

          i += _zero;
          count = 1;
          break;
        }
      }
    }
  }

  *pOutIndex = index;
  *pCount = count;
  *pLastRLE = lastRLE;

  return i;
}

#ifndef _MSC_VER
__attribute__((target("avx2")))
#endif
int64_t rle8_extreme_compress_single_avx2(IN const uint8_t *pIn, const size_t inSize, OUT uint8_t *pOut, IN OUT size_t *pOutIndex, const uint8_t maxFreqSymbol, OUT int64_t *pCount, OUT int64_t *pLastRLE)
{
  int64_t i = 0;
  size_t index = *pOutIndex;
  int64_t count = 0;
  int64_t lastRLE = 0;
  size_t wastedChances = 0;
  size_t firstWastedChanceIndex = 0;

  const __m256i symbol256 = _mm256_set1_epi8(maxFreqSymbol);
  const int64_t endInSize256 = inSize - sizeof(symbol256);

  for (; i < endInSize256; i++)
  {
    const uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(symbol256, _mm256_loadu_si256((const __m256i *)&(pIn[i]))));

    if (0xFFFFFFFF == mask)
    {
      count += sizeof(symbol256);
      i += sizeof(symbol256) - 1;
    }
    else
    {
      if (mask != 0 || count > 1)
      {
#ifdef _MSC_VER
        unsigned long _zero;
        _BitScanForward64(&_zero, ~mask);
#else
        const uint64_t _zero = __builtin_ctzl(~mask);
#endif

        count += _zero;
        i += _zero;

        const int64_t range = i - lastRLE - count + 1;

        if (count >= RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT)
        {
          if (range <= 255)
          {
            const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

            if (storedCount <= 255)
            {
              pOut[index] = (uint8_t)storedCount;
              index++;
            }
            else
            {
              pOut[index] = 0;
              index++;
              *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
              index += sizeof(uint32_t);
            }

            pOut[index] = (uint8_t)range;
            index++;

            const size_t copySize = i - count - lastRLE;

            memcpy(pOut + index, pIn + lastRLE, copySize);
            index += copySize;

            lastRLE = i;

            wastedChances = 0;
          }
          else if (count >= RLE8_EXTREME_SINGLE_MIN_RANGE_LONG)
          {
            const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

            if (storedCount <= 255)
            {
              pOut[index] = (uint8_t)storedCount;
              index++;
            }
            else
            {
              pOut[index] = 0;
              index++;
              *(uint32_t *)&(pOut[index]) = (uint32_t)storedCount;
              index += sizeof(uint32_t);
            }

            pOut[index] = 0;
            index++;
            *((uint32_t *)&pOut[index]) = (uint32_t)range;
            index += sizeof(uint32_t);

            const size_t copySize = i - count - lastRLE;

            memcpy(pOut + index, pIn + lastRLE, copySize);
            index += copySize;

            lastRLE = i;
            wastedChances = 0;
          }
          else
          {
            wastedChances++;

            if (wastedChances == 1 || i - firstWastedChanceIndex > 255)
            {
              firstWastedChanceIndex = i - count;
              wastedChances = 1;
            }
            else if (wastedChances > 2)
            {
              i = firstWastedChanceIndex;
              wastedChances = 0;
              count = 0;

              for (; i < endInSize256; i++)
                if (pIn[i] == maxFreqSymbol)
                  ++count;
                else
                  break;

              const int64_t wastedRange = i - lastRLE - count + 1;
              const int64_t storedCount = count - RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT + 1;

              pOut[index] = (uint8_t)storedCount;
              index++;

              pOut[index] = 0;
              index++;
              *((uint32_t *)&pOut[index]) = (uint32_t)wastedRange;
              index += sizeof(uint32_t);

              const size_t copySize = i - count - lastRLE;

              memcpy(pOut + index, pIn + lastRLE, copySize);
              index += copySize;

              lastRLE = i;
            }
          }
        }
      }

      count = 0;

      while (i < endInSize256)
      {
        const int32_t cmp = _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_loadu_si256((const __m256i *)(&pIn[i])), symbol256));

        if (cmp == 0 || ((cmp & 0x80000000) == 0 && __popcnt((uint32_t)cmp) < RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT))
        {
          i += sizeof(symbol256);
        }
        else
        {
#ifdef _MSC_VER
          unsigned long _zero;
          _BitScanForward64(&_zero, cmp);
#else
          const uint64_t _zero = __builtin_ctzl(cmp);
#endif

          i += _zero;
          count = 1;
          break;
        }
      }
    }
  }

  *pOutIndex = index;
  *pCount = count;
  *pLastRLE = lastRLE;

  return i;
}

void rle8_extreme_decompress_multi_sse(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
  size_t offset, symbolCount;
  __m128i symbol;

  while (true)
  {
#ifdef _DEBUG
    rle8_extreme_multi_symbol_debug_t *pSymbol = (rle8_extreme_multi_symbol_debug_t *)pInStart;
    (void)pSymbol;
#endif

    symbol = _mm_set1_epi8(*pInStart);
    pInStart++;
    symbolCount = (size_t)*pInStart;
    pInStart++;

    if (symbolCount == 0)
    {
      symbolCount = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);
    }

    offset = (size_t)*pInStart;
    pInStart++;

    if (offset == 0)
    {
      offset = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);

      if (offset == 0)
        return;
    }

    offset--;

    // memcpy.
    MEMCPY_SSE;

    if (!symbolCount)
      return;

    symbolCount += (RLE8_EXTREME_MULTI_MIN_RANGE_SHORT - 1);

    // memset.
    MEMSET_SSE;
  }
}

#ifndef _MSC_VER
__attribute__((target("avx")))
#endif
void rle8_extreme_decompress_multi_avx(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
  size_t offset, symbolCount;
  __m256i symbol;

  while (true)
  {
#ifdef _DEBUG
    rle8_extreme_multi_symbol_debug_t *pSymbol = (rle8_extreme_multi_symbol_debug_t *)pInStart;
    (void)pSymbol;
#endif

    symbol = _mm256_set1_epi8(*pInStart);
    pInStart++;
    symbolCount = (size_t)*pInStart;
    pInStart++;

    if (symbolCount == 0)
    {
      symbolCount = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);
    }

    offset = (size_t)*pInStart;
    pInStart++;

    if (offset == 0)
    {
      offset = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);

      if (offset == 0)
        return;
    }

    offset--;

    // memcpy.
    MEMCPY_AVX;

    if (!symbolCount)
      return;

    symbolCount += (RLE8_EXTREME_MULTI_MIN_RANGE_SHORT - 1);

    // memset.
    MEMSET_AVX;
  }
}

#ifndef _MSC_VER
__attribute__((target("avx512f")))
#endif
void rle8_extreme_decompress_multi_avx512f(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
  size_t offset, symbolCount;
  __m512i symbol;

  while (true)
  {
#ifdef _DEBUG
    rle8_extreme_multi_symbol_debug_t *pSymbol = (rle8_extreme_multi_symbol_debug_t *)pInStart;
    (void)pSymbol;
#endif

    symbol = _mm512_set1_epi8(*pInStart);
    pInStart++;
    symbolCount = (size_t)*pInStart;
    pInStart++;

    if (symbolCount == 0)
    {
      symbolCount = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);
    }

    offset = (size_t)*pInStart;
    pInStart++;

    if (offset == 0)
    {
      offset = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);

      if (offset == 0)
        return;
    }

    offset--;

    // memcpy.
    MEMCPY_AVX512;

    if (!symbolCount)
      return;

    symbolCount += (RLE8_EXTREME_MULTI_MIN_RANGE_SHORT - 1);

    // memset.
    MEMSET_AVX512;
  }
}

void rle8_extreme_decompress_single_sse(IN const uint8_t *pInStart, OUT uint8_t *pOut, const uint8_t singleSymbol)
{
  size_t offset, symbolCount;
  const __m128i symbol = _mm_set1_epi8(singleSymbol);

  while (true)
  {
#ifdef _DEBUG
    rle8_extreme_single_symbol_debug_t *pSymbol = (rle8_extreme_single_symbol_debug_t *)pInStart;
    (void)pSymbol;
#endif

    symbolCount = (size_t)*pInStart;
    pInStart++;

    if (symbolCount == 0)
    {
      symbolCount = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);
    }

    offset = (size_t)*pInStart;
    pInStart++;

    if (offset == 0)
    {
      offset = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);

      if (offset == 0)
        return;
    }

    offset--;

    // memcpy.
    MEMCPY_SSE;

    if (!symbolCount)
      return;

    symbolCount += (RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT - 1);

    // memset.
    MEMSET_SSE;
  }
}

#ifndef _MSC_VER
__attribute__((target("avx")))
#endif
void rle8_extreme_decompress_single_avx(IN const uint8_t *pInStart, OUT uint8_t *pOut, const uint8_t singleSymbol)
{
  size_t offset, symbolCount;
  const __m256i symbol = _mm256_set1_epi8(singleSymbol);

  while (true)
  {
#ifdef _DEBUG
    rle8_extreme_single_symbol_debug_t *pSymbol = (rle8_extreme_single_symbol_debug_t *)pInStart;
    (void)pSymbol;
#endif

    symbolCount = (size_t)*pInStart;
    pInStart++;

    if (symbolCount == 0)
    {
      symbolCount = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);
    }

    offset = (size_t)*pInStart;
    pInStart++;

    if (offset == 0)
    {
      offset = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);

      if (offset == 0)
        return;
    }

    offset--;

    // memcpy.
    MEMCPY_AVX;

    if (!symbolCount)
      return;

    symbolCount += (RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT - 1);

    // memset.
    MEMSET_AVX;
  }
}


#ifndef _MSC_VER
__attribute__((target("avx512f")))
#endif
void rle8_extreme_decompress_single_avx512f(IN const uint8_t *pInStart, OUT uint8_t *pOut, const uint8_t singleSymbol)
{
  size_t offset, symbolCount;
  const __m512i symbol = _mm512_set1_epi8(singleSymbol);

  while (true)
  {
#ifdef _DEBUG
    rle8_extreme_single_symbol_debug_t *pSymbol = (rle8_extreme_single_symbol_debug_t *)pInStart;
    (void)pSymbol;
#endif

    symbolCount = (size_t)*pInStart;
    pInStart++;

    if (symbolCount == 0)
    {
      symbolCount = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);
    }

    offset = (size_t)*pInStart;
    pInStart++;

    if (offset == 0)
    {
      offset = *(uint32_t *)pInStart;
      pInStart += sizeof(uint32_t);

      if (offset == 0)
        return;
    }

    offset--;

    // memcpy.
    MEMCPY_AVX512;

    if (!symbolCount)
      return;

    symbolCount += (RLE8_EXTREME_SINGLE_MIN_RANGE_SHORT - 1);

    // memset.
    MEMSET_AVX512;
  }
}
