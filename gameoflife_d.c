#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x, y)  ((y)*(width) + (x))

long TimeSteps = 100;

void writeHeaderVTK(long timestep, int w, int h) {
    char filename[2048];

    float deltax = 1.0;
    long nxy = w * h * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%05ld%s", "header", timestep, ".pvti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"PImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<PImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", 0, w, 0, h,
            0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<PCellData Scalars=\"%s\">\n", "gol");
    fprintf(fp, "<PDataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", "gol");
    fprintf(fp, "</PCellData>\n");
    for (int i = 0; i < 4; i++) {
        int xstart;
        int ystart;

        if (i == 0) {
            xstart = 0, ystart = 0;
        } else if (i == 1) {
            xstart = 0, ystart = 15;
        } else if (i == 2) {
            xstart = 15, ystart = 0;
        } else if (i == 3) {
            xstart = 15, ystart = 15;
        }
        int threadh = h / 2, threadw = w / 2


        fprintf(fp, "<Piece Extent=\"%d %d %d %d %d %d\" Source=\"%s-%d-%05ld%s\"/>", xstart, xstart + threadw, ystart,
                ystart + threadh, 0, 0, "gol", i, timestep, ".vti");
    }

    fprintf(fp, "</PImageData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void writeVTK(long timestep, double *data, char prefix[1024], int num, int gameW, int offsetX, int offsetY) {
    char filename[2048];
    int x, y;
    int w = 15, h = 15;
    float deltax = 1.0;
    long nxy = w * h * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%d-%05ld%s", prefix, num, timestep, ".vti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX,
            offsetX + w, offsetY, offsetY + h, 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *) &nxy, sizeof(long), 1, fp);

    for (y = offsetY; y < h + offsetY; y++) {
        for (x = offsetX; x < w + offsetX; x++) {
            float value = data[calcIndex(gameW, x, y)];
            fwrite((unsigned char *) &value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}


void show(double *currentfield, int w, int h) {
    printf("\033[H");
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x, y)] ? "\033[07m  \033[m" : "  ");
        //printf("\033[E");
        printf("\n");
    }
    fflush(stdout);
}

int coutLifingsPeriodic(double *currentfield, int x, int y, int w, int h) {
    int n = 0;
    for (int y1 = y - 1; y1 <= y + 1; y1++) {
        for (int x1 = x - 1; x1 <= x + 1; x1++) {
            if (currentfield[calcIndex(w, (x1 + w) % w, (y1 + h) % h)]) {
                n++;
            }
        }
    }
    return n;
}


void evolve(long timestep, double *currentfield, double *newfield, int w, int h) {

#pragma omp parallel for
    for (int i = 0; i < 4; i++) {
        printf("thread %d: i = %d\n", omp_get_thread_num(), i);
        int xstart, xend, ystart, yend;
        if (i == 0) {
            xstart = 0, xend = 15, ystart = 0, yend = 15;
            writeHeaderVTK(timestep, w, h);
        } else if (i == 1) {
            xstart = 0, xend = 15, ystart = 15, yend = 30;
        } else if (i == 2) {
            xstart = 15, xend = 30, ystart = 0, yend = 15;
        } else if (i == 3) {
            xstart = 15, xend = 30, ystart = 15, yend = 30;
        }

        for (int y = ystart; y < yend; y++) {
            for (int x = xstart; x < xend; x++) {
                int n = coutLifingsPeriodic(currentfield, x, y, w, h);
                if (currentfield[calcIndex(w, x, y)]) n--;
                newfield[calcIndex(w, x, y)] = (n == 3 || (n == 2 && currentfield[calcIndex(w, x, y)]));


                //newfield[calcIndex(w, x,y)] = !newfield[calcIndex(w, x,y)];
            }
        }
        writeVTK(timestep, currentfield, "gol", i, w, xstart, ystart);
    }
}

void filling(double *currentfield, int w, int h) {
    int i;
    for (i = 0; i < h * w; i++) {
        currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
    }
}

void game(int w, int h) {
    double *currentfield = calloc(w * h, sizeof(double));
    double *newfield = calloc(w * h, sizeof(double));

    //printf("size unsigned %d, size long %d\n",sizeof(float), sizeof(long));

    filling(currentfield, w, h);
    long t;
    for (t = 0; t < TimeSteps; t++) {
        show(currentfield, w, h);
        evolve(t, currentfield, newfield, w, h);

        printf("%ld timestep\n", t);
        //writeVTK2(t,currentfield,"gol", w, h);

        usleep(200000);

        //SWAP
        double *temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

    free(currentfield);
    free(newfield);

}

int main(int c, char **v) {
    int w = 0, h = 0;
    if (c > 1) w = atoi(v[1]); ///< read width
    if (c > 2) h = atoi(v[2]); ///< read height
    if (w <= 0) w = 30; ///< default width
    if (h <= 0) h = 30; ///< default height
    game(w, h);
}
