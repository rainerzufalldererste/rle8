#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rle8.h"

const char ArgumentTo[] = "--to";
const char ArgumentSubSections[] = "--sub-sections";
const char ArgumentRuns[] = "--runs";
const char ArgumentNormal[] = "--normal";
const char ArgumentSingle[] = "--single";
const char ArgumentUltra[] = "--ultra";
const char ArgumentExtreme[] = "--extreme";
const char ArgumentExtremeSize[] = "--x-size";
const char ArgumentMinimumTime[] = "--min-time";

uint64_t GetCurrentTimeNs();
bool Validate(const uint8_t *pUncompressedData, const uint8_t *pDecompressedData, const size_t fileSize);
double GetInformationRatio(const uint8_t *pData, const size_t length);

//////////////////////////////////////////////////////////////////////////

int main(int argc, char **pArgv)
{
  if (argc <= 1)
  {
    printf("Usage: rle8 <InputFileName>\n\n\t[%s <Run Count>]\n\t[%s <Minimum Benchmark Time in Seconds>]\n\n\tOR:\n\n\t[%s <Output File Name>]\n\n\t[%s]\n\t\tif '%s': [%s <Sub Section Count>]\n\n\t[%s (for shorter strings of rle-symbols)]\n\n\t[%s (for very fast decoding)]\n\t\tif '%s': [%s (8 | 16 | 24 | 32 | 48 | 64 | 128)] (symbol size)\n\n\t[%s (only rle most frequent symbol, only available for 8 bit modes)]\n\n\t[%s <Run Count>]\n", ArgumentRuns, ArgumentMinimumTime, ArgumentTo, ArgumentNormal, ArgumentNormal, ArgumentSubSections, ArgumentUltra, ArgumentExtreme, ArgumentExtreme, ArgumentExtremeSize, ArgumentSingle, ArgumentRuns);
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
  int32_t runs = 8;
  int32_t minSeconds = 2;
  bool normalMode = false;
  bool singleSymbol = false;
  bool ultraMode = false;
  bool extremeMode = false;
  bool benchmarkAll = false;
  uint8_t extremeSize = 8;

  // Parse Parameters.
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
      else if (argsRemaining >= 2 && strncmp(pArgv[argIndex], ArgumentMinimumTime, sizeof(ArgumentMinimumTime)) == 0)
      {
        minSeconds = atoi(pArgv[argIndex + 1]);

        if (minSeconds < 0)
        {
          puts("Invalid Parameter.");
          return 1;
        }

        argIndex += 2;
        argsRemaining -= 2;
      }
      else if (argsRemaining >= 1 && strncmp(pArgv[argIndex], ArgumentNormal, sizeof(ArgumentNormal)) == 0)
      {
        singleSymbol = true;
        argIndex += 1;
        argsRemaining -= 1;
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
      else if (argsRemaining >= 2 && strncmp(pArgv[argIndex], ArgumentExtremeSize, sizeof(ArgumentExtremeSize)) == 0)
      {
        extremeMode = true;
        
        switch (pArgv[argIndex + 1][0])
        {
        case '8':
          extremeSize = 8;
          break;

        case '1':
          switch (pArgv[argIndex + 1][1])
          {
          case '6':
            extremeSize = 16;
            break;

          case '2':
            extremeSize = 128;
            break;

          default:
            puts("Invalid Parameter.");
            return 1;
          }
          break;

        case '2':
          extremeSize = 24;
          break;

        case '3':
          extremeSize = 32;
          break;

        case '4':
          extremeSize = 48;
          break;

        case '6':
          extremeSize = 64;
          break;

        default:
          puts("Invalid Parameter.");
          return 1;
        }

        argIndex += 2;
        argsRemaining -= 2;
      }
      else
      {
        puts("Invalid Parameter.");
        return 1;
      }
    }
  }

  // Validate Parameters.
  {
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

    if (normalMode && ultraMode && extremeMode)
    {
      puts("Normal, Extreme and Ultra cannot be used at the same time.");
      return 1;
    }

    if (extremeMode && singleSymbol && extremeSize != 8)
    {
      puts("Single Symbol in Extreme Mode is only supported for symbol size 8.");
      return 1;
    }

    benchmarkAll = !normalMode && !ultraMode && !extremeMode;
  }

  size_t fileSize = 0;
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

  compressedBufferSize = max(rle8_compress_bounds((uint32_t)fileSize), rle8_extreme_compress_bounds((uint32_t)fileSize));
  
  if (subSections != 0)
    compressedBufferSize = max(compressedBufferSize, rle8m_compress_bounds((uint32_t)subSections, (uint32_t)fileSize));

  pUncompressedData = (uint8_t *)malloc(fileSize);
  pDecompressedData = (uint8_t *)malloc(fileSize + rle8_extreme_decompress_additional_size());
  pCompressedData = (uint8_t *)malloc(compressedBufferSize);

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

  if (benchmarkAll)
  {
    enum
    {
      Normal,
      NormalSingle,
      Ultra,
      UltraSingle,
      Extreme8,
      Extreme8Single,
      Extreme16,
      Extreme24,
      Extreme32,
      Extreme48,
      Extreme64,
      Extreme128,

      MemCopy,

      CodecCount
    } currentCodec = 0;

    const char *codecNames[] = 
    {
      "Normal               ",
      "Normal Single        ",
      "Ultra                ",
      "Ultra Single         ",
      "Extreme 8 Bit        ",
      "Extreme 8 Bit Single ",
      "Extreme 16 Bit       ",
      "Extreme 24 Bit       ",
      "Extreme 32 Bit       ",
      "Extreme 48 Bit       ",
      "Extreme 64 Bit       ",
      "Extreme 128 Bit      ",
      "memcpy               ",
    };

    _STATIC_ASSERT(ARRAYSIZE(codecNames) == CodecCount);

    uint32_t fileSize32 = (uint32_t)fileSize;

    printf("\nBenchmarking File '%s' (%" PRIu64 " Bytes)\n\n"
      "Codec                  Ratio      Encoder Throughput (Maximum)    Decoder Throughput (Maximum)    R*H/log2(|S|)\n"
      "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", pArgv[1], fileSize);

    for (; currentCodec < CodecCount; currentCodec++)
    {
      printf("%s|          | (dry run)", codecNames[currentCodec]);

      uint64_t compressionTime = 0;
      uint64_t fastestCompresionTime = UINT64_MAX;
      int64_t compressionRuns = -1;
      uint32_t compressedSize = 0;

      const uint64_t compressStartNs = GetCurrentTimeNs();

      while (compressionRuns < (int64_t)runs || (GetCurrentTimeNs() - compressStartNs) < 1000000000 * (uint64_t)minSeconds)
      {
        uint64_t runTime = GetCurrentTimeNs();

        switch (currentCodec)
        {
        case Normal:
          compressedSize = rle8_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case NormalSingle:
          compressedSize = rle8_compress_only_max_frequency(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Ultra:
          compressedSize = rle8_ultra_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case UltraSingle:
          compressedSize = rle8_ultra_compress_only_max_frequency(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme8:
          compressedSize = rle8_extreme_multi_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme8Single:
          compressedSize = rle8_extreme_single_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme16:
          compressedSize = rle16_extreme_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme24:
          compressedSize = rle24_extreme_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme32:
          compressedSize = rle32_extreme_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme48:
          compressedSize = rle48_extreme_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme64:
          compressedSize = rle64_extreme_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;
    
        case Extreme128:
          compressedSize = rle128_extreme_compress(pUncompressedData, fileSize32, pCompressedData, compressedBufferSize);
          break;

        case MemCopy:
          compressedSize = fileSize32;
          memcpy(pCompressedData, pUncompressedData, fileSize);
          break;
        }

        runTime = GetCurrentTimeNs() - runTime;

        if (compressedSize == 0)
          break;

        if (compressionRuns >= 0)
          compressionTime += runTime;

        if (runTime < fastestCompresionTime)
          fastestCompresionTime = runTime;

        compressionRuns++;

        if (compressionRuns > 0)
          printf("\r%s| %6.2f %% | %7.1f MiB/s (%7.1f MiB/s)", codecNames[currentCodec], compressedSize / (double)fileSize * 100.0, (fileSize * (double)compressionRuns / (double)(1024 * 1024)) / (compressionTime / 1000000000.0), (fileSize / (double)(1024 * 1024)) / (fastestCompresionTime / 1000000000.0));
      }

      if (compressedSize == 0)
      {
        printf("\r%s| <FAILED TO COMRPESS>\n", codecNames[currentCodec]);
        continue;
      }

      int64_t decompressionRuns = -1;
      uint64_t decompressionTime = 0;
      uint64_t fastestDecompresionTime = UINT64_MAX;
      uint32_t decompressedSize = 0;

      printf("\r%s| %6.2f %% | %7.1f MiB/s (%7.1f MiB/s) | (dry run)", codecNames[currentCodec], compressedSize / (double)fileSize * 100.0, (fileSize * (double)compressionRuns / (double)(1024 * 1024)) / (compressionTime / 1000000000.0), (fileSize / (double)(1024 * 1024)) / (fastestCompresionTime / 1000000000.0));

      const uint64_t decompressStartNs = GetCurrentTimeNs();

      while (decompressionRuns < (int64_t)runs || (GetCurrentTimeNs() - decompressStartNs) < 1000000000 * (uint64_t)minSeconds)
      {
        uint64_t runTime = GetCurrentTimeNs();

        switch (currentCodec)
        {
        case Normal:
        case NormalSingle:
          decompressedSize = rle8_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Ultra:
        case UltraSingle:
          decompressedSize = rle8_ultra_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Extreme8:
        case Extreme8Single:
          decompressedSize = rle8_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Extreme16:
          decompressedSize = rle16_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Extreme24:
          decompressedSize = rle24_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Extreme32:
          decompressedSize = rle32_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Extreme48:
          decompressedSize = rle48_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Extreme64:
          decompressedSize = rle64_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;
     
        case Extreme128:
          decompressedSize = rle128_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, compressedBufferSize);
          break;

        case MemCopy:
          decompressedSize = fileSize32;
          memcpy(pDecompressedData, pCompressedData, fileSize);
          break;
        }

        runTime = GetCurrentTimeNs() - runTime;

        if (decompressedSize == 0)
          break;

        if (decompressionRuns >= 0)
          decompressionTime += runTime;

        if (runTime < fastestDecompresionTime)
          fastestDecompresionTime = runTime;

        decompressionRuns++;

        if (decompressionRuns > 0)
          printf("\r%s| %6.2f %% | %7.1f MiB/s (%7.1f MiB/s) | %7.1f MiB/s (%7.1f MiB/s)", codecNames[currentCodec], compressedSize / (double)fileSize * 100.0, (fileSize * (double)compressionRuns / (double)(1024 * 1024)) / (compressionTime / 1000000000.0), (fileSize / (double)(1024 * 1024)) / (fastestCompresionTime / 1000000000.0), (fileSize * (double)decompressionRuns / (double)(1024 * 1024)) / (decompressionTime / 1000000000.0), (fileSize / (double)(1024 * 1024)) / (fastestDecompresionTime / 1000000000.0));
      }

      if (decompressedSize == 0)
      {
        printf("\r%s| %6.2f %% | %7.1f MiB/s (%7.1f MiB/s) | <FAILED TO DECOMRPESS>\n", codecNames[currentCodec], compressedSize / (double)fileSize * 100.0, (fileSize * (double)compressionRuns / (double)(1024 * 1024)) / (compressionTime / 1000000000.0), (fileSize / (double)(1024 * 1024)) / (fastestCompresionTime / 1000000000.0));

        continue;
      }

      printf("\r%s| %6.2f %% | %7.1f MiB/s (%7.1f MiB/s) | %7.1f MiB/s (%7.1f MiB/s) | %11.7f %%", codecNames[currentCodec], compressedSize / (double)fileSize * 100.0, (fileSize * (double)compressionRuns / (double)(1024 * 1024)) / (compressionTime / 1000000000.0), (fileSize / (double)(1024 * 1024)) / (fastestCompresionTime / 1000000000.0), (fileSize * (double)decompressionRuns / (double)(1024 * 1024)) / (decompressionTime / 1000000000.0), (fileSize / (double)(1024 * 1024)) / (fastestDecompresionTime / 1000000000.0), ((compressedSize / (double)fileSize) * (GetInformationRatio(pCompressedData, compressedSize))) * 100.0f);

      puts("");

      Validate(pUncompressedData, pDecompressedData, fileSize);
    }
  }
  else // !benchmarkAll
  {
    // Print Codec Description.
    if (!benchmarkAll)
    {
      printf("Mode: rle8 ");

      if (normalMode)
        fputs("Normal ", stdout);
      else if (extremeMode)
        fputs("Extreme ", stdout);
      else
        fputs("Ultra ", stdout);

      if ((!extremeMode || extremeSize == 8) && singleSymbol)
        fputs("Single-Symbol-Mode ", stdout);

      if (extremeMode)
        printf("with %" PRIu8 " Bit Symbols ", extremeSize);

      printf("(%" PRIi32 " Run%s)\n\n", runs, runs > 1 ? "s" : "");
    }

    uint32_t decompressedSize = 0;
    uint32_t compressedSize = 0;

    uint64_t subTimeMin = UINT64_MAX;
    uint64_t subTimeMax = 0;

    uint64_t time = GetCurrentTimeNs();

    for (int32_t i = 0; i < runs; i++)
    {
      uint64_t subTime = GetCurrentTimeNs();

      if (subSections == 0)
      {
        if (normalMode)
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
          {
            compressedSize = rle8_extreme_single_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
          }
          else
          {
            switch (extremeSize)
            {
            case 8:
            default:
              compressedSize = rle8_extreme_multi_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
              break;

            case 16:
              compressedSize = rle16_extreme_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
              break;

            case 24:
              compressedSize = rle24_extreme_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
              break;

            case 32:
              compressedSize = rle32_extreme_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
              break;

            case 48:
              compressedSize = rle48_extreme_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
              break;

            case 64:
              compressedSize = rle64_extreme_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
              break;

            case 128:
              compressedSize = rle128_extreme_compress(pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
              break;
            }
          }
        }
      }
      else
      {
        compressedSize = rle8m_compress((uint32_t)subSections, pUncompressedData, (uint32_t)fileSize, pCompressedData, compressedBufferSize);
      }

      subTime = GetCurrentTimeNs() - subTime;

      if (subTime < subTimeMin)
        subTimeMin = subTime;

      if (subTime > subTimeMax)
        subTimeMax = subTime;
    }

    time = GetCurrentTimeNs() - time;

    if (0 == compressedSize)
    {
      puts("Failed to compress file.");
      goto epilogue;
    }

    printf("Compressed %" PRIu64 " bytes -> %" PRIu32 " bytes (%f %%) in %f ms. (=> %f MB/s)\n", fileSize, compressedSize, (double)compressedSize / (double)fileSize * 100.0, time / (double)runs / 1000000.0, (fileSize / (1024.0 * 1024.0)) / (time / (double)runs / 1000000000.0));

    if (runs > 1)
      printf(" [%f ms .. %f ms | %f MB/s .. %f MB/s]\n\n", subTimeMin / 1000000.0, subTimeMax / 1000000.0, (fileSize / (1024.0 * 1024.0)) / (subTimeMax / 1000000000.0), (fileSize / (1024.0 * 1024.0)) / (subTimeMin / 1000000000.0));

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

    // Scramble data outside the claimed length, to ensure the compressors are telling the truth about the claimed length.
    for (size_t i = compressedSize; i < compressedBufferSize; i++)
      pCompressedData[i] = ~pCompressedData[i];

    subTimeMin = UINT64_MAX;
    subTimeMax = 0;

    time = GetCurrentTimeNs();

    for (int32_t i = 0; i < runs; i++)
    {
      uint64_t subTime = GetCurrentTimeNs();

      if (subSections == 0)
      {
        if (normalMode)
        {
          decompressedSize = rle8_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
        }
        else if (!extremeMode)
        {
          decompressedSize = rle8_ultra_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
        }
        else
        {
          switch (extremeSize)
          {
          case 8:
          default:
            decompressedSize = rle8_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
            break;

          case 16:
            decompressedSize = rle16_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
            break;

          case 24:
            decompressedSize = rle24_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
            break;

          case 32:
            decompressedSize = rle32_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
            break;

          case 48:
            decompressedSize = rle48_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
            break;

          case 64:
            decompressedSize = rle64_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
            break;

          case 128:
            decompressedSize = rle128_extreme_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
            break;
          }
        }
      }
      else
      {
        decompressedSize = rle8m_decompress(pCompressedData, compressedSize, pDecompressedData, (uint32_t)fileSize);
      }

      subTime = GetCurrentTimeNs() - subTime;

      if (subTime < subTimeMin)
        subTimeMin = subTime;

      if (subTime > subTimeMax)
        subTimeMax = subTime;
    }

    time = GetCurrentTimeNs() - time;

    if ((uint32_t)fileSize != decompressedSize)
    {
      puts("Failed to decompress file.");
      goto epilogue;
    }
    
    printf("Decompressed in %f ms. (=> %f MB/s)\n", time / (double)runs / 1000000.0, (fileSize / (1024.0 * 1024.0)) / (time / (double)runs / 1000000000.0));

    if (runs > 1)
      printf(" [%f ms .. %f ms | %f MB/s .. %f MB/s]\n\n", subTimeMin / 1000000.0, subTimeMax / 1000000.0, (fileSize / (1024.0 * 1024.0)) / (subTimeMax / 1000000000.0), (fileSize / (1024.0 * 1024.0)) / (subTimeMin / 1000000000.0));

    if (!Validate(pUncompressedData, pDecompressedData, fileSize))
    {
      puts("Validation Failed.");
      goto epilogue;
    }

#ifdef BUILD_WITH_OPENCL
    if (normalMode && subSections > 0)
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

      if (!Validate(pUncompressedData, pDecompressedData, fileSize))
      {
        puts("Validation Failed.");
        goto epilogue;
      }
    }
#endif
  }


  //////////////////////////////////////////////////////////////////////////

  goto epilogue;

epilogue:
  if (pFile)
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

bool Validate(const uint8_t *pUncompressedData, const uint8_t *pDecompressedData, const size_t fileSize)
{
  if (memcmp(pUncompressedData, pDecompressedData, (size_t)fileSize) != 0)
  {
    puts("Validation Failed.");

    for (size_t i = 0; i < fileSize; i++)
    {
      if (pUncompressedData[i] != pDecompressedData[i])
      {
        printf("First invalid char at %" PRIu64 " (0x%" PRIX8 " != 0x%" PRIX8 ").\n", i, pUncompressedData[i], pDecompressedData[i]);

        const int64_t start = max(0, (int64_t)i - 64);
        const int64_t end = min((int64_t)fileSize, (int64_t)(i + 64));

        printf("\nContext: (%" PRIi64 " to %" PRIi64 ")\n\n   Expected:                                        |  Actual Output:\n\n", start, end);

        for (int64_t context = start; context < end; context += 16)
        {
          const int64_t context_end = min(end, context + 16);

          bool different = false;

          for (int64_t j = context; j < context_end; j++)
          {
            if (pUncompressedData[j] != pDecompressedData[j])
            {
              different = true;
              break;
            }
          }

          if (different)
            fputs("!! ", stdout);
          else
            fputs("   ", stdout);

          for (int64_t j = context; j < context_end; j++)
            printf("%02" PRIX8 " ", pUncompressedData[j]);

          for (int64_t j = context_end; j < context + 16; j++)
            fputs("   ", stdout);

          fputs(" |  ", stdout);

          for (int64_t j = context; j < context_end; j++)
            printf("%02" PRIX8 " ", pDecompressedData[j]);

          puts("");

          if (different)
          {
            fputs("   ", stdout);

            for (int64_t j = context; j < context_end; j++)
            {
              if (pUncompressedData[j] != pDecompressedData[j])
                fputs("~~ ", stdout);
              else
                fputs("   ", stdout);
            }

            for (int64_t j = context_end; j < context + 16; j++)
              fputs("   ", stdout);

            fputs("    ", stdout);

            for (int64_t j = context; j < context_end; j++)
            {
              if (pUncompressedData[j] != pDecompressedData[j])
                fputs("~~ ", stdout);
              else
                fputs("   ", stdout);
            }
          }

          puts("");
        }

        break;
      }
    }

    return false;
  }

  return true;
}

double GetInformationRatio(const uint8_t *pData, const size_t length)
{
  uint64_t hist[256];
  memset(hist, 0, sizeof(hist));

  for (int i = 0; i < length; i++)
    hist[pData[i]]++;

  const double lengthD = (double)length;
  double ret = 0;
  uint64_t histSymbolCount = 0;

  for (int i = 0; i < 256; i++)
  {
    if (hist[i] != 0)
    {
      const double freq = hist[i] / lengthD;
      ret -= freq * log2(freq);
      histSymbolCount++;
    }
  }

  return ret / log2((double)histSymbolCount);
}
