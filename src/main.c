#include "rle8.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

const char ArgumentTo[] = "-to";
const char ArgumentSubSections[] = "-s";
const char ArgumentRuns[] = "-r";
const char ArgumentSingle[] = "--single";
const char ArgumentUltra[] = "--ultra";
const char ArgumentExtreme[] = "--extreme";

uint64_t GetCurrentTimeNs();

//////////////////////////////////////////////////////////////////////////

int main(int argc, char **pArgv)
{
  if (argc <= 1)
  {
    printf("Usage: rle8 <InputFileName> [%s <Output File Name>][%s <Sub Section Count>][%s <Run Count>][%s (only rle most frequent symbol)][%s (for shorter strings of rle-symbols) | %s (for very fast decoding)]\n", ArgumentTo, ArgumentSubSections, ArgumentRuns, ArgumentSingle, ArgumentUltra, ArgumentExtreme);
    return 1;
  }

#ifdef _WIN32
  // For more consistent benchmarking results.
  HANDLE thread = GetCurrentThread();
  SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
  SetThreadAffinityMask(thread, 1);
#endif

  const char *outputFileName = NULL;
  int32_t subSections = 0;
  int32_t runs = 1;
  bool singleSymbol = false;
  bool ultraMode = false;
  bool extremeMode = false;

  if (argc > 2)
  {
    size_t argIndex = 2;
    size_t argsRemaining = (size_t)argc - 2;

    while (argsRemaining)
    {
      if (argsRemaining >= 2 && strncmp(pArgv[argIndex], ArgumentTo, sizeof(ArgumentTo)) == 0)
      {
        outputFileName = pArgv[argIndex + 1];
        argIndex += 2;
        argsRemaining -= 2;
      }
      else if (argsRemaining >= 2 && strncmp(pArgv[argIndex], ArgumentSubSections, sizeof(ArgumentSubSections)) == 0)
      {
        subSections = atoi(pArgv[argIndex + 1]);

        if (subSections <= 0)
        {
          puts("Invalid Parameter.");
          return 1;
        }

        argIndex += 2;
        argsRemaining -= 2;
      }
      else if (argsRemaining >= 2 && strncmp(pArgv[argIndex], ArgumentRuns, sizeof(ArgumentRuns)) == 0)
      {
        runs = atoi(pArgv[argIndex + 1]);

        if (runs <= 0)
        {
          puts("Invalid Parameter.");
          return 1;
        }

        argIndex += 2;
        argsRemaining -= 2;
      }
      else if (argsRemaining >= 1 && strncmp(pArgv[argIndex], ArgumentSingle, sizeof(ArgumentSingle)) == 0)
      {
        singleSymbol = true;
        argIndex += 1;
        argsRemaining -= 1;
      }
      else if (argsRemaining >= 1 && strncmp(pArgv[argIndex], ArgumentUltra, sizeof(ArgumentUltra)) == 0)
      {
        ultraMode = true;
        argIndex += 1;
        argsRemaining -= 1;
      }
      else if (argsRemaining >= 1 && strncmp(pArgv[argIndex], ArgumentExtreme, sizeof(ArgumentExtreme)) == 0)
      {
        extremeMode = true;
        argIndex += 1;
        argsRemaining -= 1;
      }
      else
      {
        puts("Invalid Parameter.");
        return 1;
      }
    }
  }

  if (singleSymbol && subSections != 0)
  {
    puts("Single Symbol Encoding is only available without sub sections.");
    return 1;
  }

  if (ultraMode && subSections != 0)
  {
    puts("Ultra Mode Encoding is only available without sub sections.");
    return 1;
  }

  if (extremeMode && subSections != 0)
  {
    puts("Extreme Mode Encoding is only available without sub sections.");
    return 1;
  }

  if (ultraMode && extremeMode)
  {
    puts("Extreme Mode and Ultra Mode cannot be used at the same time.");
    return 1;
  }

  int32_t fileSize = 0;
  uint32_t compressedBufferSize = 0;
  uint8_t *pUncompressedData = NULL;
  uint8_t *pDecompressedData = NULL;
  uint8_t *pCompressedData = NULL;

  FILE *pFile = fopen(pArgv[1], "rb");

  if (!pFile)
  {
    puts("Failed to read file.");
    goto epilogue;
  }

  fseek(pFile, 0, SEEK_END);
  fileSize = ftell(pFile);

  if (fileSize <= 0)
  {
    puts("Invalid File size / failed to read file.");
    goto epilogue;
  }

  fseek(pFile, 0, SEEK_SET);

  if (subSections == 0)
  {
    if (!extremeMode)
      compressedBufferSize = rle8_compress_bounds((uint32_t)fileSize);
    else
      compressedBufferSize = rle8_extreme_compress_bounds((uint32_t)fileSize);
  }
  else
  {
    compressedBufferSize = rle8m_compress_bounds((uint32_t)subSections, (uint32_t)fileSize);
  }

  pUncompressedData = (uint8_t *)malloc((size_t)fileSize);
  pDecompressedData = (uint8_t *)malloc((size_t)fileSize + rle8_extreme_decompress_additional_size());
  pCompressedData = (uint8_t *)malloc((size_t)compressedBufferSize + rle8_extreme_decompress_additional_size());

  if (!pUncompressedData || !pDecompressedData || !pCompressedData)
  {
    puts("Failed to allocate memory.");
    goto epilogue;
  }

  if (fileSize != fread(pUncompressedData, 1, (size_t)fileSize, pFile))
  {
    puts("Failed to read file.");
    goto epilogue;
  }

  //////////////////////////////////////////////////////////////////////////

  // Compress.
  {
    uint32_t decompressedSize = 0;
    uint32_t compressedSize = 0;

    uint64_t time = GetCurrentTimeNs();

    for (int32_t i = 0; i < runs; i++)
    {
      if (subSections == 0)
      {
        if (!ultraMode && !extremeMode)
        {
          if (singleSymbol)
            compressedSize = rle8_compress_only_max_frequency(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
          else
            compressedSize = rle8_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
        }
        else if (ultraMode)
        {
          if (singleSymbol)
            compressedSize = rle8_ultra_compress_only_max_frequency(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
          else
            compressedSize = rle8_ultra_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
        }
        else if (extremeMode)
        {
          if (singleSymbol)
            compressedSize = rle8_extreme_single_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
          else
            compressedSize = rle8_extreme_multi_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
        }
      }
      else
      {
        compressedSize = rle8m_compress((uint32_t)subSections, pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
      }
    }

    time = GetCurrentTimeNs() - time;

    if (0 == compressedSize)
    {
      puts("Failed to compress file.");
      goto epilogue;
    }

    printf("Compressed %" PRIi32 " bytes -> %" PRIu32 " bytes (%f %%) in %f ms. (=> %f MB/s)\n", fileSize, compressedSize, (double)compressedSize / (double)fileSize * 100.0, time / (double)runs / 1000000.0, (fileSize / (1024.0 * 1024.0)) / (time / (double)runs / 1000000000.0));

    if (outputFileName)
    {
      FILE *pCompressed = fopen(outputFileName, "wb");
      
      if (!pCompressed)
      {
        puts("Failed to open file for writing.");
        fclose(pCompressed);
        goto epilogue;
      }

      if (compressedSize != fwrite(pCompressedData, 1, compressedSize, pCompressed))
      {
        puts("Failed to write to file.");
        fclose(pCompressed);
        goto epilogue;
      }

      fclose(pCompressed);
    }

    time = GetCurrentTimeNs();

    for (int32_t i = 0; i < runs; i++)
    {
      if (subSections == 0)
      {
        if (!ultraMode && !extremeMode)
          decompressedSize = rle8_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
        else if (!extremeMode)
          decompressedSize = rle8_ultra_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
        else
          decompressedSize = rle8_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
      }
      else
      {
        decompressedSize = rle8m_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
      }
    }

    time = GetCurrentTimeNs() - time;

    if ((uint32_t)fileSize != decompressedSize)
    {
      puts("Failed to decompress file.");
      goto epilogue;
    }
    
    printf("Decompressed in %f ms. (=> %f MB/s)\n", time / (double)runs / 1000000.0, (fileSize / (1024.0 * 1024.0)) / (time / (double)runs / 1000000000.0));

    if (memcmp(pUncompressedData, pDecompressedData, (size_t)fileSize) != 0)
    {
      puts("Validation Failed.");

      for (size_t i = 0; i < fileSize; i++)
      {
        if (pUncompressedData[i] != pDecompressedData[i])
        {
          printf("First invalid char at %" PRIu64 " (0x%" PRIx8 " != 0x%" PRIx8 ").\n", i, pUncompressedData[i], pDecompressedData[i]);
          break;
        }
      }

      goto epilogue;
    }

    if (subSections > 0)
    {
      memset(pDecompressedData, 0, fileSize);

      if (!rle8m_opencl_init(fileSize, compressedSize, subSections))
      {
        puts("Initialization Failed (OpenCL).");
        goto epilogue;
      }

      time = GetCurrentTimeNs();

      for (int32_t i = 0; i < runs; i++)
        decompressedSize = rle8m_opencl_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);

      time = GetCurrentTimeNs() - time;

      rle8m_opencl_destroy();

      if ((uint32_t)fileSize != decompressedSize)
      {
        puts("Failed to decompress file (OpenCL).");
        goto epilogue;
      }

      printf("Decompressed in %f ms (OpenCL).\n", time / (double)runs / 1000000.0);

      if (memcmp(pUncompressedData, pDecompressedData, (size_t)fileSize) != 0)
      {
        puts("Validation Failed. (OpenCL)");
        goto epilogue;
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////

  goto epilogue;

epilogue:
  fclose(pFile);
  free(pUncompressedData);
  free(pDecompressedData);
  free(pCompressedData);

  return 0;
}

//////////////////////////////////////////////////////////////////////////

uint64_t GetCurrentTimeNs()
{
#ifdef WIN32
  FILETIME time;
  GetSystemTimePreciseAsFileTime(&time);

  return ((uint64_t)time.dwLowDateTime | ((uint64_t)time.dwHighDateTime << 32)) * 100;
#else
  struct timespec time;
  clock_gettime(CLOCK_REALTIME, &time);

  return (uint64_t)time.tv_sec * 1000000000 + (uint64_t)time.tv_nsec;
#endif
}