#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>
#include <time.h>

#define calcIndex(width, x, y)  ((y)*(width) + (x))

//long TimeSteps = 100;

void writeMainVTK(int nx, int ny, int Px, int Py, long timestep) {
    char filename[2048];

    float deltax = 1.0;

    int width = nx * Px;
    int height = ny * Py;

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

    int num_threads = nx * ny;
    for (int i = 0; i < num_threads; i++) {

        int x_start = (i % nx) * Px;
        int x_end = x_start + Px;
        int y_start = (i / nx) * Py;
        int y_end = y_start + Py;


        fprintf(fp, "<Piece Extent=\"%d %d %d %d %d %d\" Source=\"%s-%d-%05ld%s\"/>", x_start, x_end, y_start,
                y_end, 0, 0, "gol", i, timestep, ".vti");
    }

    fprintf(fp, "</PImageData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

//void writeVTK(long timestep, double *currentfield, char prefix[1024], int threadNum, int nx, int ny, int Px, int Py) {
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


void evolve(double *currentfield, double *newfield, int nx, int ny, int Px, int Py, long timestep) {

    int width = nx * Px;
    int height = ny * Py;

    int num_threads = nx * ny;

//    for (int i=0; i<num_threads; i++)
//    #pragma omp parallel num_threads(num_threads) shared(currentfield, newfield) firstprivate (timestep, x_start, x_end, y_start,  y_end, x, y)
#pragma omp parallel num_threads(num_threads) shared(currentfield, newfield) firstprivate(timestep, width, height, nx, ny, Px, Py)

    {
        int i = omp_get_thread_num();

        int x_start = (i % nx) * Px;
        int x_end = x_start + Px;
        int y_start = (i / nx) * Py;
        int y_end = y_start + Py;

        if (i == 0) {
//            writeMainVTK(nx, ny, Px, Py, timestep);
//            printf("Thread %d: x(%d-%d) y(%d-%d)\n", i, x_start, x_end, y_start, y_end);
//            printf("timestep    %ld\n", timestep);
//            printf("width       %d\n", width);
//            printf("height      %d\n", height);
//            printf("nx          %d\n", nx);
//            printf("ny          %d\n", ny);
//            printf("Px          %d\n", Px);
//            printf("Py          %d\n", Py);
        }

        for (int y = y_start; y < y_end; ++y) {
            for (int x = x_start; x < x_end; ++x) {
                int n = countLifingsPeriodic(currentfield, x, y, width, height);
                int index = calcIndex(width, x, y);
                if (currentfield[index]) n--;
                newfield[index] = (n == 3 || (n == 2 && currentfield[index]));
            }
        }
//        writeVTK(timestep, currentfield, "gol", i, width, height, x_start, x_end, y_start, y_end);
    }
}

void filling(double *currentfield, int w, int h) {
    srand(time(NULL));
    for (int i = 0; i < h * w; i++) {
//        currentfield[i] = rand() & 1;
        currentfield[i] = (rand() < RAND_MAX / 2) ? 1 : 0; ///< init domain randomly
    }
}

void game(int TimeSteps, int nx, int ny, int Px, int Py) {
    int width = nx * Px;
    int height = ny * Py;
    double *currentfield = calloc(width * height, sizeof(double));
    double *newfield = calloc(width * height, sizeof(double));

    filling(currentfield, width, height);
    long time_step;
    for (time_step = 0; time_step < TimeSteps; time_step++) {
//        show(currentfield, width, height);
        evolve(currentfield, newfield, nx, ny, Px, Py, time_step);

//        printf("%ld time_step\n", time_step);
//        writeVTK2(time_step, currentfield, "gol", w, h);

//        FILE *fp = writeHeader(time_step, "gol", w, h);
//        writeData(currentfield, w, h, fp);
//        writeFooter(fp);

//        usleep(2000000);

//SWAP
        double *temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

    free(currentfield);
    free(newfield);

}

int main(int c, char **v) {
    int TimeSteps = 0, nx = 0, ny = 0, Px = 0, Py = 0;

    if (c > 1) TimeSteps = atoi(v[1]);
    if (c > 2) nx = atoi(v[2]);
    if (c > 3) ny = atoi(v[3]);
    if (c > 4) Px = atoi(v[4]);
    if (c > 5) Py = atoi(v[5]);

    if (TimeSteps <= 0) TimeSteps = 100;
    if (nx <= 0) nx = 18;
    if (ny <= 0) ny = 12;
    if (Px <= 0) Px = 1;
    if (Py <= 0) Py = 1;

    game(TimeSteps, nx, ny, Px, Py);
}
