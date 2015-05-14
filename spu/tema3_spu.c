/*
 * Computer Systems Architecture - Lab 6 (2015)
 * SPU code 
 */

#include <stdio.h>
#include <libmisc.h>
#include <spu_mfcio.h>
#include <spu_intrinsics.h>

#include "cmp.h"

#define waitag(t) mfc_write_tag_mask(1<<t); mfc_read_tag_status_all();
#define ALIGNMENT 16
#define N  16	

struct myBlock {
    unsigned char min, max;
    unsigned char index_matrix[BLOCK_SIZE * BLOCK_SIZE];
    unsigned char dummy[16 - ((BLOCK_SIZE * BLOCK_SIZE + 2) % 16)];
};

typedef struct {
    unsigned char *originalImagePixels;
    unsigned char *pgmImage;
    int operationMode;
    int lines;
    int threadIndex;
    int lineWidth;
    int remainingLines;
    struct myBlock *cmpImage;
} spu_param;

void computeLineOfBlocks(unsigned char *blocks, spu_param *param, int processed, int threadOffset, int blocksPerLine, uint32_t tag_id) {
    int tid = param->threadIndex;
    int lines = param->lines;
    int lineWidth = param->lineWidth;
    struct myBlock *cmpImage = param->cmpImage;
    unsigned char *pgmImage = param->pgmImage;
    unsigned char *originalImagePixels = param->originalImagePixels;
    int computationMode = param->operationMode;

    int i, j;
  /*  for (i = 0; i < N; i++) {
        for (j = 0; j < lineWidth; j++) {
            printf("%d ", *(blocks + i * lineWidth + j));
        }
        printf("\n-------------------------------\n");
    }

    printf("\n\n\n");
    */
    if (computationMode == 0) {
        //scalar version
        int i;
        struct myBlock resultingBlocks[blocksPerLine];

        for (i = 0; i < blocksPerLine; i++) {
            int min = 9999999, max = 0;
            int row, col;

            for (row = 0; row < N; row++) {
                for (col = 0; col < N; col++) {
                    int offset = i * N + col + row * lineWidth;
                    int number = (int)*(blocks + offset);
                    if (number < min)
                        min = number;

                    if (number > max)
                        max = number;

                }
            }

            resultingBlocks[i].min = min;
            resultingBlocks[i].max = max;

            float step = (max - min) / (float)(N - 1);
            
            for (row = 0; row < N; row++) {
                for (col = 0; col < N; col++) {
                    int offset = i * N + col + row * lineWidth;
                    unsigned char *number = blocks + offset;
                    float value = *number;

                    value -= min;
                    value /= step;
                    value += 0.5;
                    *number = (unsigned char)(value);
                    resultingBlocks[i].index_matrix[row * N + col] = *number;

                    value = (float) *number;
                    value *= step;
                    value += min;
                    value += 0.5;
                    *number = (unsigned char)value;
                }
            }

        }

      /*  for (i = 0; i < N; i++) {
        for (j = 0; j < lineWidth; j++) {
            printf("%d ", *(blocks + i * lineWidth + j));
        }
        printf("\n");
    }
        */
       for (i = 0; i < blocksPerLine; i++) {
            int destinationOffset = tid * lines * blocksPerLine + processed * blocksPerLine + i;
            
            printf("size of shit: %d\n", sizeof(struct myBlock));
            //mfc_put((void *) &resultingBlocks[i], (unsigned int) cmpImage + destinationOffset, (uint32_t) sizeof(struct myBlock), tag_id, 0, 0);
            
        }
        
        int j;

        for (i = 0; i < N; i++) {
            int offset = i * lineWidth * sizeof(unsigned char);
            //mfc_put((void *) blocks + offset, (unsigned int) pgmImage + threadOffset + processed * N * lineWidth * sizeof(unsigned char) + offset, (uint32_t)lineWidth * sizeof(unsigned char), tag_id, 0, 0);
        }

        waitag(tag_id);
    } else if (computationMode == 1) {
        // vectorized version
        /*vector float *vBlocks = (vector float *)blocks;
        vector float *vMin = spu_splats((float)999999);
        vector float *vMax = spu_splats((float)0);
        int n = lineWidth / (ALIGNMENT / sizeof(float));
        int row, col;

        int threadOffset = 0;
                
        for (row = 0; row < n; row++) {
            for (col = 0; col < n; col++) {
                int offset = row * n + col + threadOffset;
                vMax = spu_sel(vMax, vBlocks[offset], spu_cmpgt(vBlocks[offset], vMax));
                vMin = spu_sel(vMin, vBlocks[offset], spu_cmpgt(vMin, vBlocks[offset]));
            }
        }

        unsigned char minVal = 999999, maxVal = 0;
        vector float *vStep;
        for (i = 0; i < 4; i++) {
            short val = spu_extract(vMin, i);
            if (val < minVal)
                minVal = val;

            val = spu_extract(vMax, i);
            if (val > maxVal)
                maxVal = val;

            float step = (N - 1) / (maxVal - minVal);
            vStep = spu_splats(step);
        }
        
        vMin = spu_splats(minVal);
        vMax = spu_splats(maxVal);

        for (row = 0; row < n; row++) {
            for (col = 0; col < n; col++) {
                vBlocks[offset] = spu_sub(vBlocks[offset], vMin);
                vBlocks[offset] = spu_mul(vBlocks[offset], vStep);
            }
        }*/
    } else {
        // vectorized with intrinsics
        //
    }

}

int main(unsigned long long speid, 
		unsigned long long argp, 
		unsigned long long envp) 
{

    spu_param param __attribute__((aligned(ALIGNMENT)));
    uint32_t tag_id = mfc_tag_reserve();

    if (tag_id == MFC_TAG_INVALID) {
        printf("SPU ERR: Tag invalid!\n");
        return -1;
    }

    uint32_t spu_param_size = (uint32_t) envp;
    mfc_get((void *)&param, argp, spu_param_size, tag_id, 0, 0);
    waitag(tag_id);

    uint32_t lineSize = (uint32_t)param.lineWidth * sizeof(unsigned char);
    int i = 0;
    int processed = 0;
    int blocksPerLine = 0;
    blocksPerLine = param.lineWidth / N;

    int threadStartPosition = param.threadIndex * N * param.lines * param.lineWidth;
    int linesToProcess = param.lines + param.remainingLines;

    while(processed < 1) {
        printf("-----\n\n");
        printf("processed: %d\nlinesToProcess:%d\n", processed, linesToProcess);
        int blockPosition = processed * N * param.lineWidth;
        
        unsigned char lineOfBlocks[N * param.lineWidth] __attribute__((aligned(ALIGNMENT)));
    
        for (i = 0; i < N; i++) {
            int blockOffset = i * param.lineWidth;
            int imageOffset = i * param.lineWidth + threadStartPosition + blockPosition;
            //printf("lineSize: %d\n", lineSize);
            //printf("blockOffset: %d\n", blockOffset);
            //printf("imageOffset: %d\n", imageOffset);
            mfc_get(&lineOfBlocks[blockOffset], (uint32_t) &param.originalImagePixels[imageOffset], lineSize, tag_id, 0, 0);
        }

        waitag(tag_id);
        //compress
        //
        computeLineOfBlocks(lineOfBlocks, &param, processed, threadStartPosition, blocksPerLine, tag_id);
        
        processed++;
    }

    mfc_tag_release(tag_id);

	return 0;
}
