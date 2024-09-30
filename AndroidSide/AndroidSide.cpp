#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "cJSON.h"

typedef char *PSTR;

#define BUFFER_SIZE (1024 * 1024)
#define USB_EP (1 | LIBUSB_ENDPOINT_OUT)
#define ZeroBuffer(buff) memset(buff, 0, sizeof(buff))

#define Error(message) fprintf(stderr, message)
#define ErrorC(message, code) fprintf(stderr, message, code)

int relaunchWithTAPI(PSTR executablePath, PSTR filePath) {
    FILE *fd = popen("termux-usb -l", "r");
    char buffer[260];
    PSTR jsonData = fgets(buffer, 260, fd);
    cJSON *devicesList = cJSON_Parse(jsonData);
    PSTR &devicePath = devicesList->valuestring;
    pclose(fd);
    if (!cJSON_IsString(devicesList)) {
        Error("No USB device found!\n");
        cJSON_Delete(devicesList);
        return 1;
    }
    
    ZeroBuffer(buffer);
    sprintf(buffer, "termux-usb -r \"%s\"", devicePath);
    system(buffer);
    
    ZeroBuffer(buffer);
    sprintf(buffer, "termux-usb -e \"%s\" \"%s\" \"%s\"", executablePath, devicePath, filePath);
    system(buffer);
    
    cJSON_Delete(devicesList);
    return 0;
}

void trackProgress(long portion) {
    PSTR sizeTags[] = { "GB", "MB", "KB", "Bytes" };
    int multiplier = 1024 * 1024 * 1024;
    unsigned char i;
    
    for (i = 0; i < 4; i++)
    {
        if (portion >= multiplier)
            break;

        multiplier /= 1024;
    }

    printf("\rTransferred %.2f%s so far...", (float) portion / multiplier, sizeTags[i]);
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
        ErrorC("Invalid computer file descriptor '%s'\n", argv[1]);
        return 1;
    }
    
    r = libusb_init(&ctx);
    if (r) {
        ErrorC("Failed to initialize libusb!\nError code: %d\n", r);
        return 2;
    }
    
    printf("Connecting to your computer...\n");
    
    r = libusb_wrap_sys_device(ctx, computerFD, &hDevice);
    if (r) {
        ErrorC("Failed to connect to your computer!\nError code: %d\n", r);
        libusb_exit(ctx);
        return 3;
    }
    
    printf("Preparing to send your file...\n");

    r = libusb_claim_interface(hDevice, 0);
    if (r) {
        ErrorC("Failed to claim the USB interface!\nError code: %d\n", r);
        libusb_close(hDevice);
        libusb_exit(ctx);
        return 4;
    }
    
    pBuffer = (PSTR) malloc(BUFFER_SIZE);
    if (!pBuffer) {
        Error("Not enough memory!\n");
        libusb_release_interface(hDevice, 0);
        libusb_close(hDevice);
        libusb_exit(ctx);
        return 5;
    }
    
    fd = fopen(filePath, "wb");
    if (!fd) {
        Error("Failed to open your file!\n");
        free(pBuffer);
        libusb_release_interface(hDevice, 0);
        libusb_close(hDevice);
        libusb_exit(ctx);
        return 6;
    }
    
    while (1) {
        actualLength = fread(pBuffer, 1, BUFFER_SIZE, fd);
        if (!actualLength) {
            printf("File Transfer was successfully completed!\n");
            break;
        }
        
        if (libusb_bulk_transfer(hDevice, USB_EP, (unsigned char *) pBuffer, actualLength, &temp, 0)) {
            Error("Error in bulk transfer!\n");
            break;
        }
        
        portion += (long) actualLength;
        trackProgress(portion);
    }
    
    fclose(fd);
    free(pBuffer);
    libusb_release_interface(hDevice, 0);
    libusb_close(hDevice);
    libusb_exit(ctx);
    return 0;
}
