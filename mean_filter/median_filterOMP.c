/*
Most of the code here is based on this code: https://gist.github.com/niw/5963798
The rest has been edited and based on the rosseta stone approach of the median filter using a PPM file
This is the same as the other median_filter.c file but with OpenMP to paralellize part of the process.
NOTE: It will only accept PNG files for input and output
AUTHOR: AngelGCL
*/
#include "rgb_filt.h"
#include <stdlib.h>
#include <stdio.h>
#include <libpng16/png.h>
#include <omp.h>
#include <time.h>
#include <sys/sysinfo.h>



int cmpfunc(const void *a, const void *b);

int width, height;
png_byte color_type;
png_byte bit_s;
png_bytep *row_pointers;
RGB *pixels;

void deconstructor(int initH, int endH, int endW, int iter)
{
    int x,y;
    int i = iter;
    for (y = initH; y < endH; y++)
    {
        png_byte *row = row_pointers[y];
        for (x = 0; x < endW; x++)
        {
            png_byte *ptr = &(row[x * 4]);
            pixels[i].r = ptr[0];
            pixels[i].g = ptr[1];
            pixels[i].b = ptr[2];
            i++;
        }
    }
}

void reconstructor(int initH, int endH, int endW, int iter)
{
    int x,y;
    int i = iter;
    for (y = initH; y < endH; y++)
    {
        png_byte *row = row_pointers[y];
        for (x = 0; x < endW; x++)
        {
            png_byte *ptr = &(row[x * 4]);
            ptr[0] = pixels[i].r;
            ptr[1] = pixels[i].g;
            ptr[2] = pixels[i].b;
            i++;
        }
    }
}
void read_png(char *file, char *file2)
{
    FILE *documen = fopen(file, "rb");

    png_structp pic = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pic)
        abort();

    png_inforp infor = png_create_info_struct(pic);
    if (!infor)
        abort();

    if (setjmp(png_jmpbuf(pic)))
        abort();

    png_init_io(pic, documen);

    png_read_info(pic, infor);

    width = png_get_image_width(pic, infor);
    height = png_get_image_height(pic, infor);
    color_type = png_get_color_type(pic, infor);
    bit_s = png_get_bit_depth(pic, infor);

    if (bit_s == 16)
        png_set_strip_16(pic);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(pic);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_s < 8)
        png_set_expand_gray_1_2_4_to_8(pic);

    if (png_get_valid(pic, infor, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(pic);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(pic, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(pic);

    png_read_update_info(pic, infor);

    row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    int p;
    for (p = 0; p < height; p++)
    {
        row_pointers[p] = (png_byte *)malloc(png_get_rowbytes(pic, infor));
    }

    png_read_image(pic, row_pointers);

    fclose(documen);

    int y, x;
    int numOfPix = width * height;
    pixels = malloc(numOfPix * sizeof(RGB));
    int i = 0;


    i = (height/2) * width;
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            deconstructor(0, height/2+1, width, 0);
        }
        #pragma omp section
        {
            deconstructor(height/2, height, width, i);
        }
    }
    #pragma omp barrier

    pixels = filtImage(width, height, pixels, bit_s);
    
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            reconstructor(0, height/2+1, width, 0);
        }
        #pragma omp section
        {
            reconstructor(height/2 + 1, height, width, i);
        }
    }
    #pragma omp barrier

    FILE *out_file = fopen(file2, "wb");
    if (!documen)
        abort();

    png_structp out = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!out)
        abort();

    png_infop infor2 = png_create_info_struct(out);
    if (!infor2)
        abort();

    if (setjmp(png_jmpbuf(out)))
        abort();

    png_init_io(out, out_file);

    png_set_IHDR(
        out,
        infor2,
        width, height,
        bit_s,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info(out, infor2);

    png_write_image(out, row_pointers);
    png_write_end(out, NULL);

    for (p = 0; p < height; p++)
    {
        free(row_pointers[p]);
    }
    free(row_pointers);

    fclose(out_file);
}

RGB *filtImage(int width, int height, const RGB *image, int n)
{
    int i, j, numPix, offset;
    int changedPos[n * n], mediansR[n * n], mediansG[n * n], mediansB[n * n];
    numPix = width * height;
    RGB *fixedPixels = malloc(numPix * sizeof(RGB));
    offset = (n - 1) / 2;
    for (i = 0; i < changedPos[n * n]; i++)
    {
        changedPos[i] = 0;
    }

    for (i = 0; i < numPix; i++)
    {
        int minWid, maxWid, currW, minHeight, maxHeight, currH, firstPos, lastPos;
        int append = 0, skipcount;
        int sumR = 0, sumG = 0, sumB = 0;
        skipcount = 0;
        firstPos = (i - width) - offset;
        lastPos = (i + width) + offset;
        currW = i % width;
        currH = i / width;
        minHeight = currH - offset;
        maxHeight = currH + offset;
        minWid = currW - offset;
        maxWid = currW + offset;
        if (minWid < 0)
        {
            minWid = 0;
        }
        if (maxWid > width)
        {
            maxWid = width;
        }
        if (minHeight < 0)
        {
            minHeight = 0;
        }
        if (maxHeight > height)
        {
            maxHeight = height;
        }


        for (j = firstPos; j <= lastPos; j++)
        {
            if ((j >= 0) && (j <= numPix))
            {
                currW = j % width;
                currH = j / width;
                if (((currW >= minWid) && (currW <= maxWid)) && ((currH >= minHeight) && (currH <= maxHeight)))
                {
                    changedPos[append] = j;
                    append++;
                }
            }
            skipcount++;
            if (skipcount == n)
            {
                j += (width - n);
                skipcount = 0;
            }
        }

        for (j = 0; j < append; j++)
        {
            mediansR[j] = image[changedPos[j]].r;
            mediansG[j] = image[changedPos[j]].g;
            mediansB[j] = image[changedPos[j]].b;
        }

        qsort(mediansR, append, sizeof(int), cmpfunc);
        qsort(mediansG, append, sizeof(int), cmpfunc);
        qsort(mediansB, append, sizeof(int), cmpfunc);
        if (append % 2 == 1)
        {

            for (j = 0; j < append; j++)
            {
                fixedPixels[changedPos[j]].r = mediansR[(append - 1) / 2];
                fixedPixels[changedPos[j]].g = mediansG[(append - 1) / 2];
                fixedPixels[changedPos[j]].b = mediansB[(append - 1) / 2];
            }
        }
        else
        {
            sumR = ((mediansR[append / 2] + mediansR[(append / 2) - 1])) / 2;
            sumG = ((mediansG[append / 2] + mediansG[(append / 2) - 1])) / 2;
            sumB = ((mediansB[append / 2] + mediansB[(append / 2) - 1])) / 2;

            for (j = 0; j < append; j++)
            {
                fixedPixels[changedPos[j]].r = sumR;
                fixedPixels[changedPos[j]].g = sumG;
                fixedPixels[changedPos[j]].b = sumB;
            }
        }
    }
    return fixedPixels;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Invalid command!\n");
        return -1;
    }
    int availCPU = get_nprocs();
    omp_set_num_threads(availCPU/2);
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    read_png(argv[1], argv[2]);
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Execution time with OMP: %f\n", cpu_time_used);
    return 0;
}
//Compare function used by the quicksort
int cmpfunc(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}