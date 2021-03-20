#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include "mpi.h"

#define calcIndex(width, x, y)  ((y)*(width) + (x))

//long TimeSteps = 100;

void writeMainVTK(long timestep, int width, int height, int nx, int ny, int Px, int Py) {
    char filename[2048];

    float deltax = 1.0;

    snprintf(filename, sizeof(filename), "%s-%05ld%s", "header", timestep, ".pvti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"PImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<PImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", 0, width,
            0, height,
            0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<PCellData Scalars=\"%s\">\n", "gol");
    fprintf(fp, "<PDataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", "gol");
    fprintf(fp, "</PCellData>\n");

    int num_threads = Px * Py;
    for (int i = 0; i < num_threads; i++) {

        int x_start = (i % Px) * nx;
        int x_end = x_start + nx;
        int y_start = (i / Px) * ny;
        int y_end = y_start + ny;


        fprintf(fp, "<Piece Extent=\"%d %d %d %d %d %d\" Source=\"%s-%d-%05ld%s\"/>", x_start, x_end, y_start,
                y_end, 0, 0, "gol", i, timestep, ".vti");
    }

    fprintf(fp, "</PImageData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void writeVTK(long timestep, double *currentfield, char prefix[1024], int threadNum, int width, int height, int x_start,
              int x_end, int y_start, int y_end) {
    char filename[2048];

    float deltax = 1.0;
    long nxy = width * height * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%d-%05ld%s", prefix, threadNum, timestep, ".vti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", x_start,
            x_end, y_start, y_end, 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *) &nxy, sizeof(long), 1, fp);

    for (int y = y_start; y < y_end; y++) {
        for (int x = x_start; x < x_end; x++) {
            float value = currentfield[calcIndex(width, x, y)];
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
        printf("\033[E");
    }
    fflush(stdout);
}

int countLifingsPeriodic(double *currentfield, int x, int y, int w, int h) {
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


void evolve(double *currentfield, double *newfield, int width, int height, int num_threads, int rank) {

    MPI_Request request;
    MPI_Isend(currentfield + width, width, MPI_DOUBLE, ((rank - 1) + num_threads) % num_threads, 42, MPI_COMM_WORLD, &request);
    MPI_Isend(currentfield + height * width, width, MPI_DOUBLE, ((rank + 1) + num_threads) % num_threads, 42, MPI_COMM_WORLD, &request);

    MPI_Status status;
    MPI_Recv(currentfield, width, MPI_DOUBLE, ((rank + 1) + num_threads) % num_threads, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(currentfield + (height + 1) * width, width, MPI_DOUBLE, ((rank - 1) + num_threads) % num_threads, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

//        if (i == 0) {
//            writeMainVTK(timestep, width, height, nx, ny, Px, Py);
//        }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int n = countLifingsPeriodic(currentfield, x, y, width, height);
            int index = calcIndex(width, x, y);
            if (currentfield[index]) n--;
            newfield[index] = (n == 3 || (n == 2 && currentfield[index]));
        }
    }
//        writeVTK(timestep, currentfield, "gol", i, width, height, x_start, x_end, y_start, y_end);
}

void filling(double *currentfield, int w, int h) {
    srand(time(NULL));
    for (int i = 0; i < h * w; i++) {
//        currentfield[i] = rand() & 1;
        currentfield[i] = (rand() < RAND_MAX / 2) ? 1 : 0; ///< init domain randomly
//        currentfield[i] = 1;
    }
}

void game(int TimeSteps, int height, int width, int num_threads, int rank) {

    printf("width: %d\nheight: %d", width, height);

    double *currentfield = calloc(width * (height + 2), sizeof(double));
    double *newfield = calloc(width * (height + 2), sizeof(double));


    filling(currentfield, width, height);
    long time_step;
    for (time_step = 0; time_step < TimeSteps; time_step++) {
        if (rank==0){
            writeMainVTK(time_step, width, height*num_threads, width, height, 1, num_threads);
        }
        writeVTK(time_step, currentfield, "gol", rank, width, height, 0, width, rank*height, rank*height+height);
        evolve(currentfield, newfield, width, height, num_threads, rank);


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
    int TimeSteps = 0, height = 5, width = 5;

    if (c > 1) TimeSteps = atoi(v[1]);
//    if (c > 2) nx = atoi(v[2]);
//    if (c > 3) ny = atoi(v[3]);
//    if (c > 4) Px = atoi(v[4]);
//    if (c > 5) Py = atoi(v[5]);

    if (TimeSteps <= 0) TimeSteps = 20;
//    if (nx <= 0) nx = 5;
//    if (ny <= 0) ny = 5;
//    if (Px <= 0) Px = 5;
//    if (Py <= 0) Py = 5;

    int rank, num_threads;

    MPI_Init(&c, &v);
    MPI_Comm_size(MPI_COMM_WORLD, &num_threads);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("Initialized, Size: %d, Rank: %d\n", num_threads, rank);

    game(TimeSteps, height, width, num_threads, rank);

    MPI_Finalize();
    return 0;
}
