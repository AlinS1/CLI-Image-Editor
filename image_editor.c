// Copyright Similea Alin-Andrei 314CA 2022-2023
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NMAX_LINE 100

// ===========================
// DATA TYPES
// ===========================

struct select_struct {
	int x1;
	int x2;
	int y1;
	int y2;
};

typedef struct select_struct select_struct;

struct pixel_struct {
	int grayscale;
	int r;
	int g;
	int b;
};

typedef struct pixel_struct pixel_struct;

struct image_struct {
	char image_type[3];
	int height;
	int width;
	int max_value;
	pixel_struct **pixel;
	select_struct *select;
};

typedef struct image_struct image_struct;

// ===========================
// ALLOCATION FUNCTIONS
// ===========================

void free_pixel(pixel_struct **pixel, int nr_lin)
{
	// Deallocates the matrix of pixels from the image struct
	for (int i = 0; i < nr_lin; i++)
		free(pixel[i]);

	free(pixel);
}

int array_alloc(int **v, int n)
{
	int *w = (int *)malloc(n * sizeof(int));
	if (!w) {  // if allocation fails, stop
		fprintf(stderr, "malloc() for array failed\n");
		*v = NULL;
		return 0;
	}
	*v = w;
	return 1;
}

int pixel_alloc(pixel_struct ***pixel, int nr_lin, int nr_col)
{
	// Allocates the matrix of pixels from the image struct
	pixel_struct **m = (pixel_struct **)malloc(nr_lin * sizeof(pixel_struct *));
	if (!m) {  // if allocation fails, stop
		fprintf(stderr, "malloc() for pixel failed\n");
		*pixel = NULL;
		return 0;
	}

	*pixel = m;

	for (int i = 0; i < nr_lin; i++) {
		m[i] = (pixel_struct *)malloc(nr_col * sizeof(pixel_struct));
		if (!m[i]) {
			// if one of the allocations fails, we deallocate what we previously
			// allocated
			fprintf(stderr, "malloc() for line %d failed\n", i);
			free_pixel(m, i);
			*pixel = NULL;
			return 0;
		}
	}
	return 1;
}

int image_alloc(image_struct **image)
{
	image_struct *img = (image_struct *)malloc(sizeof(image_struct));
	if (!img) {	 // if allocation fails, stop
		fprintf(stderr, "malloc() for image failed\n");
		*image = NULL;
		free(img);
		return 0;
	}

	*image = img;
	return 1;
}

int select_alloc(select_struct **select)
{
	select_struct *sel = (select_struct *)malloc(sizeof(select_struct));
	if (!sel) {	 // if allocation fails, stop
		fprintf(stderr, "malloc() for select failed\n");
		free(sel);
		*select = NULL;
		return 0;
	}

	*select = sel;
	return 1;
}

void free_img(image_struct *image)
{
	// Deallocate the memory of an image and its elements.
	free(image->select);
	for (int i = 0; i < image->height; i++)
		free(image->pixel[i]);

	free(image->pixel);
	free(image);
}

// =============================
// FUNCTIONS THAT DEAL WITH DATA
// =============================

int load_binary(image_struct *image, char *file_path, long file_pos)
{
	FILE *pf = fopen(file_path, "rb");
	if (!pf) {
		printf("Failed to load %s\n", file_path);
		return 0;
	}

	// Reposition the cursor in file in order to read from where we left off.
	fseek(pf, file_pos, 0);

	// grayscale
	if (strcmp(image->image_type, "P5") == 0) {
		for (int i = 0; i < image->height; i++) {
			// The elements in the binary file are of type char, so we need to
			// read them accordingly. It's easier to do it as an array.
			unsigned char *v_aux =
				(unsigned char *)malloc(sizeof(unsigned char) * image->width);
			if (!v_aux) {
				fprintf(stderr, "malloc() for array failed\n");
				return 0;
			}
			fread(v_aux, sizeof(unsigned char), image->width, pf);

			for (int j = 0; j < image->width; j++)
				// Transform the chars in integers
				image->pixel[i][j].grayscale = (int)(v_aux[j]);

			free(v_aux);
		}
	}

	// colour
	if (strcmp(image->image_type, "P6") == 0) {
		for (int i = 0; i < image->height; i++) {
			for (int j = 0; j < image->width; j++) {
				// Here, we can't use an array as we previously did for
				// grayscale images, because we have one r, one g and one b
				// values at a time. It's easier to read them separately.
				unsigned char r, g, b;
				fread(&r, sizeof(unsigned char), 1, pf);
				fread(&g, sizeof(unsigned char), 1, pf);
				fread(&b, sizeof(unsigned char), 1, pf);
				image->pixel[i][j].r = (int)r;
				image->pixel[i][j].g = (int)g;
				image->pixel[i][j].b = (int)b;
			}
		}
	}
	fclose(pf);
	return 1;
}

image_struct *load(image_struct *image_test, int *loaded_img_now, char *delim)
{
	// The file_path is the next word from previously read line in main.
	char *file_path = strtok(NULL, delim);

	if (!file_path) {
		printf("Invalid command\n");
		return NULL;
	}
	if (*(loaded_img_now) == 1) {
		free_img(image_test);
		(*loaded_img_now)--;
	}
	FILE *pf = fopen(file_path, "rt");
	if (!pf) {
		printf("Failed to load %s\n", file_path);
		return NULL;
	}

	image_struct *image;
	if (image_alloc(&image) == 0)
		return NULL;

	char buffer[NMAX_LINE];
	int values[5];
	for (int i = 1; i <= 4;) {	// loop until we read 4 elements
		fscanf(pf, "%s", buffer);
		if (strchr(buffer, '#')) {	// skip comments
			fgets(buffer, NMAX_LINE, pf);
			continue;
		}
		if (i == 1) {
			strncpy(image->image_type, buffer, 2);
			image->image_type[2] = '\0';
		}
		values[i] = atoi(buffer);
		i++;
	}

	image->width = values[2];
	image->height = values[3];
	image->max_value = values[4];
	if (pixel_alloc(&image->pixel, image->height, image->width) == 0)
		return NULL;

	if (strcmp(image->image_type, "P2") == 0 ||
		strcmp(image->image_type, "P3") == 0) {
		// ASCII grayscale
		if (strcmp(image->image_type, "P2") == 0)
			for (int i = 0; i < image->height; i++)
				for (int j = 0; j < image->width; j++)
					fscanf(pf, "%d", &image->pixel[i][j].grayscale);
		// Colour
		if (strcmp(image->image_type, "P3") == 0)
			for (int i = 0; i < image->height; i++)
				for (int j = 0; j < image->width; j++)
					fscanf(pf, "%d%d%d", &image->pixel[i][j].r,
						   &image->pixel[i][j].g, &image->pixel[i][j].b);
	}

	if (strcmp(image->image_type, "P5") == 0 ||
		strcmp(image->image_type, "P6") == 0) {
		// Binary
		char character;
		fscanf(pf, "%c", &character);  // skip a "\n"
		long file_pos = ftell(pf);	// memorize the position to read from here
		fclose(pf);
		if (load_binary(image, file_path, file_pos) == 0)
			return NULL;
	}

	if (select_alloc(&image->select) == 0)
		return NULL;
	image->select->x1 = 0;	// the regular selection of the whole image
	image->select->x2 = image->width;
	image->select->y1 = 0;
	image->select->y2 = image->height;

	printf("Loaded %s\n", file_path);
	(*loaded_img_now)++;
	return image;
}

image_struct *copy_image(image_struct *initial)
{
	// Makes a copy of the initial image.
	image_struct *copy;
	if (image_alloc(&copy) == 0)
		return NULL;

	strcpy(copy->image_type, initial->image_type);
	copy->height = initial->height;
	copy->width = initial->width;
	copy->max_value = initial->max_value;

	if (pixel_alloc(&copy->pixel, copy->height, copy->width) == 0)
		return NULL;
	for (int i = 0; i < copy->height; i++)
		for (int j = 0; j < copy->width; j++)
			copy->pixel[i][j] = initial->pixel[i][j];

	if (select_alloc(&copy->select) == 0)
		return NULL;
	copy->select->x1 = initial->select->x1;
	copy->select->x2 = initial->select->x2;
	copy->select->y1 = initial->select->y1;
	copy->select->y2 = initial->select->y2;

	return copy;
}

int validate_selection(image_struct *image, int x1, int y1, int x2, int y2)
{
	// Verifies if the coordinates are inside the image's borders.
	if (x1 < 0 || x1 > image->width || x2 < 0 || x2 > image->width)
		return 0;
	if (y1 < 0 || y1 > image->height || y2 < 0 || y2 > image->height)
		return 0;
	int kon = 1;

	// Verifies how many coordinates have the same value.
	// There must be less than 3 to be a valid selection.
	// Otherwise, there will be no elements in the selection.
	int val[4] = {x1, x2, y1, y2};
	for (int i = 0; i < 4; i++) {
		kon = 1;
		for (int j = i + 1; j < 4; j++)
			if (val[i] == val[j])
				kon++;
		if (kon >= 3)
			return 0;
	}

	// Verifies how many coordinates have the same value as the upper borders of
	// the image (width and height).There must be less than 3 to be a valid
	// selection. Otherwise, there will be no elements in the selection.
	kon = 0;
	for (int i = 0; i < 4; i++) {
		if (val[i] == image->height || val[i] == image->width)
			kon++;
	}
	if (kon >= 3)
		return 0;

	return 1;
}

void select_all(image_struct *image)
{
	// Selects the whole image.
	image->select->x1 = 0;
	image->select->x2 = image->width;
	image->select->y1 = 0;
	image->select->y2 = image->height;
	printf("Selected ALL\n");
}

void select_image(image_struct *image, int loaded_img_now, char *delim)
{
	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return;
	}

	char *next_elem_in_line = strtok(NULL, delim);
	if (!next_elem_in_line) {
		printf("Invalid command\n");
		return;
	}
	if (strcmp(next_elem_in_line, "ALL") == 0) {
		select_all(image);
		return;
	}

	// If we have to execute a normal SELECT, the previously read element will
	// actually be the first coordinate (x1).
	char *element[4];
	element[0] = next_elem_in_line;
	int number[4];

	// We transform them in integers, because we read them as chars.
	number[0] = atoi(element[0]);
	char *next;
	int i = 1;

	for (i = 1; i < 4; i++) {  // We need to have 4 elements(x1, y1, x2, y2)
		next = strtok(NULL, delim);
		if (!next) {  // 4 elements have not been found
			printf("Invalid command\n");
			return;
		}
		int length = strlen(next);
		for (int k = 0; k < length; k++)
			if (isalpha(next[k]) != 0) {  // verify they are not letters
				printf("Invalid command\n");
				return;
			}
		element[i] = next;
		number[i] = atoi(element[i]);
	}

	int x1, y1, x2, y2;
	x1 = number[0];
	y1 = number[1];
	x2 = number[2];
	y2 = number[3];

	// Switch positions if they are not in ascending order (x1 < x2, y1 < y2)
	int aux;
	if (x1 > x2) {
		aux = x1;
		x1 = x2;
		x2 = aux;
	}
	if (y1 > y2) {
		aux = y1;
		y1 = y2;
		y2 = aux;
	}

	// Verify if the coordinates are valid for the image.
	if (validate_selection(image, x1, y1, x2, y2) == 1) {
		image->select->x1 = x1;
		image->select->x2 = x2;
		image->select->y1 = y1;
		image->select->y2 = y2;
		printf("Selected %d %d %d %d\n", x1, y1, x2, y2);

	} else {
		printf("Invalid set of coordinates\n");
	}
}

void save_text(image_struct *image, char *file_path)
{
	// This function saves the image in a text file.
	FILE *pf = fopen(file_path, "wt");
	if (!pf) {
		printf("Cannot open %s\n", file_path);
		return;
	}

	if (strcmp(image->image_type, "P2") == 0 ||
		strcmp(image->image_type, "P5") == 0) {
		fprintf(pf, "P2\n");
		fprintf(pf, "%d %d\n", image->width, image->height);
		fprintf(pf, "%d\n", image->max_value);

		for (int i = 0; i < image->height; i++) {
			for (int j = 0; j < image->width; j++) {
				if (j == image->width - 1)
					fprintf(pf, "%d\n", image->pixel[i][j].grayscale);
				else
					fprintf(pf, "%d ", image->pixel[i][j].grayscale);
			}
		}
	}

	if (strcmp(image->image_type, "P3") == 0 ||
		strcmp(image->image_type, "P6") == 0) {
		fprintf(pf, "P3\n");
		fprintf(pf, "%d %d\n", image->width, image->height);
		fprintf(pf, "%d\n", image->max_value);

		for (int i = 0; i < image->height; i++) {
			for (int j = 0; j < image->width; j++) {
				if (j == image->width - 1) {
					fprintf(pf, "%d %d %d\n", image->pixel[i][j].r,
							image->pixel[i][j].g, image->pixel[i][j].b);
				} else {
					fprintf(pf, "%d %d %d ", image->pixel[i][j].r,
							image->pixel[i][j].g, image->pixel[i][j].b);
				}
			}
		}
	}

	printf("Saved %s\n", file_path);

	fclose(pf);
}

void save_binary(image_struct *image, char *file_path)
{
	// This function saves the image in a binary file.
	FILE *pf = fopen(file_path, "wb");
	if (!pf) {
		printf("Cannot open %s\n", file_path);
		return;
	}

	// The image type, width, height and max_value are written as ASCII.
	if (strcmp(image->image_type, "P2") == 0 ||
		strcmp(image->image_type, "P5") == 0)
		fprintf(pf, "P5\n");
	else
		fprintf(pf, "P6\n");
	fprintf(pf, "%d %d\n", image->width, image->height);
	fprintf(pf, "%d\n", image->max_value);

	// Grayscale
	if (strcmp(image->image_type, "P2") == 0 ||
		strcmp(image->image_type, "P5") == 0) {
		for (int i = 0; i < image->height; i++) {
			// We have to transform the pixel values(ints) into chars.
			unsigned char *v_aux =
				(unsigned char *)malloc(sizeof(unsigned char) * image->width);
			if (!v_aux) {
				fprintf(stderr, "malloc() for array failed\n");
				return;
			}

			for (int j = 0; j < image->width; j++)
				v_aux[j] = (unsigned char)(image->pixel[i][j].grayscale);

			fwrite(v_aux, sizeof(unsigned char), image->width, pf);
			free(v_aux);
		}
	}

	// Colour
	if (strcmp(image->image_type, "P3") == 0 ||
		strcmp(image->image_type, "P6") == 0) {
		for (int i = 0; i < image->height; i++) {
			for (int j = 0; j < image->width; j++) {
				// We have to transform the pixel values(ints) into chars.
				unsigned char r, g, b;
				r = (unsigned char)image->pixel[i][j].r;
				g = (unsigned char)image->pixel[i][j].g;
				b = (unsigned char)image->pixel[i][j].b;

				fwrite(&r, sizeof(unsigned char), 1, pf);
				fwrite(&g, sizeof(unsigned char), 1, pf);
				fwrite(&b, sizeof(unsigned char), 1, pf);
			}
		}
	}

	printf("Saved %s\n", file_path);

	fclose(pf);
}

void save(image_struct *image, int loaded_img_now, char *delim)
{
	// File_path will be the next word on the line we previously read in main.
	char *file_path = strtok(NULL, delim);

	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return;
	}

	// We determine the type of saving whether there is a next word after the
	// file path. If there is not, we save as binary. If there is, and that word
	// is "ascii", we save as text.
	char *type = strtok(NULL, delim);
	if (!type) {
		save_binary(image, file_path);
		return;
	}
	if (strcmp(type, "ascii") == 0)
		save_text(image, file_path);
}

//=======================
// EDITING FUNCTIONS
//=======================

image_struct *crop(image_struct *initial, int loaded_img_now, char *delim)
{
	if (strtok(NULL, delim)) {	// CROP has no other parameter
		printf("Invalid command\n");
		return initial;
	}
	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return initial;
	}

	image_struct *result;
	if (image_alloc(&result) == 0)
		return NULL;

	strcpy(result->image_type, initial->image_type);
	result->max_value = initial->max_value;

	select_struct *select = initial->select;
	result->height = select->y2 - select->y1;
	result->width = select->x2 - select->x1;

	if (pixel_alloc(&result->pixel, result->height, result->width) == 0)
		return NULL;

	for (int i = 0; i < result->height; i++) {
		for (int j = 0; j < result->width; j++) {
			result->pixel[i][j] =
				initial->pixel[select->y1 + i][select->x1 + j];
		}
	}

	if (select_alloc(&result->select) == 0)
		return NULL;
	result->select->x1 = 0;
	result->select->x2 = result->width;
	result->select->y1 = 0;
	result->select->y2 = result->height;

	printf("Image cropped\n");
	free_img(initial);
	return result;
}

void histogram(image_struct *image, int loaded_img_now, char *delim)
{
	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return;
	}

	// Determine the next elements in line
	char *parameter = strtok(NULL, delim);
	if (!parameter) {
		printf("Invalid command\n");
		return;
	}
	int max_stars = atoi(parameter);

	parameter = strtok(NULL, delim);
	if (!parameter) {
		printf("Invalid command\n");
		return;
	}
	int bins_nr = atoi(parameter);

	parameter = strtok(NULL, delim);
	if (parameter) {  // HISTOGRAM needs only 2 parameters
		printf("Invalid command\n");
		return;
	}

	if (strcmp(image->image_type, "P3") == 0 ||
		strcmp(image->image_type, "P6") == 0) {
		printf("Black and white image needed\n");
		return;
	}

	int *array_freq_bins;
	if (array_alloc(&array_freq_bins, bins_nr) == 0)
		return;
	for (int i = 0; i < bins_nr; i++)
		array_freq_bins[i] = 0;

	// We determine how many pixel values are included in an interval of numbers
	// that create a bin. We need to store it as a double to keep in mind if the
	// total number of values is not divisible by the number of bins.
	double interval = (double)(image->max_value + 1) / bins_nr;

	for (int i = 0; i < image->height; i++) {
		for (int j = 0; j < image->width; j++) {
			// We determine in which bin is the pixel found by dividing to the
			// interval and approximating the value to the closest lower
			// integer. We will have a result of {0, 1, 2,..., bins_nr - 1}.
			double pos_bin_d = (double)image->pixel[i][j].grayscale / interval;
			int pos_bin = floor(pos_bin_d);

			// Increase the number of elements in the frequenct array.
			array_freq_bins[pos_bin]++;
		}
	}

	// Determine the maximum value in the frequency array.
	int max_freq = 0;
	for (int i = 0; i < bins_nr; i++)
		if (array_freq_bins[i] > max_freq)
			max_freq = array_freq_bins[i];

	// For each bin, we determine the number of stars and print the result
	// accordingly.
	for (int i = 0; i < bins_nr; i++) {
		int nr_stars = (array_freq_bins[i] * max_stars) / max_freq;
		printf("%d\t|\t", nr_stars);
		for (int j = 0; j < nr_stars; j++)
			printf("*");
		printf("\n");
	}

	free(array_freq_bins);
}

double clamp(double nr, int min, int max)
{
	// Make sure "nr" doesn't exceed the limits "min" or "max".
	if (nr < min)
		return min;
	if (nr > max)
		return max;
	return nr;
}

void equalize(image_struct *image, int loaded_img_now, char *delim)
{
	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return;
	}

	char *rest_line = strtok(NULL, delim);
	if (rest_line) {  // EQUALIZE has no parameter
		printf("Invalid command\n");
		return;
	}

	if (strcmp(image->image_type, "P3") == 0 ||
		strcmp(image->image_type, "P6") == 0) {
		printf("Black and white image needed\n");
		return;
	}

	int area = image->height * image->width;

	// We determine how many pixels of the same value exist with a frequency
	// array.
	int *array_freq_pixels;
	if (array_alloc(&array_freq_pixels, (image->max_value + 1)) == 0)
		return;
	for (int i = 0; i <= image->max_value; i++)
		array_freq_pixels[i] = 0;

	for (int i = 0; i < image->height; i++) {
		for (int j = 0; j < image->width; j++) {
			int c = image->pixel[i][j].grayscale;
			array_freq_pixels[c]++;
		}
	}

	int *new_values;
	if (array_alloc(&new_values, (image->max_value + 1)) == 0)
		return;

	for (int i = 0; i <= image->max_value; i++) {
		// For each value of a pixel, we calculate the sum of appearances of
		// pixels with values lower or equal with that pixel.
		int partial_sum_freq = 0;
		for (int j = 0; j <= i; j++)
			partial_sum_freq += array_freq_pixels[j];

		// We calculate the new value of the pixel with the formula provided in
		// the documentation.
		double new_val =
			(double)image->max_value * (double)partial_sum_freq / (double)area;
		new_values[i] = round(clamp(new_val, 0, image->max_value));
	}

	// We replace the old values with the new ones.
	for (int i = 0; i < image->height; i++) {
		for (int j = 0; j < image->width; j++) {
			image->pixel[i][j].grayscale =
				new_values[image->pixel[i][j].grayscale];
		}
	}

	free(array_freq_pixels);
	free(new_values);
	printf("Equalize done\n");
}

int border_kernel_min(int number)
{
	// Determines the starting coordinate from which the kernel should be
	// applied.
	// Because we have a 3x3 matrix to apply on the image, we need a value
	// greater than 0.
	if (number > 0)
		return number;
	else
		return 1;
}

int border_kernel_max(int max_selected, int max_img)
{
	// Determines the ending coordinate until which the kernel should be
	// applied.
	// Because we have a 3x3 matrix to apply on the image, we need a value
	// lower than the height or width of the image.
	if (max_selected < max_img)
		return max_selected;
	else
		return (max_selected - 1);
}

image_struct *apply_kernel(image_struct *initial, double mat[][3],
						   char *apply_type)
{
	if (strcmp(initial->image_type, "P2") == 0 ||
		strcmp(initial->image_type, "P5") == 0) {
		printf("Easy, Charlie Chaplin\n");
		return initial;
	}

	image_struct *result = copy_image(initial);
	int i_min, i_max, j_min, j_max;

	// We determine the starting and ending coordinates for the kernel
	// application.
	i_min = border_kernel_min(initial->select->y1);
	j_min = border_kernel_min(initial->select->x1);
	i_max = border_kernel_max(initial->select->y2, initial->height);
	j_max = border_kernel_max(initial->select->x2, initial->width);

	if (strcmp(initial->image_type, "P3") == 0 ||
		strcmp(initial->image_type, "P6") == 0) {
		// Because we have a 3x3 matrix and the element we calculate for is in
		// its center, we have to start from [i - 1][j - 1] until [i + 1][j +
		// 1].
		for (int i_mat = i_min - 1; i_mat < i_max - 1; i_mat++) {
			for (int j_mat = j_min - 1; j_mat < j_max - 1; j_mat++) {
				double sumR = 0.0;
				double sumG = 0.0;
				double sumB = 0.0;
				for (int i = 0; i < 3; i++) {
					for (int j = 0; j < 3; j++) {
						sumR += (double)mat[i][j] *
								initial->pixel[i_mat + i][j_mat + j].r;
						sumG += (double)mat[i][j] *
								initial->pixel[i_mat + i][j_mat + j].g;
						sumB += (double)mat[i][j] *
								initial->pixel[i_mat + i][j_mat + j].b;
					}
				}
				sumR = round(sumR);
				sumG = round(sumG);
				sumB = round(sumB);

				// We have to make sure the calculated values are not
				// negative or go beyond the maximum value.
				result->pixel[i_mat + 1][j_mat + 1].r =
					clamp(sumR, 0, initial->max_value);
				result->pixel[i_mat + 1][j_mat + 1].g =
					clamp(sumG, 0, initial->max_value);
				result->pixel[i_mat + 1][j_mat + 1].b =
					clamp(sumB, 0, initial->max_value);
			}
		}
	}

	printf("APPLY %s done\n", apply_type);
	free_img(initial);
	return result;
}

image_struct *apply(image_struct *image, int loaded_img_now, char *delim)
{
	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return image;
	}
	char *apply_type = strtok(NULL, delim);
	if (!apply_type) {	// We need to have a parameter.
		printf("Invalid command\n");
		return image;
	}

	image_struct *result;

	// For every type, we initialize the kernel matrix and apply it on the
	// image.
	if (strcmp(apply_type, "EDGE") == 0) {
		double mat[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
		result = apply_kernel(image, mat, apply_type);
	} else {
		if (strcmp(apply_type, "SHARPEN") == 0) {
			double mat[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};
			result = apply_kernel(image, mat, apply_type);
		} else {
			if (strcmp(apply_type, "BLUR") == 0) {
				double mat[3][3] = {{1.0 / 9, 1.0 / 9, 1.0 / 9},
									{1.0 / 9, 1.0 / 9, 1.0 / 9},
									{1.0 / 9, 1.0 / 9, 1.0 / 9}};
				result = apply_kernel(image, mat, apply_type);
			} else {
				if (strcmp(apply_type, "GAUSSIAN_BLUR") == 0) {
					double mat[3][3] = {{1.0 / 16, 2.0 / 16, 1.0 / 16},
										{2.0 / 16, 4.0 / 16, 2.0 / 16},
										{1.0 / 16, 2.0 / 16, 1.0 / 16}};
					result = apply_kernel(image, mat, apply_type);
				} else {
					printf("APPLY parameter invalid\n");
					return image;
				}
			}
		}
	}
	return result;
}

image_struct *full_rotation_90_back(image_struct *image)
{
	image_struct *result;
	if (image_alloc(&result) == 0)
		return NULL;

	strcpy(result->image_type, image->image_type);
	result->height = image->width;	// We swap the height with the width.
	result->width = image->height;
	result->max_value = image->max_value;

	if (pixel_alloc(&result->pixel, result->height, result->width) == 0)
		return NULL;
	if (select_alloc(&result->select) == 0)
		return NULL;
	result->select->x1 = image->select->x1;
	result->select->y1 = image->select->y1;
	result->select->x2 = image->select->y2;
	result->select->y2 = image->select->x2;

	// We go through the resulting matrix from the first column upward (which
	// corresponds to the first line in the initial matrix from left to right)
	// towards the last column of the matrix(which corresponds to the last line
	// in the initial matrix).
	int i_initial = 0;
	for (int j = 0; j < result->width; j++) {
		int j_initial = 0;
		for (int i = result->height - 1; i >= 0; i--) {
			result->pixel[i][j] = image->pixel[i_initial][j_initial];
			j_initial++;
		}
		i_initial++;
	}

	free_img(image);

	return result;
}

image_struct *full_rotation_90(image_struct *image)
{
	image_struct *result;
	if (image_alloc(&result) == 0)
		return NULL;

	strcpy(result->image_type, image->image_type);
	result->height = image->width;	// We swap the height with the width.
	result->width = image->height;
	result->max_value = image->max_value;

	if (pixel_alloc(&result->pixel, result->height, result->width) == 0)
		return NULL;
	if (select_alloc(&result->select) == 0)
		return NULL;
	result->select->x1 = image->select->x1;
	result->select->y1 = image->select->y1;
	result->select->x2 = image->select->y2;
	result->select->y2 = image->select->x2;

	// We go through the resulting matrix from the last column downward (which
	// corresponds to the first line in the initial matrix from left to right)
	// towards the first column of the matrix(which corresponds to the last line
	// in the initial matrix).
	int i_initial = 0;
	for (int j = result->width - 1; j >= 0; j--) {
		int j_initial = 0;
		for (int i = 0; i < result->height; i++) {
			result->pixel[i][j] = image->pixel[i_initial][j_initial];
			j_initial++;
		}
		i_initial++;
	}

	free_img(image);

	return result;
}

image_struct *select_rotation_90_back(image_struct *image)
{
	image_struct *result = copy_image(image);
	select_struct *select = image->select;

	int difference_x = select->x2 - select->x1;
	int difference_y = select->y2 - select->y1;

	// We copy the selected pixels in an auxiliary matrix
	pixel_struct **pixel_aux;
	if (pixel_alloc(&pixel_aux, difference_y, difference_x) == 0)
		return NULL;

	for (int i = 0; i < difference_x; i++)
		for (int j = 0; j < difference_y; j++)
			pixel_aux[i][j] = image->pixel[select->y1 + i][select->x1 + j];

	// We rotate the selected pixels in another auxiliary matrix.
	pixel_struct **pixel_aux_res;
	if (pixel_alloc(&pixel_aux_res, difference_y, difference_x) == 0)
		return NULL;

	for (int i = 0; i < difference_x; i++)
		for (int j = 0; j < difference_y; j++)
			pixel_aux_res[i][j] = pixel_aux[j][difference_x - 1 - i];

	// We replace the selected pixels from the initial matrix with the new
	// rotated ones.
	int i_curent = 0;
	for (int i = select->y1; i < select->y2; i++) {
		int j_curent = 0;
		for (int j = select->x1; j < select->x2; j++) {
			result->pixel[i][j] = pixel_aux_res[i_curent][j_curent];
			j_curent++;
		}
		i_curent++;
	}

	free_img(image);
	free_pixel(pixel_aux, difference_y);
	free_pixel(pixel_aux_res, difference_y);

	return result;
}

image_struct *select_rotation_90(image_struct *image)
{
	image_struct *result = copy_image(image);
	select_struct *select = result->select;

	int difference_x = select->x2 - select->x1;
	int difference_y = select->y2 - select->y1;

	// We copy the selected pixels in an auxiliary matrix
	pixel_struct **pixel_aux;
	if (pixel_alloc(&pixel_aux, difference_y, difference_x) == 0)
		return NULL;

	for (int i = 0; i < difference_x; i++)
		for (int j = 0; j < difference_y; j++)
			pixel_aux[i][j] = image->pixel[select->y1 + i][select->x1 + j];

	// We rotate the selected pixels in another auxiliary matrix.
	pixel_struct **pixel_aux_res;
	if (pixel_alloc(&pixel_aux_res, difference_y, difference_x) == 0)
		return NULL;

	for (int i = 0; i < difference_x; i++)
		for (int j = 0; j < difference_y; j++)
			pixel_aux_res[i][j] = pixel_aux[difference_x - 1 - j][i];

	// We replace the selected pixels from the initial matrix with the new
	// rotated ones.
	int i_curent = 0;
	for (int i = select->y1; i < select->y2; i++) {
		int j_curent = 0;
		for (int j = select->x1; j < select->x2; j++) {
			result->pixel[i][j] = pixel_aux_res[i_curent][j_curent];
			j_curent++;
		}
		i_curent++;
	}

	free_img(image);
	free_pixel(pixel_aux, difference_y);
	free_pixel(pixel_aux_res, difference_y);
	return result;
}

image_struct *rotate(image_struct *image, int loaded_img_now, char *delim)
{
	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return image;
	}
	char *elem = strtok(NULL, delim);  // Needs a parameter (the angle)
	int rotation_nr = atoi(elem);

	if (rotation_nr % 90 != 0) {
		printf("Unsupported rotation angle\n");
		return image;
	}

	select_struct *select = image->select;	// For easier use

	// We verify if the whole image is selected.
	if (select->x1 == 0 && select->y1 == 0 && select->x2 == image->width &&
		select->y2 == image->height) {	// FULL ROTATION
		if (rotation_nr == 0) {
			printf("Rotated %d\n", rotation_nr);
			return image;
		}

		image_struct *result;
		int times = rotation_nr / 90;
		if (times < 0) {  // NEGATIVE ANGLE
			times = -times;
			for (int k = 0; k < times; k++) {
				result = full_rotation_90_back(image);
				image = copy_image(result);
				free_img(result);
			}
		} else {  // POSITIVE ANGLE
			for (int k = 0; k < times; k++) {
				result = full_rotation_90(image);
				image = copy_image(result);
				free_img(result);
			}
		}
		printf("Rotated %d\n", rotation_nr);
		return image;
	}
	if (select->x1 != 0 || select->y1 != 0 || select->x2 != image->width ||
		select->y2 != image->height) {	// SELECTION ROTATION
		if (select->x2 - select->x1 != select->y2 - select->y1) {
			printf("The selection must be square\n");
			return image;
		}

		image_struct *result;
		int times = rotation_nr / 90;
		if (times < 0) {  // NEGATIVE ANGLE
			times = -times;
			for (int k = 0; k < times; k++) {
				result = select_rotation_90_back(image);
				image = copy_image(result);
				free_img(result);
			}
		} else {  // POSITIVE ANGLE
			for (int k = 0; k < times; k++) {
				result = select_rotation_90(image);
				image = copy_image(result);
				free_img(result);
			}
		}
	}
	printf("Rotated %d\n", rotation_nr);
	return image;
}

int exit_program(image_struct *image, int loaded_img_now)
{
	// If there is an image loaded, we deallocate its memory and return 1,
	// so in main we will be able to verify it and end the program.
	if (loaded_img_now == 0) {
		printf("No image loaded\n");
		return 0;
	}
	free_img(image);
	return 1;
}

int command_type(char *command)
{
	// Translate the commands into numbers so we will be able to use switch
	// case.
	if (strcmp(command, "LOAD") == 0)
		return 1;
	if (strcmp(command, "SELECT") == 0)
		return 2;
	if (strcmp(command, "HISTOGRAM") == 0)
		return 3;
	if (strcmp(command, "EQUALIZE") == 0)
		return 4;
	if (strcmp(command, "CROP") == 0)
		return 5;
	if (strcmp(command, "APPLY") == 0)
		return 6;
	if (strcmp(command, "SAVE") == 0)
		return 7;
	if (strcmp(command, "EXIT") == 0)
		return 8;
	if (strcmp(command, "ROTATE") == 0)
		return 9;
	return 0;
}

int main(void)
{
	char line[NMAX_LINE];
	char *command;
	char delim[] = "\n ";  // to separate the words on a line
	image_struct *image;
	int loaded_img_now = 0;	 // to keep track whether there is a loaded image

	// We read the line on every loop. The program either stops with the
	// "EXIT" command or when there are no more lines to read.
	while (fgets(line, NMAX_LINE, stdin)) {
		command = strtok(line, delim);
		int type = command_type(command);
		switch (type) {
			case 1: {  // LOAD
				image = load(image, &loaded_img_now, delim);
				break;
			}
			case 2: {  // SELECT / SELECT ALL
				select_image(image, loaded_img_now, delim);
				break;
			}
			case 3: {  // HISTOGRAM
				histogram(image, loaded_img_now, delim);
				break;
			}
			case 4: {  // EQUALIZE
				equalize(image, loaded_img_now, delim);
				break;
			}
			case 5: {  // CROP
				image = crop(image, loaded_img_now, delim);
				break;
			}
			case 6: {  // APPLY
				image = apply(image, loaded_img_now, delim);
				break;
			}
			case 7: {  // SAVE
				save(image, loaded_img_now, delim);
				break;
			}
			case 8: {  // EXIT
				if (exit_program(image, loaded_img_now) == 1)
					return 0;
				break;
			}
			case 9: {  // ROTATE
				image = rotate(image, loaded_img_now, delim);
				break;
			}
			default: {	// OTHER
				printf("Invalid command\n");
			}
		}
	}
	return 0;
}
