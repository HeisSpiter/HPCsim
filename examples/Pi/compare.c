/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          PI simulation
 * FILE:             examples/Pi/compare.c
 * PURPOSE:          Compare two output files
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include "simulation.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#define PATH_MAX 0x1000

int main(int argc, char *argv[])
{
    FILE * file1, * file2;
    char id1[ID_FIELD_SIZE];
    char id2[ID_FIELD_SIZE];
    uint32_t len;
    double res1[2], res2[2];
    unsigned long total = 0, correct = 0;

    if (argc < 3)
    {
        return 0;
    }

    file1 = fopen(argv[1], "r");
    if (file1 == NULL)
    {
        fprintf(stderr, "Error while opening %s\n", argv[1]);
        return -1;
    }

    file2 = fopen(argv[2], "r");
    if (file2 == NULL)
    {
        fclose(file1);
        fprintf(stderr, "Error while opening %s\n", argv[2]);
        return -1;
    }

    do
    {
        unsigned char found = 0;

        /* Read ID from first */
        if (fread(id1, sizeof(id1), 1, file1) != 1)
        {
            break;
        }
        ++total;

        if (fseek(file2, 0, SEEK_SET) == -1)
        {
            break;
        }

        do
        {
            if (fread(id2, sizeof(id2), 1, file2) != 1)
            {
                break;
            }

            /* Compare the ID */
            if (memcmp(id1, id2, sizeof(id1)) == 0)
            {
                ++correct;
                found = 1;
            }

            /* Sanity check: verify we have correct length */
            if (fread(&len, sizeof(uint32_t), 1, file2) != 1)
            {
                break;
            }
            if (len != 2 * sizeof(double))
            {
                break;
            }

            /* Read the two doubles */
            if (fread(res2, sizeof(double), 2, file2) != 2)
            {
                break;
            }
        /* Read till we reach EOF or till we found a matching event */
        } while (!feof(file2) && found == 0);

        /* If we couldn't find the event, print */
        if (found == 0)
        {
            unsigned int i;
            char digestStr[ID_FIELD_SIZE*2+1];

            for (i = 0; i < ID_FIELD_SIZE; i++)
            {
                sprintf(&digestStr[i * 2], "%02x", (unsigned int)id1[i]);
            }

            printf("Mismatching ID: %s\n", digestStr);
        }

        /* Sanity check: verify we have correct length */
        if (fread(&len, sizeof(uint32_t), 1, file1) != 1)
        {
            break;
        }
        if (len != 2 * sizeof(double))
        {
            break;
        }

        /* Read the two doubles */
        if (fread(res1, sizeof(double), 2, file1) != 2)
        {
            break;
        }

        /* If we found matching ID, compare data */
        if (found == 1)
        {
            if (fabs(res1[0] - res2[0]) > DBL_EPSILON ||
                fabs(res1[1] - res2[1]) > DBL_EPSILON)
            {
                unsigned int i;
                char digestStr[ID_FIELD_SIZE*2+1];

                for (i = 0; i < ID_FIELD_SIZE; i++)
                {
                    sprintf(&digestStr[i * 2], "%02x", (unsigned int)id1[i]);
                }

                printf("Mismatching ID: %s\n", digestStr);
                printf("res1[0]: %f, res1[1]: %f\nres2[0]: %f, res2[1]: %f\n", res1[0], res1[1], res2[0], res2[1]);
                --correct;
            }
        }
    }
    while (!feof(file1));
    fclose(file2);
    fclose(file1);

    /* Print results */
    printf("Total results: %lu\nCorrect results: %lu\n", total, correct);

    return 0;
}
