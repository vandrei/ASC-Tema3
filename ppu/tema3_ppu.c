/*
 * Computer Systems Architecture - Lab 6 (2015)
 * PPU code 
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libspe2.h>
#include <pthread.h>
#include <ppu_intrinsics.h>
#include <math.h>
#include <sys/time.h>  
#include <string.h>

#include "cmp.h"

extern spe_program_handle_t tema3_spu;

#define MAX_SPU_THREADS   16
#define ALIGNMENT 16
#define N 16

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

void *ppu_pthread_function(void *thread_arg) {

	spe_context_ptr_t ctx;
    spu_param *arg = (spu_param *)thread_arg;

	/* Create SPE context */
	if ((ctx = spe_context_create (0, NULL)) == NULL) {
		perror ("Failed creating context");
		exit (1);
	}


	if (spe_program_load (ctx, &tema3_spu)) {
		perror ("Failed loading program");
		exit (1);
	}

	unsigned int entry = SPE_DEFAULT_ENTRY;
	if (spe_context_run(ctx, &entry, 0, (void *)arg, (void *)sizeof(spu_param), NULL) < 0) { 
		perror ("Failed running context");
		exit (1);
	}

	if (spe_context_destroy (ctx) != 0) {
		perror("Failed destroying context");
		exit (1);
    }

	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	if (argc != 7) {
        printf("Error: Invalid program run\n");
        return -1;
    }
    
    struct timeval totalTimeStart, totalTimeEnd;
    struct timeval computationalTimeStart, computationalTimeEnd;

    int computationMode = atoi(argv[1]);
    int dmaMode = atoi(argv[2]);
    int spuCount = atoi(argv[3]);
    
    char *inputFile = argv[4];
    char *cmpOutputFile = argv[5];
    char *pgmOutputFile = argv[6];
   
    if (dmaMode == 0) {
        // normal dma transfer
        struct img originalImage __attribute__ ((aligned(ALIGNMENT)));
        gettimeofday(&totalTimeStart, NULL);
        read_pgm(inputFile, &originalImage);
        gettimeofday(&computationalTimeStart, NULL);

        unsigned char originalPixels[originalImage.height * originalImage.width] __attribute__ ((aligned(ALIGNMENT)));
        memcpy(originalPixels, originalImage.pixels, originalImage.width * originalImage.height * sizeof(unsigned char));
        
        struct img pgmImage __attribute__ ((aligned(ALIGNMENT)));
        pgmImage.width = originalImage.width;
        pgmImage.height = originalImage.height;
        pgmImage.pixels = (unsigned char *)_alloc(originalImage.width * originalImage.height * sizeof(unsigned char));
       
        unsigned char pgmPixels[originalImage.width * originalImage.height] __attribute__ ((aligned(ALIGNMENT)));
        
        struct c_img cmpImage __attribute__ ((aligned(ALIGNMENT)));
        cmpImage.width = originalImage.width;
        cmpImage.height = originalImage.height;
        cmpImage.blocks = (struct block*) _alloc((originalImage.height * originalImage.width / (N * N)) * sizeof(struct block));
      
        int cmpSize = originalImage.height * originalImage.width / (N * N);
        struct myBlock cmpImageBlocks[cmpSize] __attribute__ ((aligned(ALIGNMENT)));
       
        pthread_t threads[spuCount];
        spu_param messages[spuCount] __attribute__ ((aligned(ALIGNMENT)));
       
        printf("Image size: %d x %d\n\n", originalImage.height, originalImage.width); 
        int i;
        for (i = 0; i < spuCount; i++) {
            messages[i].threadIndex = i;
            messages[i].originalImagePixels = originalPixels;
            messages[i].lines = originalImage.height / N / spuCount;
            messages[i].lineWidth = originalImage.width;
            messages[i].operationMode = computationMode;
            if (i == spuCount - 1) {
                messages[i].remainingLines = originalImage.height / N % spuCount;
            } else {
                messages[i].remainingLines = 0;
            }
            messages[i].cmpImage = cmpImageBlocks;
            messages[i].pgmImage = pgmPixels;
            if (pthread_create(&threads[i], NULL, &ppu_pthread_function, &messages[i])) {
	    	    perror ("Failed creating thread");
                exit(-1);
            }
        }

        for (i = 0; i < spuCount; i++) {
            if (pthread_join(threads[i], NULL)) {
    		    perror("Failed pthread_join");
            }
        }

        int j;
        for (i = 0; i < originalImage.height / N; i++) {
            for (j = 0; j < originalImage.width / N; j++) {
                int offset = originalImage.width / N * i + j;
                cmpImage.blocks[offset].min = cmpImageBlocks[offset].min;
                cmpImage.blocks[offset].max = cmpImageBlocks[offset].max;
                memcpy(cmpImage.blocks[offset].index_matrix, cmpImageBlocks[offset].index_matrix, N * N * sizeof(unsigned char));
            }
        }

        memcpy(pgmImage.pixels, pgmPixels, originalImage.width * originalImage.height * sizeof(unsigned char));
      
        /*for (i = 0; i < originalImage.height/N;i++) {
            for (j = 0; j < originalImage.width/N;j++) {
                int x,y;
                for (x = 0; x < N; x++) {
                    for (y = 0; y < N; y++) {
                        int offset = originalImage.width / N * i + j;
                        printf("%d ", cmpImage.blocks[offset].index_matrix[x * N + y]);
                    }
                    printf("\n");
                }
            }
        }*/

        gettimeofday(&computationalTimeEnd, NULL);
        write_cmp(cmpOutputFile, &cmpImage);
        write_pgm(pgmOutputFile, &pgmImage);
        free_pgm(&pgmImage);
        free_pgm(&originalImage);
        free_cmp(&cmpImage);
        gettimeofday(&totalTimeEnd, NULL);
        
        double totalTimeDelta = GET_TIME_DELTA(totalTimeStart, totalTimeEnd);
        double computationalTimeDelta = GET_TIME_DELTA(computationalTimeStart, computationalTimeEnd);

        printf("Computational time: %lf\n", computationalTimeDelta);
        printf("Total runtime: %lf\n", totalTimeDelta);
    } else {
        // double dma buffering
        //
    }
	
    return 0;
}
