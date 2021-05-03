# rle8

### What is it?
- Possibly the fastest run length en/decoder (obviously dependent on the dataset). **Single Core Decompression Speeds > 17.5 GB/s have been observed.**
- Tries to keep symbol general symbol frequency to improve compression ratio of an entropy encoder that could go after the Run Length Encoding like ANS, Arithmetic Coding or Huffman.
- Written in C.
- SIMD Variants for AVX-512F, AVX2, AVX, SSSE3 and SSE2 are available for the decoder. Automatically picked by the en- & decoder based on the extensions available on the current platform.
- Specialized versions for various different scenarios. (Single RLE Symbol, Short Strings of RLE Symbol, 8 Bit, 16 Bit, 24 Bit, 32 Bit, 48 Bit, 64 Bit, 128 Bit)
- `OpenCL` variant available for some of the decoders.

### Benchmark
 - Single-Threaded
 - Running on an `Intel(R) Core(TM) i9-9900K CPU @ 3.60GHz` on Windows 10. (Thanks, Silv3rfire!)
 - Compiled with `Visual Studio 2017`.
 - Compared to [TurboRLE](https://github.com/powturbo/TurboRLE) (with AVX2 enabled; benchmarking results have been converted from MB/s to MiB/s)

#### [video-frame.raw](https://www.dropbox.com/s/yvsl1lg98c4maq1/video_frame.raw?dl=1) (heavily quantized video frame DCTs, 88.473.600 Bytes)
| Type | Ratio | Encoding Speed | Decoding Speed | Entropy Compressible To |
| -- | -- | -- | -- | -- |
| rle8 Normal                    |   19.93 %   |    581.4 MiB/s   |    1961.4 MiB/s   |  9.05 % |
| rle8 Normal Single             |   19.96 %   |    580.6 MiB/s   |    3318.0 MiB/s   |  9.10 % |
| rle8 Ultra                     |   24.08 %   |    578.1 MiB/s   |    2314.4 MiB/s   | 11.61 % |
| rle8 Ultra  Single             |   24.11 %   |    570.6 MiB/s   |    3644.5 MiB/s   | 11.69 % |
| **rle8 Extreme  8 Bit**        | **19.38 %** | **2147.4 MiB/s** | **13791.5 MiB/s** |  9.47 % |
| **rle8 Extreme  8 Bit Single** | **18.36 %** |    528.4 MiB/s   | **12222.8 MiB/s** |  9.69 % |
| **rle8 Extreme 16 Bit**        | **20.32 %** |   1354.3 MiB/s   | **12384.4 MiB/s** |  9.56 % |
| **rle8 Extreme 24 Bit**        | **21.50 %** |   1620.4 MiB/s   | **14162.6 MiB/s** |  9.45 % |
| **rle8 Extreme 32 Bit**        | **22.01 %** |   1810.8 MiB/s   | **12994.4 MiB/s** |  9.67 % |
| **rle8 Extreme 48 Bit**        | **23.41 %** | **2328.5 MiB/s** | **14190.0 MiB/s** |  9.60 % |
| **rle8 Extreme 64 Bit**        | **24.53 %** |   2059.8 MiB/s   | **13783.4 MiB/s** |  9.61 % |
| rle8 Extreme 128 Bit           |   27.60 %   |   2016.4 MiB/s   |   14262.9 MiB/s   |  9.76 % |
| - | - | - | - | - |
| memcpy                         | 100.00 %    | 13213.9 MiB/s    | 13061.1 MiB/s     | 14.03 % |
| trle    | 16.0 % |  965.95 MiB/s |  4201.95 MiB/s | - |
| srle 0  | 17.8 % |  588.80 MiB/s |  8057.97 MiB/s | - |
| srle 8  | 18.7 % | 1367.13 MiB/s |  8928.58 MiB/s | - |
| srle 16 | 21.3 % | 1225.40 MiB/s | 10657.44 MiB/s | - |
| srle 32 | 24.2 % | 2098.15 MiB/s | 12287.03 MiB/s | - |
| srle 64 | 27.5 % | 3696.44 MiB/s | 13943.98 MiB/s | - |
| mrle    | 19.7 % |  259.77 MiB/s |  1670.99 MiB/s | - |

#### [1034.db](http://encode.su/threads/2077-EGTB-compression?p=41392&viewfull=1#post41392) (Checkers program "End Game Table Base", 419.225.625 Bytes)
| Type | Ratio | Encoding Speed | Decoding Speed | Entropy Compressible To |
| -- | -- | -- | -- | -- |
| rle8 Normal                    |   21.15 %   |    535.2 MiB/s   |    3327.8 MiB/s   | 10.37 % |
| rle8 Normal Single             |   21.15 %   |    536.7 MiB/s   |    3352.0 MiB/s   | 10.37 % |
| rle8 Ultra                     |   24.87 %   |    526.6 MiB/s   |    3704.0 MiB/s   | 14.20 % |
| rle8 Ultra  Single             |   24.87 %   |    521.2 MiB/s   |    3698.2 MiB/s   | 14.20 % |
| **rle8 Extreme  8 Bit**        | **23.02 %** | **1482.5 MiB/s** | **12473.0 MiB/s** | 12.08 % |
| **rle8 Extreme  8 Bit Single** | **20.59 %** |    473.3 MiB/s   | **10676.4 MiB/s** | 12.06 % |
| **rle8 Extreme 16 Bit**        | **24.89 %** | **1118.5 MiB/s** | **10503.6 MiB/s** | 12.27 % |
| **rle8 Extreme 24 Bit**        | **27.27 %** | **1345.8 MiB/s** | **12788.1 MiB/s** | 11.95 % |
| **rle8 Extreme 32 Bit**        | **28.39 %** | **1485.0 MiB/s** | **10928.2 MiB/s** | 12.27 % |
| **rle8 Extreme 48 Bit**        | **31.22 %** | **1758.2 MiB/s** | **13129.2 MiB/s** | 12.02 % |
| **rle8 Extreme 64 Bit**        | **33.36 %** |   1623.6 MiB/s   | **12586.9 MiB/s** | 11.98 % |
| rle8 Extreme 128 Bit           |   39.84 %   |   1521.6 MiB/s   |   13386.2 MiB/s   | 12.16 % |
| - | - | - | - | - |
| memcpy                         |  100.00 %   |   12839.1 MiB/s  |   12832.8 MiB/s   | 16.76 % |
| trle    | 17.4 % |  599.75 MiB/s |  3241.93 MiB/s | - |
| srle 0  | 20.2 % |  370.41 MiB/s |  6296.73 MiB/s | - |
| srle 8  | 22.0 % |  837.80 MiB/s |  7024.35 MiB/s | - |
| srle 16 | 27.1 % |  763.00 MiB/s |  8050.19 MiB/s | - |
| srle 32 | 32.7 % | 1249.88 MiB/s | 10299.73 MiB/s | - |
| srle 64 | 39.5 % | 2039.26 MiB/s | 12279.01 MiB/s | - |
| mrle    | 21.0 % |  197.12 MiB/s |  2392.61 MiB/s | - |

The 24 Bit and 48 Bit Variants allow for run length encoding of common data layouts that are usually not covered by RLE implementations:

#### [Pixel Art Bitmap Image](https://i.redd.it/tj5oyhhuehv11.png) (PNG converted to BMP, 123.710.454 Bytes)
| Type | Ratio | Encoding Speed | Decoding Speed | Entropy Compressible To |
| -- | -- | -- | -- | -- |
| rle8 Normal                   |   100.00 %   |    691.6 MiB/s   |   11875.4 MiB/s   | 77.03 % |
| **rle8 Extreme 8 Bit**        |  **99.99 %** | **4892.4 MiB/s** |   10863.6 MiB/s   | 76.18 % |
| rle8 Extreme 16 Bit           |    99.99 %   |    581.5 MiB/s   |   11166.5 MiB/s   | 76.02 % |
| **rle8 Extreme 24 Bit**       |   **1.84 %** | **3710.0 MiB/s** | **16767.0 MiB/s** |  1.32 % |
| rle8 Extreme 32 Bit           |    99.99 %   |    609.9 MiB/s   |   11268.7 MiB/s   | 76.02 % |
| **rle8 Extreme 48 Bit**       |   **2.78 %** | **6407.1 MiB/s** | **16446.9 MiB/s** |  2.12 % |
| rle8 Extreme 64 Bit           |    99.99 %   |    592.6 MiB/s   |   10949.2 MiB/s   | 75.87 % |
| rle8 Extreme 128 Bit          |    99.99 %   |    598.1 MiB/s   |   10896.0 MiB/s   | 75.87 % |
| - | - | - | - | - |
| memcpy                       |   100.00 %   |  13089.0 MiB/s   |   13086.8 MiB/s   | 77.03 % |
| trle    | 100.0 % |  273.17 MiB/s |  3817.19 MiB/s | - |
| srle 0  | 100.0 % |  103.62 MiB/s | 11594.23 MiB/s | - |
| srle 8  | 100.0 % |  295.08 MiB/s | 11309.32 MiB/s | - |
| srle 16 | 100.0 % |  288.40 MiB/s | 11384.59 MiB/s | - |
| srle 32 | 100.0 % |  563.25 MiB/s | 11509.88 MiB/s | - |
| srle 64 | 100.0 % | 1129.42 MiB/s | 11360.07 MiB/s | - |
| mrle    | 100.0 % |  125.91 MiB/s |  1942.42 MiB/s | - |

### Setup
``` bash
git clone https://github.com/rainerzufalldererste/rle8.git
cd rle8
git submodule update --init --recursive
```
#### On Windows
```
create_project.bat
```
Choose your preferred compiler toolset
```
MSBuild /p:Configuration=Release /nologo /v:m
```
#### On Linux
```
premake/premake5 gmake2
config=release_x64 make
```

### How to use it?

```c
#include "rle8.h"

uint8_t *pUncompressedData; // Some Data.
uint32_t fileSize; // Some Size.

// Get Compress Bounds.
const uint32_t compressedBufferSize = rle8_compress_bounds(fileSize);
uint8_t *pCompressedData = (uint8_t *)malloc(compressedBufferSize);

// Compress.
const uint32_t compressedSize = rle8_compress(pUncompressedData, fileSize, pCompressedData, compressedBufferSize);

// Allocate Output Buffer.
uint8_t *pDecompressedData = (uint8_t *)malloc(fileSize);

// Decompress.
const uint32_t decompressedSize = rle8_decompress(pCompressedData, compressedSize, pDecompressedData, fileSize);

// Cleanup.
free(pCompressedData);
free(pDecompressedData);
```

### License
Two Clause BSD
