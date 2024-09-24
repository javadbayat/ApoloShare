#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "cJSON.h"

typedef char *PSTR;

#define BUFFER_SIZE 4096

int relaunchWithTAPI(PSTR executablePath, PSTR filePath) {
    FILE *fd = popen("termux-usb -l", "r");
    char buffer[260];
    char *jsonData = fgets(buffer, 260, fd);
    cJSON *devicesList = cJSON_Parse(jsonData);
    pclose(fd);
    if (!cJSON_IsString(devicesList)) {
        cJSON_Delete(devicesList);
        return 1;
    }
    
    memset(buffer, 0, 260);
    sprintf(buffer, "termux-usb -r \"%s\"", jsonData->valuestring);
    system(buffer);
    
    memset(buffer, 0, 260);
    sprintf(buffer, "termux-usb -e \"%s\" \"%s\" \"%s\"", executablePath, jsonData->valuestring, filePath);
    system(buffer);
    
    cJSON_Delete(devicesList);
    return 0;
}

void trackProgress(long portion) {
    const PSTR sizeTags[] = { "GB", "MB", "KB", "Bytes" };
    int multiplier = 1024 * 1024 * 1024;
    unsigned char i;
    
    system("clear");
    
    for (i = 0; i < 4; i++)
    {
        if (portion >= multiplier)
            break;

        multiplier /= 1024;
    }

    printf("Transferred %.2f%s so far...\n", (float) portion / multiplier, sizeTags[i]);
}

int main(int argc, char **argv) {
    PSTR &filePath = argv[2];
    int computerFD;
    int r;
    int &temp = r;
    libusb_context *ctx = NULL;
    libusb_device_handle *hDevice = NULL;
    FILE *fd = NULL;
    int actualLength = 0;
    long portion = 0;
    PSTR pBuffer = NULL;
    
    if (argc == 2)
        return relaunchWithTAPI(argv[0], argv[1]);
    
    if (argc != 3)
        return 1;
    
    printf("Initializing...\n");
    
    computerFD = atoi(argv[1]);
    if (!computerFD) {
        fprintf(stderr, "Invalid computer file descriptor '%s'\n", argv[1]);
        return 1;
    }
    
    r = libusb_init(&ctx);
    if (r < 0) {
        fprintf(stderr, "Failed to initialize libusb!\nError code: %d\n", r);
        return 2;
    }
    
    printf("Connecting to your computer...\n");
    
    r = libusb_wrap_sys_device(ctx, computerFD, &hDevice);
    if (r) {
        fprintf(stderr, "Failed to connect to your computer!\nError code: %d\n", r);
        libusb_exit(ctx);
        return 3;
    }
    
    printf("Preparing to send your file...\n");
    
    pBuffer = malloc(BUFFER_SIZE);
    if (!pBuffer) {
        fprintf(stderr, "Not enough memory!\n");
        libusb_close(hDevice);
        libusb_exit(ctx);
        return 3;
    }
    
    fd = fopen(filePath, "wb");
    if (!fd) {
        fprintf(stderr, "Failed to open your file!\n");
        free(pBuffer);
        libusb_close(hDevice);
        libusb_exit(ctx);
        return 3;
    }
    
    while (TRUE) {
        actualLength = fread(pBuffer, 1, BUFFER_SIZE, fd);
        if (!actualLength) {
            printf("File Transfer was successfully completed!\n");
            break;
        }
        
        if (libusb_bulk_transfer(hDevice, (1 | LIBUSB_ENDPOINT_OUT), (unsigned char *) pBuffer, actualLength, &temp, 0)) {
            fprintf(stderr, "Error in bulk transfer!\n");
            break;
        }
        
        portion += (long) actualLength;
        trackProgress(portion);
    }
    
    fclose(fd);
    free(pBuffer);
    libusb_close(hDevice);
    libusb_exit(ctx);
    return 0;
}