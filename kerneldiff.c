#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DIFF_COMP_MAX_SIZE 20

typedef struct {
    char offset[DIFF_COMP_MAX_SIZE];
    char originalByte[DIFF_COMP_MAX_SIZE];
    char patchedByte[DIFF_COMP_MAX_SIZE];
} Difference;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: kernelcache_raw kernelcache_patched kc.bpatch\n");
        return 0;
    }

    char *patched = argv[2];
    char *original = argv[1];

    struct stat stP;
    struct stat stO;

    int ret_patched = stat(patched, &stP);
    int ret_original = stat(original, &stO);

    if (ret_patched) { perror("stat() patched"); abort(); }
    if (ret_original) { perror("stat() original"); abort(); }

    if (stP.st_size != stO.st_size) {
        printf("size does not match, can't compare files! exiting...\n");
        return 1;
    }

    FILE *fP = fopen(patched, "rb");
    FILE *fO = fopen(original, "rb");

    if (fP == NULL) { perror("fopen() patched"); abort(); }
    if (fO == NULL) { perror("fopen() original"); abort(); }

    char *p = malloc(stP.st_size);
    char *o = malloc(stO.st_size);

    if (p == NULL) { perror("malloc() patched"); abort(); }
    if (o == NULL) { perror("malloc() original"); abort(); }

    fread(p, 1, stP.st_size, fP);
    fread(o, 1, stO.st_size, fO);

    fclose(fP);
    fclose(fO);

    Difference *diff = malloc(stO.st_size * sizeof(Difference));
    int diffIndex = 0;

    for (int i = 0; i < stO.st_size; i++) {
        char originalByte = o[i];
        char patchedByte = p[i];

        if (originalByte != patchedByte) {
            snprintf(diff[diffIndex].offset, DIFF_COMP_MAX_SIZE, "0x%x", i);
            snprintf(diff[diffIndex].originalByte, DIFF_COMP_MAX_SIZE, "0x%x", originalByte);
            snprintf(diff[diffIndex].patchedByte, DIFF_COMP_MAX_SIZE, "0x%x", patchedByte);

            diffIndex++;
        }
    }

    free(p);
    free(o);

    FILE *diffFile = fopen(argv[3], "w+");
    fwrite("#AMFI\n\n", 1, 7, diffFile);

    for (int i = 0; i < diffIndex; i++) {
        int dataSize = strlen(diff[i].offset) + strlen(diff[i].originalByte) + strlen(diff[i].patchedByte) + 3;
        char data[dataSize];
        sprintf(data, "%s %s %s\n", diff[i].offset, diff[i].originalByte, diff[i].patchedByte);

        fwrite(data, 1, dataSize, diffFile);
    }

    free(diff);
    fclose(diffFile);

    return 0;
}
