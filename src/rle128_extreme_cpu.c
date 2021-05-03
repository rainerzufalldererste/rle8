#include "rle8.h"
#include "rleX_extreme_common.h"

#define RLE128_EXTREME_MULTI_MIN_RANGE_SHORT ((16 + 1 + 1) + 2)
#define RLE128_EXTREME_MULTI_MIN_RANGE_LONG ((16 + 1 + 4 + 1 + 4) + 2)

#ifndef _MSC_VER
#define popcnt16 __builtin_popcount
#define popcnt32 __builtin_popcountl
#define popcnt64 __builtin_popcountll
#else
#define popcnt16 __popcnt16
#define popcnt32 __popcnt
#define popcnt64 __popcnt64
#endif

uint32_t rle128_extreme_compress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
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

	typedef __m128i symbol_t;
	const size_t symbolSize = sizeof(symbol_t);

	int64_t count = 0;
	symbol_t symbol = _mm_xor_si128(_mm_setzero_si128(), _mm_loadu_si128((const symbol_t *)pIn));

	for (; i < inSize; i++)
	{
		if (count && _mm_movemask_epi8(_mm_cmpeq_epi8(symbol, _mm_loadu_si128((const symbol_t *)&pIn[i]))) == 0xFFFF && i + symbolSize <= inSize)
		{
			count += symbolSize;
			i += symbolSize - 1;
		}
		else
		{
			{
				const int64_t range = i - lastRLE - count + 1;

				if (range <= 255 && count >= RLE128_EXTREME_MULTI_MIN_RANGE_SHORT)
				{
					_mm_store_si128((symbol_t *)(&pOut[index]), symbol);
					index += symbolSize;

					const int64_t storedCount = (count / symbolSize) - (RLE128_EXTREME_MULTI_MIN_RANGE_SHORT / symbolSize) + 1;

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
				else if (count >= RLE128_EXTREME_MULTI_MIN_RANGE_LONG)
				{
					_mm_store_si128((symbol_t *)(&pOut[index]), symbol);
					index += symbolSize;

					const int64_t storedCount = (count / symbolSize) - (RLE128_EXTREME_MULTI_MIN_RANGE_SHORT / symbolSize) + 1;

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

			symbol = _mm_loadu_si128((const symbol_t *)& pIn[i]);

			if (i + symbolSize <= inSize && _mm_movemask_epi8(_mm_cmpeq_epi8(symbol, _mm_loadu_si128((const symbol_t *)((&pIn[i]) + symbolSize)))) == 0xFFFF)
			{
				count = symbolSize * 2;
				i += symbolSize * 2 - 1;
			}
			else
			{
				count = 0;
			}
		}
	}

	{
		const int64_t range = i - lastRLE - count + 1;

		if (range <= 255 && count >= RLE128_EXTREME_MULTI_MIN_RANGE_SHORT)
		{
			_mm_store_si128((symbol_t *)(&pOut[index]), symbol);
			index += symbolSize;

			const int64_t storedCount = (count / symbolSize) - (RLE128_EXTREME_MULTI_MIN_RANGE_SHORT / symbolSize) + 1;

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

			_mm_store_si128((symbol_t *)(&pOut[index]), _mm_setzero_si128());
			index += symbolSize;
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
		else if (count >= RLE128_EXTREME_MULTI_MIN_RANGE_LONG)
		{
			_mm_store_si128((symbol_t *)(&pOut[index]), symbol);
			index += symbolSize;

			const int64_t storedCount = (count / symbolSize) - (RLE128_EXTREME_MULTI_MIN_RANGE_SHORT / symbolSize) + 1;

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

			_mm_store_si128((symbol_t *)(&pOut[index]), _mm_setzero_si128());
			index += symbolSize;
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
			_mm_store_si128((symbol_t *)(&pOut[index]), _mm_setzero_si128());
			index += symbolSize;
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

void rle128_extreme_decompress_sse(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
	size_t offset, symbolCount;
	__m128i symbol;

	typedef __m128i symbol_t;

	while (true)
	{
		symbol = _mm_loadu_si128((const symbol_t *)pInStart);
		pInStart += sizeof(symbol_t);
		symbolCount = (size_t)* pInStart;
		pInStart++;

		if (symbolCount == 0)
		{
			symbolCount = *(uint32_t *)pInStart;
			pInStart += sizeof(uint32_t);
		}

		offset = (size_t)* pInStart;
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

		symbolCount = (symbolCount + (RLE128_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) - 1) * sizeof(symbol_t);

		// memset.
		MEMSET_SSE_MULTI;
	}
}

#ifndef _MSC_VER
__attribute__((target("avx")))
#endif
void rle128_extreme_decompress_avx(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
	size_t offset, symbolCount;
	__m256i symbol;

	typedef __m128i symbol_t;

	while (true)
	{
		symbol = _mm256_castps_si256(_mm256_broadcast_ps((const __m128 *)pInStart));
		pInStart += sizeof(symbol_t);
		symbolCount = (size_t)* pInStart;
		pInStart++;

		if (symbolCount == 0)
		{
			symbolCount = *(uint32_t *)pInStart;
			pInStart += sizeof(uint32_t);
		}

		offset = (size_t)* pInStart;
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

		symbolCount = (symbolCount + (RLE128_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) - 1) * sizeof(symbol_t);

		// memset.
		MEMSET_AVX_MULTI;
	}
}

#ifndef _MSC_VER
__attribute__((target("avx512f")))
#endif
void rle128_extreme_decompress_avx512f(IN const uint8_t *pInStart, OUT uint8_t *pOut)
{
	size_t offset, symbolCount;
	__m512i symbol;

	typedef __m128i symbol_t;

	while (true)
	{
		symbol = _mm512_castps_si512(_mm512_broadcast_f32x4(_mm_loadu_ps((const float *)pInStart)));
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

		symbolCount = (symbolCount + (RLE128_EXTREME_MULTI_MIN_RANGE_SHORT / sizeof(symbol_t)) - 1) * sizeof(symbol_t);

		// memset.
		MEMSET_AVX512_MULTI;
	}
}

uint32_t rle128_extreme_decompress(IN const uint8_t *pIn, const uint32_t inSize, OUT uint8_t *pOut, const uint32_t outSize)
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
		rle128_extreme_decompress_avx512f(pIn, pOut);
	else if (avxSupported)
		rle128_extreme_decompress_avx(pIn, pOut);
	else
		rle128_extreme_decompress_sse(pIn, pOut);

	return (uint32_t)expectedOutSize;
}
