#include <math.h>
#include <mpi/mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TERMINATE_TAG -1
#define DATA_TAG 1
#define NUM_OF_RESULTS_FROM_WORKER 10

int ***readFromFileTo3DArray(FILE *fp, int, int *);
void free3DArray(int ***array, int x, int *y);
void findObjectsInPicture(int ***pictures, int *picturesLineSize, int ***objects, int numObjects, int *objectsLineSize, double matchingValue, int *found, int **positions, int picIndex);

int main(int argc, char *argv[]) {
    int rank, size;
    int *buffer = NULL;
    int numPictures, numObjects;
    float matchingValue;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        FILE *file = fopen("input.txt", "r");
        char *line = NULL, *token = NULL;
        size_t lineLength = 0;
        // Check if file exists
        if (file == NULL) {
            printf("Error: Unable to open file\n");
            return 1;
        }

        // Read matching value from first line
        if (getline(&line, &lineLength, file) == -1) {
            perror("Error reading matching value");
            return 1;
        }
        matchingValue = atof(line);
        // Read number of pictures
        if (getline(&line, &lineLength, file) == -1) {
            perror("Error reading number of pictures");
            return 1;
        }
        numPictures = atoi(line);
        int *picturesLineSize = (int *)malloc(numPictures * sizeof(int));
        int ***pictures = readFromFileTo3DArray(file, numPictures, picturesLineSize);

        if (getline(&line, &lineLength, file) == -1) {
            perror("Error reading matching value");
            return 1;
        }
        numObjects = atof(line);
        printf("Num of objects: %d\n", numObjects);
        int *objectsLineSize = (int *)malloc(numObjects * sizeof(int));
        int ***objects = readFromFileTo3DArray(file, numObjects, objectsLineSize);

        int found[numObjects];
        int *positions[numObjects];

        // Send objects to all workers.
        MPI_Bcast(&numObjects, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(objectsLineSize, numObjects, MPI_INT, 0, MPI_COMM_WORLD);
        int totalObjectsSize = 0;
        for (int i = 0; i < numObjects; i++) {
            totalObjectsSize += objectsLineSize[i] * objectsLineSize[i];
        }
        totalObjectsSize = totalObjectsSize * numObjects;
        buffer = (int *)malloc(totalObjectsSize * sizeof(int));
        int count = 0;
        for (int i = 0; i < numObjects; i++) {
            for (int j = 0; j < objectsLineSize[i]; j++) {
                for (int k = 0; k < objectsLineSize[i]; k++) {
                    buffer[count] = objects[i][j][k];
                    count++;
                }
            }
        }
        printf("total: %d\n", totalObjectsSize);
        // MPI_Bcast(buffer, totalObjectsSize, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(buffer, totalObjectsSize, MPI_INT, 0, MPI_COMM_WORLD);
        master_job(0,size,pictures,picturesLineSize,numPictures);

    } else {
        MPI_Status status;
        int rec;
        int numObjects;
        MPI_Recv(&numObjects, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        MPI_Bcast(&numObjects, 1, MPI_INT, 0, MPI_COMM_WORLD);
        int *objectsLineSize = (int *)malloc(sizeof(int) * numObjects);
        MPI_Bcast(objectsLineSize, numObjects, MPI_INT, 0, MPI_COMM_WORLD);
        int ***objects = (int ***)malloc(numObjects * sizeof(int **));
        int totalObjectsSize = 0;
        for (int i = 0; i < numObjects; i++) {
            totalObjectsSize += objectsLineSize[i] * objectsLineSize[i];
            objects[i] = (int **)malloc(objectsLineSize[i] * sizeof(int *));
            for (int j = 0; j < objectsLineSize[i]; j++) {
                objects[i][j] = (int *)malloc(objectsLineSize[i] * sizeof(int));
            }

            // printf("objects array: %d\n", objectsLineSize[i]);
        }
        totalObjectsSize = totalObjectsSize * numObjects;
        buffer = (int *)malloc(totalObjectsSize * sizeof(int));
        MPI_Bcast(buffer, totalObjectsSize, MPI_INT, 0, MPI_COMM_WORLD);
        // totalObjectsSize *= numObjects;
        // printf("total worker: %d\n", buffer[0]);
        int count = 0;
        for (int i = 0; i < numObjects; i++) {
            for (int j = 0; j < objectsLineSize[i]; j++) {
                for (int k = 0; k < objectsLineSize[i]; k++) {
                    objects[i][j][k] = buffer[count];
                    count++;
                }
            }
        }
        printf("Rank %d received:\n", rank);
        printf("Got %d my rank is: %d\n", objects[1][0][2], rank);
        // free3DArray(objects, numObjects, objectsLineSize);

        printf("Rank: %d/%d\n", rank, size);
    }

    MPI_Finalize();

    return 0;
}

// int main(int argc, char *argv[]) {
//     int rank, size;
//     MPI_Init(&argc, &argv);
//     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//     MPI_Comm_size(MPI_COMM_WORLD, &size);

//     printf("Rank: %d/%d\n", rank, size);
//     if (rank == 0) {
//         int a[5] = {1, 2, 3, 4, 5};
//         int size = 5;
//         MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

//         MPI_Bcast(a, size, MPI_INT, 0, MPI_COMM_WORLD);
//     } else {
//         int size;
//         MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);
//         int *a = malloc(sizeof(int) * size);
//         MPI_Bcast(a, size, MPI_INT, 0, MPI_COMM_WORLD);
//         printf(" result %d \n", a[4]);
//     }

//     MPI_Finalize();

//     return 0;
// }

// int main() {
//     // Open input.txt file
//     time_t start, end;
//     FILE *file = fopen("input.txt", "r");
//     int numPictures, numObjects;
//     float matchingValue;
//     char *line = NULL, *token = NULL;
//     size_t lineLength = 0;

//     // Check if file exists
//     if (file == NULL) {
//         printf("Error: Unable to open file\n");
//         return 1;
//     }

//     // Read matching value from first line
//     if (getline(&line, &lineLength, file) == -1) {
//         perror("Error reading matching value");
//         return 1;
//     }
//     matchingValue = atof(line);

//     // Read number of pictures
//     if (getline(&line, &lineLength, file) == -1) {
//         perror("Error reading number of pictures");
//         return 1;
//     }
//     numPictures = atoi(line);
//     int *picturesLineSize = (int *)malloc(numPictures * sizeof(int));
//     int ***pictures = readFromFileTo3DArray(file, numPictures, picturesLineSize);

//     // Free memory

//     if (getline(&line, &lineLength, file) == -1) {
//         perror("Error reading matching value");
//         return 1;
//     }
//     numObjects = atof(line);
//     printf("Num of objects: %d\n", numObjects);
//     int *objectsLineSize = (int *)malloc(numObjects * sizeof(int));
//     int ***objects = readFromFileTo3DArray(file, numObjects, objectsLineSize);

//     int found[numObjects];
//     int *positions[numObjects];
//     // 3d pictures , 3d objects
//     start = time(NULL);
//     for (int picIndex = 0; picIndex < numPictures; picIndex++) {
//         findObjectsInPicture(pictures, picturesLineSize, objects, numObjects, objectsLineSize, matchingValue, found, positions, picIndex);
//         printf("Picture: %d\n", picIndex + 1);
//         for (int k = 0; k < numObjects; k++) {
//             if (found[k] == 1) {
//                 printf("\tSubarray %d found", k + 1);
//                 printf("\tPosition : (%d,%d)\n", positions[k][0], positions[k][1]);
//             } else {
//                 printf("\tSubarray not found\n");
//             }
//         }
//         for (int i = 0; i < numObjects; i++) {
//             free(positions[i]);
//         }
//     }
//     end = time(NULL);
//     printf("Time: %f\n", (double)(end - start));
//     free3DArray(pictures, numPictures, picturesLineSize);
//     free3DArray(objects, numObjects, objectsLineSize);
//     // Close input.txt file
//     free(line);
//     free(picturesLineSize);
//     free(objectsLineSize);
//     fclose(file);

//     return 0;
// }
void master_job(int size, int mpi_size, int ***pictures, int *picturesLineSize, int numOfPictures) {  // master send to each worker a (x,y)
    MPI_Status status;
    int counter = 0;
    int picIndex = 0;
    for (int i = 1; i < mpi_size; i++) {
        MPI_Send(pictures[picIndex], picturesLineSize[picIndex] * picturesLineSize[picIndex], MPI_INT, i, picIndex, MPI_COMM_WORLD);
        counter++;
        picIndex++;
        if (picIndex == numOfPictures) {
            break;
        }
    }
    int res[NUM_OF_RESULTS_FROM_WORKER];
    // numobjects + 1
    /// 1 PIC  8 objects -  [true/false,0,0,0,0,0,0,0,0,0]
    do {  // wait for worker to finish job and send new job or if finished task send TERMINATE
        MPI_Recv(res, NUM_OF_RESULTS_FROM_WORKER, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        counter--;
        if (picIndex < numOfPictures) {
            MPI_Send(pictures[picIndex], picturesLineSize[picIndex] * picturesLineSize[picIndex], MPI_INT, status.MPI_SOURCE, picIndex, MPI_COMM_WORLD);
            counter++, picIndex++;
        } else {  // send terminate
            MPI_Send(NULL, 0, MPI_INT, status.MPI_SOURCE, TERMINATE_TAG, MPI_COMM_WORLD);
        }
        // write to file
    } while (counter > 0);
}

int ***readFromFileTo3DArray(FILE *file, int size, int *elementSizeToUpdate) {
    int id, elementSize;
    char *line = NULL, *token = NULL;
    size_t lineLength = 0;
    int ***array = (int ***)malloc(size * sizeof(int **));
    for (int i = 0; i < size; i++) {
        if (getline(&line, &lineLength, file) == -1) {
            perror("Error reading picture ID");
            return NULL;
        }
        id = atoi(line);

        if (getline(&line, &lineLength, file) == -1) {
            perror("Error reading picture size");
            return NULL;
        }
        elementSize = atoi(line);
        elementSizeToUpdate[i] = elementSize;

        // Allocate memory for picture matrix

        array[i] = (int **)malloc(elementSize * sizeof(int *));

        for (int j = 0; j < elementSize; j++) {
            array[i][j] = (int *)malloc(elementSize * sizeof(int));
        }

        // Read picture elements
        for (int j = 0; j < elementSize; j++) {
            // Read line of picture elements
            getline(&line, &lineLength, file);

            // Parse line and store elements in picture matrix
            token = strtok(line, " \t");
            for (int k = 0; k < elementSize; k++) {
                array[i][j][k] = atoi(token);
                token = strtok(NULL, " \t");
            }
            free(line);
            line = NULL;
        }

        // Print picture information
        printf("ID: %d\n", id);
        printf("Size: %d\n", elementSize);
        printf("Elements:\n");
        // if (elementSize == 15) {
        //     /* code */

        //     for (int j = 0; j < elementSize; j++) {
        //         for (int k = 0; k < elementSize; k++) {
        //             printf("%d ", array[i][j][k]);
        //         }
        //         printf("\n----------------------------------\n");
        //     }
        // }
    }
    return array;
}
void free3DArray(int ***array, int x, int *y) {
    for (int i = 0; i < x; i++) {
        for (int j = 0; j < y[i]; j++) {
            free(array[i][j]);
        }
        free(array[i]);
    }
    free(array);
}
void worker_job(int size, int mpi_size) {
    int rank;
    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int data[2];
    MPI_Recv(data, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    while (status.MPI_TAG == DATA_TAG) {  // wait for new job
        double answer = heavy(data[0], data[1]);
        MPI_Send(&answer, 1, MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);
        MPI_Recv(data, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    }
}
void findObjectsInPicture(int ***pictures, int *picturesLineSize, int ***objects, int numObjects, int *objectsLineSize, double matchingValue, int *found, int **positions, int picIndex) {
    for (int k = 0; k < numObjects; k++) {
        int flag = 0;
        int m = objectsLineSize[k];
        found[k] = 0;
        positions[k] = NULL;
        if (m > picturesLineSize[picIndex]) {
            continue;
        }
        for (int i = 0; i <= picturesLineSize[picIndex] - m; i++) {
            for (int j = 0; j <= picturesLineSize[picIndex] - m; j++) {
                double diff = 0.0;
                for (int x = 0; x < m; x++) {
                    for (int y = 0; y < m; y++) {
                        diff += fabs((double)(pictures[picIndex][i + x][j + y] - objects[k][x][y])) / ((double)(pictures[picIndex][i + x][j + y]));
                    }
                }
                double matching = diff / (m * m);
                if (matching <= matchingValue) {
                    found[k] = 1;
                    positions[k] = (int *)malloc(2 * sizeof(int));
                    positions[k][0] = i;
                    positions[k][1] = j;
                    flag = 1;
                    break;
                }
            }
            if (flag == 1) {
                break;
            }
        }
    }
}