#include "rle8.h"

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

//#define DO_NOT_OPTIMIZE_DECODER

uint32_t rle8_compress_bounds(const uint32_t inSize)
{
  return inSize + (256 / 8) + 1 + 256 + sizeof(uint32_t) * 2;
}

uint32_t rle8_compress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || inSize == 0 || pOut == NULL || outSize < rle8_compress_bounds(inSize))
    return 0;

  rle8_compress_info_t compressInfo;

  if (!rle8_get_compress_info(pIn, inSize, &compressInfo))
    return 0;

  size_t index = sizeof(uint32_t); // to make room for the uint32_t length as the first value.

  // Store required information.
  {
    *((uint32_t *)&pOut[index]) = inSize;
    index += sizeof(uint32_t);

    const uint32_t size = rle8_write_compress_info(&compressInfo, &pOut[index], outSize);

    if (size == 0)
      return 0;

    index += size;
  }

  // Compress.
  {
    const uint32_t size = rle8_compress_with_info(pIn, inSize, &compressInfo, &pOut[index], outSize - (uint32_t)index);

    if (size == 0)
      return 0;

    index += size;
  }
  
  // Store compressed length.
  ((uint32_t *)pOut)[0] = (uint32_t)index;

  return (uint32_t)index;
}

uint32_t rle8_compress_only_max_frequency(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || inSize == 0 || pOut == NULL || outSize < rle8_compress_bounds(inSize))
    return 0;

  rle8_compress_info_t compressInfo;

  if (!rle8_get_compress_info_only_max_frequency(pIn, inSize, &compressInfo))
    return 0;

  size_t index = sizeof(uint32_t); // to make room for the uint32_t length as the first value.

  // Store required information.
  {
    *((uint32_t *)&pOut[index]) = inSize;
    index += sizeof(uint32_t);

    const uint32_t size = rle8_write_compress_info(&compressInfo, &pOut[index], outSize);

    if (size == 0)
      return 0;

    index += size;
  }

  // Compress.
  {
    const uint32_t size = rle8_compress_with_info(pIn, inSize, &compressInfo, &pOut[index], outSize - (uint32_t)index);

    if (size == 0)
      return 0;

    index += size;
  }

  // Store compressed length.
  ((uint32_t *)pOut)[0] = (uint32_t)index;

  return (uint32_t)index;
}

uint32_t rle8_decompressed_size(IN const uint8_t *pIn, const uint32_t inSize)
{
  if (pIn == NULL || inSize < sizeof(uint32_t) * 2)
    return 0;

  return ((uint32_t *)pIn)[1];
}

uint32_t rle8_decompress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || pOut == NULL || inSize == 0 || outSize == 0)
    return 0;

  const size_t expectedInSize = ((uint32_t *)pIn)[0];
  const size_t expectedOutSize = ((uint32_t *)pIn)[1];

  if (expectedOutSize > outSize || expectedInSize > inSize)
    return 0;

  size_t index = 2 * sizeof(uint32_t);
  
  rle8_decompress_info_t decompressInfo;

  index += rle8_read_decompress_info(&pIn[index], inSize, &decompressInfo);

  const uint8_t *pEnd = pIn + expectedInSize;
  pIn += index;

  return rle8_decompress_with_info(pIn, pEnd, &decompressInfo, pOut, (uint32_t)expectedOutSize);
}

//////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#define cpuid __cpuid
#else
#include <cpuid.h>

void cpuid(int info[4], int infoType)
{
  __cpuid_count(infoType, 0, info[0], info[1], info[2], info[3]);
}

uint64_t _xgetbv(unsigned int index)
{
  uint32_t eax, edx;
  __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
  return ((uint64_t)edx << 32) | eax;
}

#ifndef _XCR_XFEATURE_ENABLED_MASK
#define _XCR_XFEATURE_ENABLED_MASK  0
#endif
#endif

bool _CpuFeaturesDetected = false;
bool sseSupported = false;
bool sse2Supported = false;
bool sse3Supported = false;
bool ssse3Supported = false;
bool sse41Supported = false;
bool sse42Supported = false;
bool avxSupported = false;
bool avx2Supported = false;
bool fma3Supported = false;
bool avx512FSupported = false;
bool avx512PFSupported = false;
bool avx512ERSupported = false;
bool avx512CDSupported = false;
bool avx512BWSupported = false;
bool avx512DQSupported = false;
bool avx512VLSupported = false;
bool avx512IFMASupported = false;
bool avx512VBMISupported = false;
bool avx512VNNISupported = false;
bool avx512VBMI2Supported = false;
bool avx512POPCNTDQSupported = false;
bool avx512BITALGSupported = false;
bool avx5124VNNIWSupported = false;
bool avx5124FMAPSSupported = false;

void _DetectCPUFeatures()
{
  if (_CpuFeaturesDetected)
    return;

  int32_t info[4];
  cpuid(info, 0);
  int32_t idCount = info[0];

  if (idCount >= 0x1)
  {
    int32_t cpuInfo[4];
    cpuid(cpuInfo, 1);

    const bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
    const bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

    if (osUsesXSAVE_XRSTORE && cpuAVXSuport)
    {
      uint64_t xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
      avxSupported = (xcrFeatureMask & 0x6) != 0;
    }

    sseSupported = (cpuInfo[3] & (1 << 25)) != 0;
    sse2Supported = (cpuInfo[3] & (1 << 26)) != 0;
    sse3Supported = (cpuInfo[2] & (1 << 0)) != 0;
    ssse3Supported = (cpuInfo[2] & (1 << 9)) != 0;
    sse41Supported = (cpuInfo[2] & (1 << 19)) != 0;
    sse42Supported = (cpuInfo[2] & (1 << 20)) != 0;
    fma3Supported = (cpuInfo[2] & (1 << 12)) != 0;
  }

  if (idCount >= 0x7)
  {
    int32_t cpuInfo[4];
    cpuid(cpuInfo, 7);

    avx2Supported = (cpuInfo[1] & (1 << 5)) != 0;
    avx512FSupported = (cpuInfo[1] & 1 << 16) != 0;
    avx512PFSupported = (cpuInfo[1] & 1 << 26) != 0;
    avx512ERSupported = (cpuInfo[1] & 1 << 27) != 0;
    avx512CDSupported = (cpuInfo[1] & 1 << 28) != 0;
    avx512BWSupported = (cpuInfo[1] & 1 << 30) != 0;
    avx512DQSupported = (cpuInfo[1] & 1 << 17) != 0;
    avx512VLSupported = (cpuInfo[1] & 1 << 31) != 0;
    avx512IFMASupported = (cpuInfo[1] & 1 << 21) != 0;
    avx512VBMISupported = (cpuInfo[2] & 1 << 1) != 0;
    avx512VNNISupported = (cpuInfo[2] & 1 << 11) != 0;
    avx512VBMI2Supported = (cpuInfo[2] & 1 << 6) != 0;
    avx512POPCNTDQSupported = (cpuInfo[2] & 1 << 14) != 0;
    avx512BITALGSupported = (cpuInfo[2] & 1 << 12) != 0;
    avx5124VNNIWSupported = (cpuInfo[3] & 1 << 2) != 0;
    avx5124FMAPSSupported = (cpuInfo[3] & 1 << 3) != 0;
  }

  _CpuFeaturesDetected = true;
}

//////////////////////////////////////////////////////////////////////////

uint32_t rle8m_compress_bounds(const uint32_t subSections, const uint32_t inSize)
{
  return inSize + (256 / 8) + 1 + 256 + sizeof(uint32_t) * (2 + subSections - 1 + 1);
}

uint32_t rle8m_compress(const uint32_t subSections, IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || inSize == 0 || pOut == NULL || outSize < rle8m_compress_bounds(subSections, inSize) || subSections == 0)
    return 0;

  rle8_compress_info_t compressInfo;

  if (!rle8_get_compress_info(pIn, inSize, &compressInfo))
    return 0;

  size_t index = sizeof(uint32_t); // Expected Input Size for the Decoder.

  *((uint32_t *)(&pOut[index])) = inSize;
  index += sizeof(uint32_t);

  *((uint32_t *)(&pOut[index])) = subSections;
  index += sizeof(uint32_t);

  const size_t subSectionIndex = index;
  index += sizeof(uint32_t) * (subSections - 1);

  // Write Info.
  {
    const uint32_t size = rle8_write_compress_info(&compressInfo, &pOut[index], outSize);

    if (size == 0)
      return 0;

    index += size;
  }

  const uint32_t subSectionSize = inSize / subSections;

  for (uint32_t i = 0; i < subSections - 1; i++)
  {
    const uint32_t size = rle8_compress_with_info(pIn + subSectionSize * i, subSectionSize, &compressInfo, pOut + index, (uint32_t)(outSize - index));

    if (size == 0)
      return 0;

    index += size;

    ((uint32_t *)(&pOut[subSectionIndex]))[i] = (uint32_t)index;
  }

  const size_t remainingSize = inSize - subSectionSize * (subSections - 1);

  // Compress last block.
  {
    const uint32_t size = rle8_compress_with_info(pIn + subSectionSize * (subSections - 1), (uint32_t)remainingSize, &compressInfo, pOut + index, (uint32_t)(outSize - index));

    if (size == 0)
      return 0;

    index += size;
  }

  ((uint32_t *)pOut)[0] = (uint32_t)index;

  return (uint32_t)index;
}

uint32_t rle8m_decompress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || pOut == NULL || inSize == 0 || outSize == 0)
    return 0;

  const size_t expectedInSize = ((uint32_t *)pIn)[0];
  const size_t expectedOutSize = ((uint32_t *)pIn)[1];

  if (expectedOutSize > outSize || expectedInSize > inSize)
    return 0;

  size_t index = 2 * sizeof(uint32_t);

  const uint32_t subSections = *((uint32_t *)(&pIn[index]));
  index += sizeof(uint32_t);

  if (subSections == 0)
    return 0;

  const size_t subSectionIndex = index;
  index += (subSections - 1) * sizeof(uint32_t);

  rle8_decompress_info_t decompressInfo;

  index += rle8_read_decompress_info(&pIn[index], inSize, &decompressInfo);

  const uint32_t subSectionSize = (uint32_t)(expectedOutSize / subSections);

  const uint8_t *pCIn = pIn + index;

  for (size_t i = 0; i < subSections - 1; i++)
  {
    const uint32_t sectionCompressedOffset = ((uint32_t *)(&pIn[subSectionIndex]))[i];
    const uint8_t *pEnd = pIn + sectionCompressedOffset;

    const uint32_t size = rle8_decompress_with_info(pCIn, pEnd, &decompressInfo, pOut, subSectionSize);

    if (size != subSectionSize)
      return 0;

    pCIn = pEnd;
    pOut += subSectionSize;
  }

  const size_t remainingSize = expectedOutSize - subSectionSize * (subSections - 1);

  // Decode last section.
  {
    const uint8_t *pEnd = pIn + inSize;

    const uint32_t size = rle8_decompress_with_info(pCIn, pEnd, &decompressInfo, pOut, (uint32_t)remainingSize);

    if (size != remainingSize)
      return 0;
  }

  return (uint32_t)expectedOutSize;
}

//////////////////////////////////////////////////////////////////////////

bool rle8_get_compress_info(IN const uint8_t *pIn, const uint32_t inSize, OUT rle8_compress_info_t *pCompressInfo)
{
  if (pIn == NULL || inSize == 0 || pCompressInfo == NULL)
    return false;

  uint8_t symbolsByProb[256];
  bool rle[256];

  uint32_t remaining = 256;

  // Get Probabilities.
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

    for (size_t i = 0; i < inSize; i++)
    {
      if (pIn[i] == lastSymbol)
      {
        count++;
      }
      else
      {
        prob[lastSymbol] += count;
        pcount[lastSymbol]++;
        count = 1;
        lastSymbol = pIn[i];
      }
    }

    prob[lastSymbol] += count;
    pcount[lastSymbol]++;

    for (size_t i = 0; i < 256; i++)
      if (pcount[i] > 0)
        rle[i] = (prob[i] / pcount[i]) >= 2;
      else
        rle[i] = false;

    for (int64_t i = 255; i >= 0; i--)
    {
      if (pcount[i] == 0)
      {
        consumed[i] = true;
        remaining--;
        symbolsByProb[remaining] = (uint8_t)i;
      }
    }

    for (size_t index = 0; index < remaining; index++)
    {
      uint32_t max = 0;
      size_t maxIndex = 0;

      for (size_t i = 0; i < 256; i++)
      {
        if (!consumed[i] && pcount[i] > max)
        {
          max = pcount[i];
          maxIndex = i;
        }
      }

      symbolsByProb[index] = (uint8_t)maxIndex;
      consumed[maxIndex] = true;
    }
  }

  pCompressInfo->symbolCount = (uint8_t)remaining;
  memcpy(pCompressInfo->symbolsByProb, symbolsByProb, sizeof(symbolsByProb));
  memcpy(pCompressInfo->rle, rle, sizeof(rle));

  return true;
}

bool rle8_get_compress_info_only_max_frequency(IN const uint8_t * pIn, const uint32_t inSize, OUT rle8_compress_info_t * pCompressInfo)
{
  if (pIn == NULL || inSize == 0 || pCompressInfo == NULL)
    return false;

  uint8_t symbolsByProb[256];
  bool rle[256];
  memset(rle, 0, sizeof(rle));

  uint32_t remaining = 256;

  // Get Probabilities.
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

    for (size_t i = 0; i < inSize; i++)
    {
      if (pIn[i] == lastSymbol)
      {
        count++;
      }
      else
      {
        prob[lastSymbol] += count;
        pcount[lastSymbol]++;
        count = 1;
        lastSymbol = pIn[i];
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

    if (maxBytesSaved > 0)
      rle[maxBytesSavedIndexIndex] = true;

    for (int64_t i = 255; i >= 0; i--)
    {
      if (pcount[i] == 0)
      {
        consumed[i] = true;
        remaining--;
        symbolsByProb[remaining] = (uint8_t)i;
      }
    }

    for (size_t index = 0; index < remaining; index++)
    {
      uint32_t max = 0;
      size_t maxIndex = 0;

      for (size_t i = 0; i < 256; i++)
      {
        if (!consumed[i] && pcount[i] > max)
        {
          max = pcount[i];
          maxIndex = i;
        }
      }

      symbolsByProb[index] = (uint8_t)maxIndex;
      consumed[maxIndex] = true;
    }
  }

  pCompressInfo->symbolCount = (uint8_t)remaining;
  memcpy(pCompressInfo->symbolsByProb, symbolsByProb, sizeof(symbolsByProb));
  memcpy(pCompressInfo->rle, rle, sizeof(rle));

  return true;
}

uint32_t rle8_write_compress_info(IN rle8_compress_info_t *pCompressInfo, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pCompressInfo == NULL || pOut == NULL || outSize < 256 / 8 + 256 + 1)
    return 0;

  uint32_t index = 0;

  for (size_t i = 0; i < 256 / 8; i++)
  {
    pOut[index] = 0;

    for (size_t j = 0; j < 8; j++)
      pOut[index] |= (((uint8_t)(!!pCompressInfo->rle[j + i * 8])) << j);

    index++;
  }

  pOut[index] = pCompressInfo->symbolCount;
  index++;

  size_t symbolCount = pCompressInfo->symbolCount;
  
  if (!symbolCount)
    symbolCount = 255;

  for (size_t i = 0; i < symbolCount; i++)
    pOut[index + i] = pCompressInfo->symbolsByProb[i];

  index += (uint32_t)symbolCount;

  return index;
}

uint32_t rle8_compress_with_info(IN const uint8_t *pIn, const uint32_t inSize, IN const rle8_compress_info_t *pCompressInfo, OUT uint8_t *pOut, const uint32_t outSize)
{
  if (pIn == NULL || inSize == 0 || pCompressInfo == NULL || pOut == NULL || outSize < inSize)
    return 0;

  size_t index = 0;
  size_t i = 0;
  size_t target_ = (inSize - 256);

  if (target_ > inSize)
    target_ = 0;

  const size_t target = target_;

  for (; i < target; i++)
  {
    const uint8_t b = pIn[i];

    pOut[index] = b;
    index++;

    if (pCompressInfo->rle[b])
    {
      const uint8_t range = 255;
      uint8_t count = 0;

      int32_t j = 1;

      for (; j < range; j++)
        if (pIn[j + i] == b)
          count++;
        else
          break;

      i += j - 1;

      pOut[index] = pCompressInfo->symbolsByProb[count];
      index++;
    }
  }

  for (; i < inSize; i++)
  {
    const uint8_t b = pIn[i];

    pOut[index] = b;
    index++;

    if (pCompressInfo->rle[b])
    {
      const uint8_t range = (uint8_t)min(inSize - i - 1, 255);

      uint8_t count = 0;
      size_t j = 1;

      for (; j < range; j++)
        if (pIn[j + i] == b)
          count++;
        else
          break;

      i += j - 1;

      pOut[index] = pCompressInfo->symbolsByProb[count];
      index++;
    }
  }

  return (uint32_t)index;
}

uint32_t rle8_read_decompress_info(IN const uint8_t *pIn, const uint32_t inSize, OUT rle8_decompress_info_t *pDecompressInfo)
{
  if (pIn == NULL || pDecompressInfo == NULL || inSize == 0)
    return 0;

  size_t index = 0;

  bool rle[256];

  for (size_t i = 0; i < 256 / 8; i++)
  {
    rle[i * 8 + 0] = !!(pIn[index] & 0b1);
    rle[i * 8 + 1] = !!(pIn[index] & 0b10);
    rle[i * 8 + 2] = !!(pIn[index] & 0b100);
    rle[i * 8 + 3] = !!(pIn[index] & 0b1000);
    rle[i * 8 + 4] = !!(pIn[index] & 0b10000);
    rle[i * 8 + 5] = !!(pIn[index] & 0b100000);
    rle[i * 8 + 6] = !!(pIn[index] & 0b1000000);
    rle[i * 8 + 7] = !!(pIn[index] & 0b10000000);

    index++;
  }

  uint8_t symbolToCount[256];

  {
    uint8_t symbolsByProb[256];

    size_t symbolsWithProb = pIn[index];
    index++;

    if (!symbolsWithProb)
      symbolsWithProb = 255;

    for (uint8_t i = 0; i < symbolsWithProb; i++)
    {
      symbolsByProb[i] = pIn[index];
      index++;
      symbolToCount[symbolsByProb[i]] = i;
    }

    uint8_t nextSymbol = (uint8_t)symbolsWithProb;

    for (size_t i = 0; i < 256; i++)
    {
      for (size_t j = 0; j < symbolsWithProb; j++)
        if (symbolsByProb[j] == i)
          goto next_symbol;

      symbolToCount[i] = nextSymbol;
      nextSymbol++;

    next_symbol:
      ;
    }
  }

  memcpy(pDecompressInfo->rle, rle, sizeof(rle));
  memcpy(pDecompressInfo->symbolToCount, symbolToCount, sizeof(symbolToCount));

  return (uint32_t)index;
}

//////////////////////////////////////////////////////////////////////////

const uint8_t * rle8_decompress_single_sse(IN const uint8_t *pIn, IN const uint8_t *pPreEnd, OUT uint8_t *pOut, const bool rle[256], const uint8_t symbolToCount[256], OUT uint8_t **ppOut)
{
  typedef __m128i simd_t;
#define SIMD_SIZE 16
  _STATIC_ASSERT(SIMD_SIZE == sizeof(simd_t));

  simd_t interestingSymbol;

  for (size_t i = 0; i < 256; i++)
  {
    if (rle[i])
    {
      interestingSymbol = _mm_set1_epi8((char)i);
      break;
    }
  }

  while (pIn < pPreEnd)
  {
    ALIGN(SIMD_SIZE) const simd_t data = _mm_loadu_si128((const simd_t *)pIn);
    _mm_storeu_si128((simd_t *)pOut, data);

    const int32_t contains = _mm_movemask_epi8(_mm_cmpeq_epi8(data, interestingSymbol));

    if (contains == 0)
    {
      pOut += sizeof(simd_t);
      pIn += sizeof(simd_t);
    }
    else
    {
      ALIGN(SIMD_SIZE) uint8_t dataA[sizeof(simd_t)];
      _mm_store_si128((simd_t *)dataA, data);

#ifdef _MSC_VER
      unsigned long index;
      _BitScanForward(&index, contains);
#else
      const uint32_t index = __builtin_ctz(contains);
#endif

      pIn += index + 1;
      pOut += index + 1;

      const uint8_t count = symbolToCount[*pIn];
      pIn++;

      if (count)
      {
        if (count <= sizeof(simd_t))
        {
          _mm_storeu_si128((simd_t *)pOut, interestingSymbol);
          pOut += count;
        }
        else
        {
          size_t unaligned = ((size_t)pOut & (sizeof(simd_t) - 1));
          const uint8_t *pCOut = pOut;

          if (unaligned != 0)
          {
            _mm_storeu_si128((simd_t *)pCOut, interestingSymbol);
            pCOut = (uint8_t *)((size_t)pCOut & ~(size_t)(sizeof(simd_t) - 1)) + sizeof(simd_t);
          }

          pOut += count;

          while (pCOut < pOut)
          {
            _mm_store_si128((simd_t *)pCOut, interestingSymbol);
            pCOut += sizeof(simd_t);
          }
        }
      }
    }
  }

  *ppOut = pOut;
  return pIn;
#undef SIMD_SIZE
}

#ifndef _MSC_VER
__attribute__((target("avx2")))
#endif
const uint8_t * rle8_decompress_single_avx2(IN const uint8_t *pIn, IN const uint8_t *pPreEnd, OUT uint8_t *pOut, const bool rle[256], const uint8_t symbolToCount[256], OUT uint8_t **ppOut)
{
  typedef __m256i simd_t;
#define SIMD_SIZE 32
  _STATIC_ASSERT(SIMD_SIZE == sizeof(simd_t));

  simd_t interestingSymbol;

  for (size_t i = 0; i < 256; i++)
  {
    if (rle[i])
    {
      interestingSymbol = _mm256_set1_epi8((char)i);
      break;
    }
  }

  while (pIn < pPreEnd)
  {
    ALIGN(SIMD_SIZE) const simd_t data = _mm256_loadu_si256((const simd_t *)pIn);
    _mm256_storeu_si256((simd_t *)pOut, data);

    const int32_t contains = _mm256_movemask_epi8(_mm256_cmpeq_epi8(data, interestingSymbol));

    if (contains == 0)
    {
      pOut += sizeof(simd_t);
      pIn += sizeof(simd_t);
    }
    else
    {
      ALIGN(SIMD_SIZE) uint8_t dataA[sizeof(simd_t)];
      _mm256_store_si256((simd_t *)dataA, data);

#ifdef _MSC_VER
      unsigned long index;
      _BitScanForward(&index, contains);
#else
      const uint32_t index = __builtin_ctz(contains);
#endif

      pIn += index + 1;
      pOut += index + 1;

      const uint8_t count = symbolToCount[*pIn];
      pIn++;

      if (count)
      {
        if (count <= sizeof(simd_t))
        {
          _mm256_storeu_si256((simd_t *)pOut, interestingSymbol);
          pOut += count;
        }
        else
        {
          size_t unaligned = ((size_t)pOut & (sizeof(simd_t) - 1));
          const uint8_t *pCOut = pOut;

          if (unaligned != 0)
          {
            _mm256_storeu_si256((simd_t *)pCOut, interestingSymbol);
            pCOut = (uint8_t *)((size_t)pCOut & ~(size_t)(sizeof(simd_t) - 1)) + sizeof(simd_t);
          }

          pOut += count;

          while (pCOut < pOut)
          {
            _mm256_store_si256((simd_t *)pCOut, interestingSymbol);
            pCOut += sizeof(simd_t);
          }
        }
      }
    }
  }

  *ppOut = pOut;
  return pIn;
#undef SIMD_SIZE
}

const uint8_t * rle8_decompress_multi_sse(IN const uint8_t *pIn, IN const uint8_t *pPreEnd, OUT uint8_t *pOut, const bool rle[256], const uint8_t symbolToCount[256], OUT uint8_t **ppOut)
{
  while (pIn < pPreEnd)
  {
    typedef __m128i simd_t;
#define SIMD_SIZE 16

    _STATIC_ASSERT(SIMD_SIZE == sizeof(simd_t));

    ALIGN(SIMD_SIZE) const simd_t data = _mm_loadu_si128((const simd_t *)pIn);
    _mm_storeu_si128((simd_t *)pOut, data);

    ALIGN(SIMD_SIZE) uint8_t dataA[sizeof(simd_t)];

    _mm_store_si128((simd_t *)dataA, data);

    for (size_t i = 0; i < sizeof(simd_t); i++)
    {
      if (rle[dataA[i]])
      {
        pIn += i + 1;
        pOut += i + 1;

        const uint8_t count = symbolToCount[*pIn];
        pIn++;

        if (count)
        {
          const simd_t bb = _mm_set1_epi8((char)dataA[i]);

          if (count <= sizeof(simd_t))
          {
            _mm_storeu_si128((simd_t *)pOut, bb);
            pOut += count;
          }
          else
          {
            size_t unaligned = ((size_t)pOut & (sizeof(simd_t) - 1));
            const uint8_t *pCOut = pOut;

            if (unaligned != 0)
            {
              _mm_storeu_si128((simd_t *)pCOut, bb);
              pCOut = (uint8_t *)((size_t)pCOut & ~(size_t)(sizeof(simd_t) - 1)) + sizeof(simd_t);
            }

            pOut += count;

            while (pCOut < pOut)
            {
              _mm_store_si128((simd_t *)pCOut, bb);
              pCOut += sizeof(simd_t);
            }
          }
        }

        goto symbol_found;
      }
    }

    pIn += sizeof(simd_t);
    pOut += sizeof(simd_t);

  symbol_found:
    ;
  }

  *ppOut = pOut;
  return pIn;
#undef SIMD_SIZE
}

#ifndef _MSC_VER
__attribute__((target("avx")))
#else
__declspec(noinline)
#endif
const uint8_t * rle8_decompress_multi_avx(IN const uint8_t *pIn, IN const uint8_t *pPreEnd, OUT uint8_t *pOut, const bool rle[256], const uint8_t symbolToCount[256], OUT uint8_t **ppOut)
{
  while (pIn < pPreEnd)
  {
    typedef __m256i simd_t;
#define SIMD_SIZE 32

    _STATIC_ASSERT(SIMD_SIZE == sizeof(simd_t));

    ALIGN(SIMD_SIZE) const simd_t data = _mm256_loadu_si256((const simd_t *)pIn);
    _mm256_storeu_si256((simd_t *)pOut, data);

    ALIGN(SIMD_SIZE) uint8_t dataA[sizeof(simd_t)];

    _mm256_store_si256((simd_t *)dataA, data);

    for (size_t i = 0; i < sizeof(simd_t); i++)
    {
      if (rle[dataA[i]])
      {
        pIn += i + 1;
        pOut += i + 1;

        const uint8_t count = symbolToCount[*pIn];
        pIn++;

        if (count)
        {
#if !defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER <= 1900
          const simd_t bb = _mm256_set1_epi16(dataA[i] | (dataA[i] << 8));
#else
          const simd_t bb = _mm256_set1_epi8((char)dataA[i]);
#endif

          if (count <= sizeof(simd_t))
          {
            _mm256_storeu_si256((simd_t *)pOut, bb);
            pOut += count;
          }
          else
          {
            size_t unaligned = ((size_t)pOut & (sizeof(simd_t) - 1));
            const uint8_t *pCOut = pOut;

            if (unaligned != 0)
            {
              _mm256_storeu_si256((simd_t *)pCOut, bb);
              pCOut = (uint8_t *)((size_t)pCOut & ~(size_t)(sizeof(simd_t) - 1)) + sizeof(simd_t);
            }

            pOut += count;

            while (pCOut < pOut)
            {
              _mm256_store_si256((simd_t *)pCOut, bb);
              pCOut += sizeof(simd_t);
            }
          }
        }

        goto symbol_found;
      }
    }

    pIn += sizeof(simd_t);
    pOut += sizeof(simd_t);

  symbol_found:
    ;
  }

  *ppOut = pOut;
  return pIn;
#undef SIMD_SIZE
}

uint32_t rle8_decompress_with_info(IN const uint8_t *pIn, IN const uint8_t *pEnd, IN const rle8_decompress_info_t *pDecompressInfo, OUT uint8_t *pOut, const uint32_t expectedOutSize)
{
  bool rle[256];
  uint8_t symbolToCount[256];

  memcpy(rle, pDecompressInfo->rle, sizeof(rle));
  memcpy(symbolToCount, pDecompressInfo->symbolToCount, sizeof(symbolToCount));

  const uint8_t *pPreEnd = pEnd - 256;

  uint8_t rleSymbolCount = 0;

  for (size_t i = 0; i < 256; i++)
    rleSymbolCount += rle[i];

  if (rleSymbolCount == 0 || expectedOutSize == (uint32_t)(pEnd - pIn))
  {
    memcpy(pOut, pIn, expectedOutSize);
    return (uint32_t)expectedOutSize;
  }
  else if (rleSymbolCount == 1)
  {
    _DetectCPUFeatures();

    if (avx2Supported)
      pIn = rle8_decompress_single_avx2(pIn, pPreEnd, pOut, rle, symbolToCount, &pOut);
    else
      pIn = rle8_decompress_single_sse(pIn, pPreEnd, pOut, rle, symbolToCount, &pOut);
  }
  else
  {
    _DetectCPUFeatures();

    if (avxSupported)
      pIn = rle8_decompress_multi_avx(pIn, pPreEnd, pOut, rle, symbolToCount, &pOut);
    else
      pIn = rle8_decompress_multi_sse(pIn, pPreEnd, pOut, rle, symbolToCount, &pOut);
  }

  while (pIn < pEnd)
  {
    const uint8_t b = *pOut = *pIn;
    pIn++;
    pOut++;

    if (rle[b])
    {
      const uint8_t count = symbolToCount[*pIn];
      pIn++;

      if (count)
      {
        if (count < 16)
        {
          for (size_t i = 0; i < count; i++)
          {
            *pOut = b;
            pOut++;
          }
        }
        else
        {
          size_t countRemaining = count;
          size_t unaligned = ((size_t)pOut & (sizeof(__m128i) - 1));
          const __m128i bb = _mm_set1_epi8((char)b);

          if (unaligned != 0)
          {
            _mm_storeu_si128((__m128i *)pOut, bb);
            uint8_t *pPrevOut = pOut;
            pOut = (uint8_t *)((size_t)pOut & ~(size_t)(sizeof(__m128i) - 1)) + sizeof(__m128i);
            countRemaining -= (pOut - pPrevOut);
          }

          while (countRemaining > (sizeof(__m128i) - 1))
          {
            _mm_store_si128((__m128i *)pOut, bb);
            pOut += sizeof(__m128i);
            countRemaining -= sizeof(__m128i);
          }

          if (countRemaining != 0)
          {
            _mm_storeu_si128((__m128i *)(pOut - (sizeof(__m128i) - countRemaining)), bb);
            pOut += countRemaining;
          }
        }
      }
    }
  }

  return (uint32_t)expectedOutSize;
}
