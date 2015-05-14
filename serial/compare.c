#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#include "cmp.h"

void compare_pgm(struct img* img1, struct img* img2){
	int i, nr_diff_pixels = 0, max_pixel_diff = 0;
	unsigned char c1, c2, diff;
	
	if (img1->width != img2->width){
		printf("Image 1 has a width of %d, while image 2 has a width of %d\n", 
			img1->width, img2->width);
		return;
	}
	if (img1->height != img2->height){
		printf("Image 1 has a height of %d, while image 2 has a height of %d\n", 
			img1->height, img2->height);
		return;
	}
	
	//compare pixels
	for (i = 0; i < img1->width * img1->height; i++){
		c1 = img1->pixels[i];
		c2 = img2->pixels[i];
		if (c1 > c2)
			diff = c1 - c2;
		else
			diff = c2 - c1;;
		if (diff > 0)
			nr_diff_pixels++;
		if (diff > max_pixel_diff)
			max_pixel_diff = diff;
	}
	
	printf("%s: Percentage of different pixels is %lf\n", __func__, 
		( 100 * (float)nr_diff_pixels) / (img1->width * img1->height)); 
	printf("%s: Maximum pixel difference is %d\n", __func__, max_pixel_diff); 
}

void compare_cmp(struct c_img* img1, struct c_img* img2){
	int i, nr_blocks, nr_diff_min = 0, nr_diff_max = 0, j, 
		nr_diff_index_matrix = 0, nr_diff_indexes = 0, tmp, 
		max_index_diff = 0, diff;
	
	if (img1->width != img2->width){
		printf("Image 1 has a width of %d, while image 2 has a width of %d\n", 
			img1->width, img2->width);
		return;
	}
	if (img1->height != img2->height){
		printf("Image 1 has a height of %d, while image 2 has a height of %d\n", 
			img1->height, img2->height);
		return;
	}
	
	nr_blocks = img1->width * img1->height / (BLOCK_SIZE * BLOCK_SIZE);
	
	for (i = 0; i < nr_blocks; i++){
		//compare min values
		if (img1->blocks[i].min != img2->blocks[i].min) {
			nr_diff_min++;
		}
		//compare max values
		if (img1->blocks[i].max != img2->blocks[i].max)
			nr_diff_max++;
		//compare index matrix values
		tmp = 0;
		for (j = 0; j < BLOCK_SIZE * BLOCK_SIZE; j++) {
			diff = abs(img1->blocks[i].index_matrix[j] - img2->blocks[i].index_matrix[j]);
			if (diff > 0) {
				tmp++;
			}
			if (diff > max_index_diff)
				max_index_diff = diff;
		}
		nr_diff_indexes += tmp;
		if (tmp > 0)
			nr_diff_index_matrix++;
		
	}
	
	printf("%s: Different min values percentage = %f\n", 
		__func__, 100.0 * ((float)  nr_diff_min) / nr_blocks);
	printf("%s: Different max values percentage = %f\n", 
		__func__, 100.0 * ((float)  nr_diff_max) / nr_blocks);
	printf("%s: Different index matrix percentage = %f\n", 
		__func__, 100.0 * ((float)  nr_diff_index_matrix) / 
		nr_blocks);
	printf("%s: Different indexes percentage = %f\n", 
		__func__, 100.0 * ((float)  nr_diff_indexes) / 
		(nr_blocks * BLOCK_SIZE * BLOCK_SIZE));
	printf("%s: Max index diff = %d\n", __func__, max_index_diff);
		
}

int main(int argc, char** argv){
	struct img img1, img2;
	struct c_img c_img1, c_img2;
	struct timeval t1, t2, t3, t4;
	float total_time = 0.0, scale_time = 0.0;

	if (argc != 4){
		printf("Usage: %s mode file1 file2\n", argv[0]);
		return 0;
	}

	gettimeofday(&t1, NULL);

	if (!strcmp(argv[1], "pgm")){
		read_pgm(argv[2], &img1);
		read_pgm(argv[3], &img2);
		gettimeofday(&t3, NULL);
		compare_pgm(&img1, &img2);
		gettimeofday(&t4, NULL);
		free_pgm(&img1);
		free_pgm(&img2);
	}

	if (!strcmp(argv[1], "cmp")){
		read_cmp(argv[2], &c_img1);
		read_cmp(argv[3], &c_img2);
		gettimeofday(&t3, NULL);
		compare_cmp(&c_img1, &c_img2);
		gettimeofday(&t4, NULL);
		free_cmp(&c_img1);
		free_cmp(&c_img2);
	}
	gettimeofday(&t2, NULL);

	scale_time += GET_TIME_DELTA(t3, t4);
	total_time += GET_TIME_DELTA(t1, t2);

	printf("Compare time: %lf\n", scale_time);
	printf("Total time: %lf\n", total_time);

	return 0;
}

