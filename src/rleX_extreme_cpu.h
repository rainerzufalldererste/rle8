#define RLEX_EXTREME_MULTI_MIN_RANGE_SHORT ((sizeof(symbol_t) + 1 + 1) + 2)
#define RLEX_EXTREME_MULTI_MIN_RANGE_LONG ((sizeof(symbol_t) + 1 + 4 + 1 + 4) + 2)

#define CONCAT_LITERALS3(a, b, c) a ## b ## c
#define CONCAT3(a, b, c) CONCAT_LITERALS3(a, b, c)

#define CONCAT_LITERALS2(a, b) a ## b
#define CONCAT2(a, b) CONCAT_LITERALS2(a, b)

#if TYPE_SIZE == 64
  #define _mm_set1_epi64 _mm_set1_epi64x
  #define _mm256_set1_epi64 _mm256_set1_epi64x
#endif

uint32_t CONCAT3(rle, TYPE_SIZE, _extreme_compress)(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || inSize == 0 || pOut == NULL || outSize < rle8_extreme_compress_bounds(inSize))
    return 0;

  size_t index = 0;

  *(uint32_t *)(&(pOut[index])) = inSize;
  index += sizeof(uint32_t);
  *(uint32_t *)(&(pOut[index])) = 0; // will be compressed size.
  index += sizeof(uint32_t);

  int64_t i = 0;
  int64_t lastRLE = 0;

  typedef CONCAT3(uint, TYPE_SIZE, _t) symbol_t;

  int64_t count = 0;
  symbol_t symbol = ~(*((symbol_t *)(pIn)));

  for (; i < inSize; i++)
  {
    if (count && *(symbol_t *)&pIn[i] == symbol && i + sizeof(symbol_t) <= inSize)
    {
      count += sizeof(symbol_t);
      i += sizeof(symbol_t) - 1;
    }
    else
    {
      {
        const int64_t range = i - lastRLE - count + 1;

        if (range <= 255 && count >= RLEX_EXTREME_MULTI_MIN_RANGE_SHORT)
        {
          *(symbol_t *)(&pOut[index]) = symbol;
          index += sizeof(symbol_t);

          const int64_t storedCount = (count / sizeof(symbol_t)) - (RLEX_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) + 1;

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
        else if (count >= RLEX_EXTREME_MULTI_MIN_RANGE_LONG)
        {
          *(symbol_t *)(&pOut[index]) = symbol;
          index += sizeof(symbol_t);

          const int64_t storedCount = (count / sizeof(symbol_t)) - (RLEX_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) + 1;

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

      symbol = *(symbol_t *)(&pIn[i]);

      if (i + sizeof(symbol_t) <= inSize && ((symbol_t *)(&pIn[i]))[1] == symbol)
      {
        count = sizeof(symbol_t) * 2;
        i += sizeof(symbol_t) * 2 - 1;
      }
      else
      {
        count = 0;
      }
    }
  }

  {
    const int64_t range = i - lastRLE - count + 1;

    if (range <= 255 && count >= RLEX_EXTREME_MULTI_MIN_RANGE_SHORT)
    {
      *(symbol_t *)(&pOut[index]) = symbol;
      index += sizeof(symbol_t);

      const int64_t storedCount = (count / sizeof(symbol_t)) - (RLEX_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) + 1;

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

      *(symbol_t *)(&pOut[index]) = 0;
      index += sizeof(symbol_t);
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
    else if (count >= RLEX_EXTREME_MULTI_MIN_RANGE_LONG)
    {
      *(symbol_t *)(&pOut[index]) = symbol;
      index += sizeof(symbol_t);

      const int64_t storedCount = (count / sizeof(symbol_t)) - (RLEX_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) + 1;

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

      *(symbol_t *)(&pOut[index]) = 0;
      index += sizeof(symbol_t);
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
      *(symbol_t *)(&pOut[index]) = 0;
      index += sizeof(symbol_t);
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
  ((uint32_t *)pOut)[1] = (uint32_t)index;

  return (uint32_t)index;
}

void CONCAT3(rle, TYPE_SIZE, _extreme_decompress_sse)(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
  size_t offset, symbolCount;
  __m128i symbol;

  typedef CONCAT3(uint, TYPE_SIZE, _t) symbol_t;

  while (true)
  {
    symbol = CONCAT2(_mm_set1_epi, TYPE_SIZE)(*(symbol_t *)pInStart);
    pInStart += sizeof(symbol_t);
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
    MEMCPY_SSE_MULTI;

    if (!symbolCount)
      return;

    symbolCount = (symbolCount + (RLEX_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) - 1) * sizeof(symbol_t);

    // memset.
    MEMSET_SSE_MULTI;
  }
}

#ifndef _MSC_VER
__attribute__((target("avx")))
#endif
void CONCAT3(rle, TYPE_SIZE, _extreme_decompress_avx)(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
  size_t offset, symbolCount;
  __m256i symbol;

  typedef CONCAT3(uint, TYPE_SIZE, _t) symbol_t;

  while (true)
  {
    symbol = CONCAT2(_mm256_set1_epi, TYPE_SIZE)(*(symbol_t *)pInStart);
    pInStart += sizeof(symbol_t);
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
    MEMCPY_AVX_MULTI;

    if (!symbolCount)
      return;

    symbolCount = (symbolCount + (RLEX_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) - 1) * sizeof(symbol_t);

    // memset.
    MEMSET_AVX_MULTI;
  }
}

#ifndef _MSC_VER
__attribute__((target("avx512f")))
#endif
void CONCAT3(rle, TYPE_SIZE, _extreme_decompress_avx512f)(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
  size_t offset, symbolCount;
  __m512i symbol;

  typedef CONCAT3(uint, TYPE_SIZE, _t) symbol_t;

  while (true)
  {
    symbol = CONCAT2(_mm512_set1_epi, TYPE_SIZE)(*(symbol_t *)pInStart);
    pInStart += sizeof(symbol_t);
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
    MEMCPY_AVX512_MULTI;

    if (!symbolCount)
      return;

    symbolCount = (symbolCount + (RLEX_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) - 1) * sizeof(symbol_t);

    // memset.
    MEMSET_AVX512_MULTI;
  }
}

uint32_t CONCAT3(rle, TYPE_SIZE, _extreme_decompress)(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || pOut == NULL || inSize == 0 || outSize == 0)
    return 0;

  const size_t expectedInSize = ((uint32_t *)pIn)[1];
  const size_t expectedOutSize = ((uint32_t *)pIn)[0];

  if (expectedOutSize > outSize || expectedInSize > inSize)
    return 0;

  size_t index = sizeof(uint32_t) * 2;

  _DetectCPUFeatures();

  pIn += index;

  if (avx512FSupported)
    CONCAT3(rle, TYPE_SIZE, _extreme_decompress_avx512f)(pIn, pOut);
  else if (avxSupported)
    CONCAT3(rle, TYPE_SIZE, _extreme_decompress_avx)(pIn, pOut);
  else
    CONCAT3(rle, TYPE_SIZE, _extreme_decompress_sse)(pIn, pOut);

  return (uint32_t)expectedOutSize;
}

#undef RLEX_EXTREME_MULTI_MIN_RANGE_SHORT
#undef RLEX_EXTREME_MULTI_MIN_RANGE_LONG

#undef CONCAT_LITERALS3
#undef CONCAT3

#undef CONCAT_LITERALS2
#undef CONCAT2

#if TYPE_SIZE == 64
  #undef _mm_set1_epi64
  #undef _mm256_set1_epi64
#endif