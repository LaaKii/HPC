#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>

#define calcIndex(width, x, y)  ((y)*(width) + (x))

//long TimeSteps = 100;

void writeData(const unsigned int *data, int w, int h, FILE *fp);

void writeFooter(FILE *fp);

FILE *writeHeader(long timestep, const char *prefix, int w, int h);

//void writeVTK2(long timestep, unsigned *data, char prefix[1024], int w, int h) {
//    FILE *fp = writeHeader(timestep, prefix, w, h);
//
//    writeData(data, w, h, fp);
//
//    writeFooter(fp);
//}

FILE *writeHeader(long timestep, const char *prefix, int w, int h) {
    char filename[2048];

    int offsetX = 0;
    int offsetY = 0;
    float deltax = 1.0;
    long nxy = w * h * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
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
    return fp;
}

void writeFooter(FILE *fp) {
    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

//void writeData(const unsigned int *data, int w, int h, FILE *fp) {
//    int x, y;
//    for (y = 0; y < h; y++) {
//        for (x = 0; x < w; x++) {
//            float value = data[calcIndex(h, x, y)];
//            fwrite((unsigned char *) &value, sizeof(float), 1, fp);
//        }
//    }
//}


void show(unsigned *currentfield, int w, int h) {
    printf("\033[H");
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x, y)] ? "\033[07m  \033[m" : "  ");
        printf("\033[E");
    }
    fflush(stdout);
}

int countLifingsPeriodic(unsigned *currentfield, int x, int y, int w, int h) {
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


void evolve(unsigned *currentfield, unsigned *newfield, int w, int h, int num_threads, long timestep) {

    int x, y;
    FILE *fp = writeHeader(timestep, "gol", w, h);

#pragma omp parallel for collapse(2) num_threads(num_threads)
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
//            printf("thread %d\n", omp_get_thread_num());
            int n = countLifingsPeriodic(currentfield, x, y, w, h);
            if (currentfield[calcIndex(w, x, y)]) n--;
            newfield[calcIndex(w, x, y)] = (n == 3 || (n == 2 && currentfield[calcIndex(w, x, y)]));

            float value = currentfield[calcIndex(w, x, y)];
            fwrite((unsigned char *) &value, sizeof(float), 1, fp);
        }
    }
    writeFooter(fp);
}

void filling(unsigned *currentfield, int w, int h) {
    for (int i = 0; i < h * w; i++) {
        currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
    }
}

void game(int TimeSteps, int w, int h, int num_threads) {
    unsigned *currentfield = calloc(w * h, sizeof(unsigned));
    unsigned *newfield = calloc(w * h, sizeof(unsigned));

    //printf("size unsigned %d, size long %d\n",sizeof(float), sizeof(long));

    filling(currentfield, w, h);
    long timestep;
    for (timestep = 0; timestep < TimeSteps; timestep++) {
        show(currentfield, w, h);
        evolve(currentfield, newfield, w, h, num_threads, timestep);

        printf("%ld timestep\n", timestep);
//        writeVTK2(timestep, currentfield, "gol", w, h);

//        FILE *fp = writeHeader(timestep, "gol", w, h);
//        writeData(currentfield, w, h, fp);
//        writeFooter(fp);

        usleep(2000000);

        //SWAP
        unsigned *temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

    free(currentfield);
    free(newfield);

}

int main(int c, char **v) {
    int width = 0, height = 0, TimeSteps = 0;
    int nx, ny;
    int Px, Py;
    int num_threads;

    if (c > 1) TimeSteps = atoi(v[1]);
    if (c > 2) nx = atoi(v[2]); ///< read width
    if (c > 3) ny = atoi(v[3]); ///< read height
    if (c > 4) Px = atoi(v[4]);
    if (c > 5) Py = atoi(v[5]);

    width = nx * Px;
    height = ny * Py;
    num_threads = Px * Py;

//    if (width <= 0) width = 30; ///< default width
//    if (height <= 0) height = 30; ///< default height

    game(TimeSteps, width, height, num_threads);
}
