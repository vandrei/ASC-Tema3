#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "cmp.h"

void decompress_serial(struct img* image, struct c_img* c_image){
	int block_row_start, block_col_start, i, j, nr_blocks, k;
	float factor;
	struct block* curr_block;
	
	nr_blocks = c_image->width * c_image->height / (BLOCK_SIZE * BLOCK_SIZE);

	image->width = c_image->width;
	image->height = c_image->height;
	image->pixels = _alloc(image->width * image->height * sizeof(unsigned char));
	block_row_start = block_col_start = 0;
	
	for (i=0; i<nr_blocks; i++){
		k = block_row_start * image->width + block_col_start;
		curr_block = &c_image->blocks[i];
		factor = (curr_block->max - curr_block->min) / (float) (NUM_COLORS_PALETTE - 1);
		for (j=0; j<BLOCK_SIZE * BLOCK_SIZE; j++){
			image->pixels[k++] = (unsigned char) (curr_block->min + 
					curr_block->index_matrix[j] * factor + 0.5);
			if ((j + 1) % BLOCK_SIZE == 0){
				k -= BLOCK_SIZE; //back to the first column of the block
				k += image->width ; //go to the next line
			}
		}
		block_col_start += BLOCK_SIZE;
		if (block_col_start >= image->width){
			block_col_start = 0;
			block_row_start += BLOCK_SIZE;
		}
	}
}

void compress_serial(struct img* image, struct c_img* c_image){
	int row, col, bl_row, bl_col, bl_index, nr_blocks;
	unsigned char min, max;
	float aux, factor;
	struct block *curr_block;
	
	c_image->width = image->width;
	c_image->height = image->height;
	nr_blocks = image->width * image->height / (BLOCK_SIZE * BLOCK_SIZE);

	c_image->blocks = (struct block*) _alloc(nr_blocks * sizeof(struct block));

	bl_index = 0;
	for (bl_row = 0; bl_row < image->height; bl_row += BLOCK_SIZE){
		for (bl_col = 0; bl_col < image->width; bl_col += BLOCK_SIZE){
			//process 1 block from input image

			curr_block = &c_image->blocks[bl_index];
			//compute min and max
			min = max = image->pixels[bl_row * image->width + bl_col];
			for (row = bl_row; row < bl_row + BLOCK_SIZE; row++){
				for (col = bl_col; col < bl_col + BLOCK_SIZE; col++){
					if (image->pixels[row * image->width + col] < min)
						min = image->pixels[row * image->width + col];
					if (image->pixels[row * image->width + col] > max)
						max = image->pixels[row * image->width + col];
				}
			}
			curr_block->min = min;
			curr_block->max = max;
			
			//compute factor
			factor = (max - min) / (float) (NUM_COLORS_PALETTE - 1);
			
			//compute index matrix
			if (factor != 0) {
				//min != max
				for (row = bl_row; row < bl_row + BLOCK_SIZE; row++){
					for (col = bl_col; col < bl_col + BLOCK_SIZE; col++){
						aux =  (image->pixels[row * image->width + col] - min) / factor;
						curr_block->index_matrix[(row - bl_row) * BLOCK_SIZE + col - bl_col] = 
								(unsigned char) (aux + 0.5);
					}
				}
			} else {
				// min == max
				// all colors represented with min => index = 0
				memset(curr_block->index_matrix, 0, BLOCK_SIZE * BLOCK_SIZE);
			}
					
			bl_index++;
		}
	}
	
}

int main(int argc, char** argv){
	struct img image, image2;
	struct c_img c_image;
	struct timeval t1, t2, t3, t4;
	double total_time = 0, scale_time = 0;

	if (argc != 4){
		printf("Usage: %s in.pgm out.cmp out.pgm\n", argv[0]);
		return 0;
	}
	
	gettimeofday(&t3, NULL);	

	read_pgm(argv[1], &image);	
	
	gettimeofday(&t1, NULL);	
	compress_serial(&image, &c_image);
	decompress_serial(&image2, &c_image);
	gettimeofday(&t2, NULL);	

	write_cmp(argv[2], &c_image);
	write_pgm(argv[3], &image2);

	free_cmp(&c_image);
	free_pgm(&image);
	free_pgm(&image2);

	gettimeofday(&t4, NULL);

	total_time += GET_TIME_DELTA(t3, t4);
	scale_time += GET_TIME_DELTA(t1, t2);

	printf("Compress / Decompress time: %lf\n", scale_time);
	printf("Total time: %lf\n", total_time);
	
	return 0;
}	
