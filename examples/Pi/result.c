/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          PI simulation
 * FILE:             examples/Pi/result.c
 * PURPOSE:          Read results and print Pi
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include "simulation.h"
#include <string.h>
#include <stdio.h>    

#define PATH_MAX 0x1000

#define DEFAULT_NAME {'H', 'P', 'C', 's', 'i', 'm', '.', 'o', 'u', 't', '\0'}

int main(int argc, char *argv[])
{
    FILE * inFD;
    char resFile[PATH_MAX] = DEFAULT_NAME;
    double inside = 0.0, total = 0.0;
    unsigned short len;
    double res[2];

    if (argc > 1)
    {
        strncpy(resFile, argv[1], PATH_MAX - 1);
        resFile[PATH_MAX - 1] = '\0';
    }

    inFD = fopen(resFile, "r");
    if (inFD == NULL)
    {
        fprintf(stderr, "Error while opening %s\n", resFile);
        return -1;
    }

    do
    {
        /* Skip ID */
        if (fseek(inFD, ID_FIELD_SIZE, SEEK_CUR) == -1)
        {
            break;
        }

        /* Sanity check: verify we have correct length */
        if (fread(&len, sizeof(uint32_t), 1, inFD) != 1)
        {
            break;
        }
        if (len != 2 * sizeof(double))
        {
            break;
        }

        /* Read the two doubles */
        if (fread(res, sizeof(double), 2, inFD) != 2)
        {
            break;
        }

        /* Adjust counts */
        total += res[0];
        inside += res[1];
    }
    while (!feof(inFD));
    fclose(inFD);

    /* Compute PI for real */
    printf("Pi: %lf (with %lf samples)\n", (4.0 * inside) / total, total);

    return 0;
}
