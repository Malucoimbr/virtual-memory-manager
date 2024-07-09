#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define INITIAL_SIZE 1000

int *pageNums; 
int *offsets;
int *pageTable;
int *lAddresses;
char **frames;
int **tlb;
int *pageFrames;
int *usageTimes; 
int currentTime = 0; 

int findLRUFrame();

int readFile(char *file, int *addressCount) {
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }
    char *line = NULL;
    size_t len = 0;
    int index = 0;
    int capacity = INITIAL_SIZE;

    lAddresses = malloc(sizeof(int) * capacity);
    pageNums = malloc(sizeof(int) * capacity);
    offsets = malloc(sizeof(int) * capacity);

    if (!lAddresses || !pageNums || !offsets) {
        exit(EXIT_FAILURE);
    }

    while (getline(&line, &len, fp) != -1) {
        // Remove newline character if present
        char *pos;
        if ((pos = strchr(line, '\n')) != NULL) {
            *pos = '\0';
        }
        if ((pos = strchr(line, '\r')) != NULL) {
            *pos = '\0';
        }

        // Check if the line is not empty
        if (strlen(line) > 0) {
            if (index >= capacity) {
                capacity *= 2;
                lAddresses = realloc(lAddresses, sizeof(int) * capacity);
                pageNums = realloc(pageNums, sizeof(int) * capacity);
                offsets = realloc(offsets, sizeof(int) * capacity);
                if (!lAddresses || !pageNums || !offsets) {
                    exit(EXIT_FAILURE);
                }
            }
            int address = atoi(line);
            offsets[index] = address & 0xFF; // Lower 8 bits for offset
            pageNums[index] = (address >> 8) & 0xFF; // Next 8 bits for page number
            lAddresses[index] = address;
            index++;
        }
    }
    fclose(fp);
    free(line);
    *addressCount = index;
    return 0;
}

char *getPage(int pageNum, int offset, char *bytes) {
    FILE *fp = fopen("BACKING_STORE.bin", "rb");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }
    fseek(fp, (pageNum * 256), SEEK_SET);
    fread(bytes, 1, 256, fp);
    fclose(fp);
    return bytes;
}

int checkTLB(int pageNum) {
    for (int i = 0; i < 16; i++) {
        if (pageNum == tlb[i][0]) {
            return i; // Return TLB index instead of frame number
        }
    }
    return -1;
}

void updateTLB(int pageNum, int frameNum, int index) {
    index = index % 16;
    tlb[index][0] = pageNum;
    tlb[index][1] = frameNum;
}

int logicalToPhysical(int index) {
    int frameNum = pageTable[pageNums[index]];
    int offset = offsets[index];
    return (frameNum << 8) | offset; // Shift frame number by 8 bits and add offset
}

void initializeArrays() {
    pageTable = malloc(sizeof(int) * 256);
    if (!pageTable) {
        exit(EXIT_FAILURE);
    }
    for (int j = 0; j < 256; j++) {
        pageTable[j] = -1;
    }

    tlb = malloc(sizeof(int*) * 16);
    if (!tlb) {
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 16; i++) {
        tlb[i] = malloc(sizeof(int) * 2);
        if (!tlb[i]) {
            exit(EXIT_FAILURE);
        }
        tlb[i][0] = -1;
        tlb[i][1] = -1;
    }

    frames = malloc(sizeof(char*) * 128);
    if (!frames) {
        exit(EXIT_FAILURE);
    }
    for (int k = 0; k < 128; k++) {
        frames[k] = malloc(sizeof(char) * 256);
        if (!frames[k]) {
            exit(EXIT_FAILURE);
        }
    }
    pageFrames = malloc(sizeof(int) * 128);
    if (!pageFrames) {
        exit(EXIT_FAILURE);
    }
    for (int l = 0; l < 128; l++) {
        pageFrames[l] = -1;
    }

    usageTimes = malloc(sizeof(int) * 128);
    if (!usageTimes) {
        exit(EXIT_FAILURE);
    }
    for (int m = 0; m < 128; m++) {
        usageTimes[m] = 0;
    }
}

void fifo(char *fName) {
    FILE *output_file = fopen("correct.txt", "w");

    int addressCount;
    initializeArrays();
    readFile(fName, &addressCount);

    int physicalAddress;
    readFile(fName, &addressCount);
    int pageTableHits = 0;
    int tlbHits = 0;
    int pageFaults = 0;
    int totalAddresses = 0;

    int frameNum = 0;
    int tlbIndex = 0;
    for (int i = 0; i < addressCount; i++) {
        int pageNum = pageNums[i];
        int offset = offsets[i];
        int signedByte;

        frameNum = frameNum % 128;
        int tlbCheck = checkTLB(pageNum);
        if (tlbCheck != -1) {
            int frameNumInTLB = tlb[tlbCheck][1];
            physicalAddress = logicalToPhysical(i);
            signedByte = (int) frames[frameNumInTLB][offset];
            tlbHits++;
            fprintf(output_file,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", lAddresses[i], tlbCheck, physicalAddress, signedByte);
        } else if (pageTable[pageNum] == -1) {
            pageTable[pageNum] = frameNum;
            physicalAddress = logicalToPhysical(i);
            char *f = malloc(256 * sizeof(char));
            if (f == NULL) {
                exit(EXIT_FAILURE);
            }
            f = getPage(pageNum, offset, f);
            if (pageFrames[frameNum] != -1) {
                int oldPageNum = pageFrames[frameNum];
                pageTable[oldPageNum] = -1;
            }
            pageFrames[frameNum] = pageNum;
            frames[frameNum] = f;
            signedByte = (int) frames[frameNum][offset];
            updateTLB(pageNum, frameNum, tlbIndex);
            fprintf(output_file,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", lAddresses[i], tlbIndex % 16, physicalAddress, signedByte);
            tlbIndex++;
            frameNum++;
            pageFaults++;
        } else {
            physicalAddress = logicalToPhysical(i);
            int fNum = pageTable[pageNum];
            signedByte = (int) frames[fNum][offset];
            updateTLB(pageNum, fNum, tlbIndex);
            fprintf(output_file,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", lAddresses[i], tlbIndex % 16, physicalAddress, signedByte);
            tlbIndex++;
            pageTableHits++;
        }
        totalAddresses++;
    }
    fprintf(output_file,"Number of Translated Addresses = %d\n", totalAddresses);
    fprintf(output_file,"Page Faults = %d\n", pageFaults);
    fprintf(output_file,"Page Fault Rate = %.3f\n", (double) pageFaults / totalAddresses);
    fprintf(output_file,"TLB Hits = %d\n", tlbHits);
    fprintf(output_file,"TLB Hit Rate = %.3f\n", (double) tlbHits / totalAddresses);
}

void lru(char *fName) {
    FILE *output_file = fopen("correct.txt", "w");

    int addressCount;
    initializeArrays();
    readFile(fName, &addressCount);

    int physicalAddress;
    int pageTableHits = 0;
    int tlbHits = 0;
    int pageFaults = 0;
    int totalAddresses = 0;

    int frameNum = 0;
    int tlbIndex = 0;
    for (int i = 0; i < addressCount; i++) {
        currentTime++; // Increment the global time
        int pageNum = pageNums[i];
        int offset = offsets[i];
        int signedByte;

        int tlbCheck = checkTLB(pageNum);
        if (tlbCheck != -1) {
            int frameNumInTLB = tlb[tlbCheck][1];
            physicalAddress = logicalToPhysical(i);
            signedByte = (int) frames[frameNumInTLB][offset];
            tlbHits++;
            usageTimes[frameNumInTLB] = currentTime; // Update usage time for LRU
            fprintf(output_file,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", lAddresses[i], tlbCheck, physicalAddress, signedByte);
        } else if (pageTable[pageNum] == -1) {
            if (frameNum < 128) {
                pageTable[pageNum] = frameNum;
                physicalAddress = logicalToPhysical(i);
                char *f = malloc(256 * sizeof(char));
                if (f == NULL) {
                    exit(EXIT_FAILURE);
                }
                f = getPage(pageNum, offset, f);
                if (pageFrames[frameNum] != -1) {
                    int oldPageNum = pageFrames[frameNum];
                    pageTable[oldPageNum] = -1;
                }
                pageFrames[frameNum] = pageNum;
                frames[frameNum] = f;
                signedByte = (int) frames[frameNum][offset];
                updateTLB(pageNum, frameNum, tlbIndex);
                usageTimes[frameNum] = currentTime;
                fprintf(output_file,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", lAddresses[i], tlbIndex % 16, physicalAddress, signedByte);
                tlbIndex++;
                frameNum++;
                pageFaults++;
            } else {
                int lruFrame = findLRUFrame();
                pageTable[pageNum] = lruFrame;
                physicalAddress = logicalToPhysical(i);
                char *f = malloc(256 * sizeof(char));
                if (f == NULL) {
                    exit(EXIT_FAILURE);
                }
                f = getPage(pageNum, offset, f);
                if (pageFrames[lruFrame] != -1) {
                    int oldPageNum = pageFrames[lruFrame];
                    pageTable[oldPageNum] = -1;
                }
                pageFrames[lruFrame] = pageNum;
                frames[lruFrame] = f;
                signedByte = (int) frames[lruFrame][offset];
                updateTLB(pageNum, lruFrame, tlbIndex);
                usageTimes[lruFrame] = currentTime;
                fprintf(output_file,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", lAddresses[i], tlbIndex % 16, physicalAddress, signedByte);
                tlbIndex++;
                pageFaults++;
            }
        } else {
            physicalAddress = logicalToPhysical(i);
            int fNum = pageTable[pageNum];
            signedByte = (int) frames[fNum][offset];
            updateTLB(pageNum, fNum, tlbIndex);
            usageTimes[fNum] = currentTime; // Update usage time for LRU
            fprintf(output_file,"Virtual address: %d TLB: %d Physical address: %d Value: %d\n", lAddresses[i], tlbIndex % 16, physicalAddress, signedByte);
            tlbIndex++;
            pageTableHits++;
        }

        totalAddresses++;
    }
    fprintf(output_file,"Number of Translated Addresses = %d\n", totalAddresses);
    fprintf(output_file,"Page Faults = %d\n", pageFaults);
    fprintf(output_file,"Page Fault Rate = %.3f\n", (double) pageFaults / totalAddresses);
    fprintf(output_file,"TLB Hits = %d\n", tlbHits);
    fprintf(output_file,"TLB Hit Rate = %.3f\n", (double) tlbHits / totalAddresses);
}

int findLRUFrame() {
    int lruFrame = 0;
    for (int j = 1; j < 128; j++) {
        if (usageTimes[j] < usageTimes[lruFrame]) {
            lruFrame = j;
        }
    }
    return lruFrame;
}

void cleanup() {
    free(lAddresses);
    free(pageNums);
    free(offsets);
    free(pageTable);
    for (int i = 0; i < 16; i++) {
        free(tlb[i]);
    }
    free(tlb);
    for (int i = 0; i < 128; i++) {
        free(frames[i]);
    }
    free(frames);
    free(pageFrames);
    free(usageTimes);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <address_file> <replacement_algorithm>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[2], "fifo") == 0) {
        fifo(argv[1]);
    } else if (strcmp(argv[2], "lru") == 0) {
        lru(argv[1]);
    } else {
        printf("Unsupported replacement algorithm: %s\n", argv[2]);
        return 1;
    }

    cleanup();
    return 0;
}
