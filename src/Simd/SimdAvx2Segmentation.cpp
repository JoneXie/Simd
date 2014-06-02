/*
* Simd Library.
*
* Copyright (c) 2011-2014 Yermalayeu Ihar.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy 
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
* copies of the Software, and to permit persons to whom the Software is 
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in 
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "Simd/SimdAvx2.h"
#include "Simd/SimdMemory.h"
#include "Simd/SimdConst.h"

namespace Simd
{
#ifdef SIMD_AVX2_ENABLE
    namespace Avx2
    {
        SIMD_INLINE bool RowHasIndex(const uint8_t * mask, size_t alignedSize, size_t fullSize, __m256i index)
        {
            for (size_t col = 0; col < alignedSize; col += A)
            {
                if(!_mm256_testz_si256(_mm256_cmpeq_epi8(_mm256_loadu_si256((__m256i*)(mask + col)), index), K_INV_ZERO))
                    return true;
            }
            if(alignedSize != fullSize)
            {
                if(!_mm256_testz_si256(_mm256_cmpeq_epi8(_mm256_loadu_si256((__m256i*)(mask + fullSize - A)), index), K_INV_ZERO))
                    return true;
            }
            return false;
        }

        SIMD_INLINE bool ColsHasIndex(const uint8_t * mask, size_t stride, size_t size, __m256i index, uint8_t * cols)
        {
            __m256i _cols = _mm256_setzero_si256();
            for (size_t row = 0; row < size; ++row)
            {
                _cols = _mm256_or_si256(_cols, _mm256_cmpeq_epi8(_mm256_loadu_si256((__m256i*)mask), index));
                mask += stride;
            }
            _mm256_storeu_si256((__m256i*)cols, _cols);
            return !_mm256_testz_si256(_cols, K_INV_ZERO);
        }

        void SegmentationShrinkRegion(const uint8_t * mask, size_t stride, size_t width, size_t height, uint8_t index,
            ptrdiff_t * left, ptrdiff_t * top, ptrdiff_t * right, ptrdiff_t * bottom)
        {
            assert(*right - *left >= A && *bottom > *top);
            assert(*left >= 0 && *right <= (ptrdiff_t)width && *top >= 0 && *bottom <= (ptrdiff_t)width);

            size_t fullWidth = *right - *left;          
            ptrdiff_t alignedWidth = Simd::AlignLo(fullWidth, A);
            __m256i _index = _mm256_set1_epi8(index);
            bool search = true;
            for (ptrdiff_t row = *top; search && row < *bottom; ++row)
            {
                if(RowHasIndex(mask + row*stride + *left, alignedWidth, fullWidth, _index))
                {
                    search = false;
                    *top = row;
                }
            }

            if(search)
            {
                *left = 0;
                *top = 0;
                *right = 0;
                *bottom = 0;
                return;
            }

            search = true;
            for (ptrdiff_t row = *bottom - 1; search && row >= *top; --row)
            {
                if(RowHasIndex(mask + row*stride + *left, alignedWidth, fullWidth, _index))
                {
                    search = false;
                    *bottom = row + 1;
                }
            }

            search = true;
            for (ptrdiff_t col = *left; search && col < *left + alignedWidth; col += A)
            {
                uint8_t cols[A];
                if(ColsHasIndex(mask + (*top)*stride + col, stride, *bottom - *top, _index, cols))
                {
                    for(size_t i = 0; i < A; i++)
                    {
                        if (cols[i])
                        {
                            *left = col + i;
                            break;
                        }
                    }
                    search = false;
                    break;
                }
            }

            search = true;
            for (ptrdiff_t col = *right; search && col > *left; col -= A)
            {
                uint8_t cols[A];
                if(ColsHasIndex(mask + (*top)*stride + col - A, stride, *bottom - *top, _index, cols))
                {
                    for(ptrdiff_t i = A - 1; i >= 0; i--)
                    {
                        if (cols[i])
                        {
                            *right = col - A + i + 1;
                            break;
                        }
                    }
                    search = false;
                    break;
                }
            }
        }
    }
#endif//SIMD_AVX2_ENABLE
}