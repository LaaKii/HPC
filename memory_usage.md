## memory usage
Used variables:
```
int width
int height
int num_threads
int TimeSteps
long time_step
double *temp = currentfield
double *currentfield = calloc(width * height, sizeof(double))
double *newfield = calloc(width * height, sizeof(double))
```

in omp parallel:
```
int i
int x_start
int x_end
int y_start
int y_end
int y
int x
int n
int index
```

Variable sizes (bytes):
```
int_size = 4
double_size = 8
float_size = 4
long_size = 8
```

max memory usage:
```
usage = (4 * int_size) + (1 * long_size) + (2 * width * height * double_size) 
+ num_threads * (int_size * 9)
```

## VTI disk usage for one time step

```
main_file = 334 * 1 + 58 * #fields
     = 334 * 1 + 58 * (nx * ny)

sub_files = 397 * #files + float * #pixel
    = 397 * (nx * ny) + 4 * (nx * ny * Px * Py)

sum_disk_usage = 334 * 1 + 58 * (nx * ny) + 397 * (nx * ny) + 4 * (nx * ny * Px * Py)
```