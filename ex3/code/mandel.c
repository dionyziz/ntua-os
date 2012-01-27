/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "mandel-lib.h"
#include "pipesem.h"

#define MANDEL_MAX_ITERATION 100000
#define NCHILDREN 1

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
    
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
    /*
     * x and y traverse the complex plane.
     */
    double x, y;

    int n;
    int val;

    /* Find out the y value corresponding to this line */
    y = ymax - ystep * line;

    /* and iterate for all points on this line */
    for (x = xmin, n = 0; x <= xmax; x+= xstep, n++) {

        /* Compute the point's color value */
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        if (val > 255)
            val = 255;

        /* And store it in the color_val[] array */
        val = xterm_color(val);
        color_val[n] = val;
    }
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
    int i;
    
    char point ='@';
    char newline='\n';

    for (i = 0; i < x_chars; i++) {
        /* Set the current color, then output the point */
        set_xterm_color(fd, color_val[i]);
        if (write(fd, &point, 1) != 1) {
            perror("compute_and_output_mandel_line: write point");
            exit(1);
        }
    }

    /* Now that the line is done, output a newline character */
    if (write(fd, &newline, 1) != 1) {
        perror("compute_and_output_mandel_line: write newline");
        exit(1);
    }
}

void compute_and_output_mandel_line(int fd, int line)
{
    /*
     * A temporary array, used to hold color values for the line being drawn
     */
    int color_val[x_chars];

    compute_mandel_line(line, color_val);
    output_mandel_line(fd, color_val);
}

int main(void)
{
    int line;
    int buffer[x_chars];
    struct pipesem sems[NCHILDREN];
    int pipes[NCHILDREN][2];

    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    int pids[NCHILDREN];
    int i,j;
    for (i = 0; i < NCHILDREN; i++) {
        pipe(pipes[i]);
        pipesem_init(&sems[i], 0);
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Failed to fork");
            return 0;
        }
        else if (pids[i] == 0) {
            close(pipes[i][0]);

            for (j = i; j < y_chars; j += NCHILDREN) {
                compute_mandel_line(j, buffer);
                int status;
                int bytes_written = 0;
                //printf( "Process %d. Computing line %d\n", i, j );
                while (bytes_written < x_chars * sizeof(int)) {
                    status = write(pipes[i][1], (void*) buffer + bytes_written, x_chars * sizeof(int) - bytes_written);
                    if (status < 0) {
                        perror("Could not write to pipe");
                        //TODO Inform your mama
                        return 0;
                    }
                    bytes_written += status;
                }
                pipesem_signal(&sems[i]);
            }
            close(pipes[i][1]);
            return 0;
        }
        else {
            close(pipes[i][1]);
        }
    }

    /*
     * draw the Mandelbrot Set, one line at a time.
     * Output is sent to file descriptor '1', i.e., standard output.
     */
    int status;
    int bytes_read;
    for (line = 0; line < y_chars; line++) {
        bytes_read = 0;
        pipesem_wait(&sems[line % NCHILDREN]);
        while (bytes_read < x_chars * sizeof(int)) {
            //printf( "Reading %d bytes\n", x_chars * sizeof(int) - bytes_read);
            status = read(pipes[line % NCHILDREN][0], (void*) buffer + bytes_read, x_chars * sizeof(int) - bytes_read);
            if (status < 0) {
                perror("Could not read from pipe");
                return 0;
            }
            bytes_read += status;
        }
        output_mandel_line(1, buffer);
    }

    reset_xterm_color(1);
    return 0;
}
