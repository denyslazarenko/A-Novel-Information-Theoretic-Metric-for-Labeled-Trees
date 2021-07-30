#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char iface[] = "eth0";
const char allowedHostname[] = "abcdefghijklmnopqrstuvwxyz1234567890-";
const char allowedImage[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXZYabcdefghijklmnopqrstuvwxyz"
        "1234567890_.~!*''();:@&=+$,/?#[%-]+";
const char allowedUUID[] = "ABCDEFabcdef1234567890-";

int checkFile(const char *filename) {
    FILE *file;
    file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 0;
    }
    return 1;
}

int getMacAddress(char *macStr) {
    char mac[] = {0, 0, 0, 0, 0, 0};
    char path[100];
    strcpy(path, "/sys/class/net/");
    strcat(path, iface);
    strcat(path, "/address");
    FILE *addressFile = fopen(path, "r");
    if (addressFile) {
        if (fgets(macStr, 18, addressFile)) {
            sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                    &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        } else {
            fclose(addressFile);
            return 1;
        }
        fclose(addressFile);
    } else {
        return 2;
    }
    return 0;
}

int getServerAddress(char *addr) {
    FILE *resolv = fopen("/etc/resolv.conf", "r");
    char *line = NULL;
    size_t len = 0;
    int f = 1;
    while (getline(&line, &len, resolv) != -1) {
        if (strncmp(line, "nameserver ", 11) == 0) {
            strncpy(addr, line + 11, strlen(line) - 12);
            addr[strlen(line) - 12] = '\0';
            f = 0;
            break;
        }
    }
    if (line)
        free(line);
    fclose(resolv);
    return f;
}

int getConfigs(const char *macStr, char *hostname, char *image, char *uuid) {
    FILE *configs = fopen("config.yml", "r");
    char *line = NULL;
    size_t len = 0;

    int found = 0;
    while (getline(&line, &len, configs) != -1) {
        if (line[0] == '-') {
            found = 0;
            if (strstr(line, macStr)) {
                found = 1;

            }
        } else if (found) {
            if (strncmp(line, "  hostname: ", 12) == 0) {
                strncpy(hostname, line + 12, strlen(line) - 13);
                hostname[strlen(line) - 13] = '\0';
            } else if (strncmp(line, "  image: ", 9) == 0) {
                strncpy(image, line + 9, strlen(line) - 10);
                image[strlen(line) - 10] = '\0';
            } else if (strncmp(line, "  uuid: ", 8) == 0) {
                strncpy(uuid, line + 8, 36);
                uuid[36] = '\0';
            }
        }
    }
    if (line)
        free(line);
    fclose(configs);
    return ((strspn(hostname, allowedHostname) == strlen(hostname)) &&
            (strspn(image, allowedImage) == strlen(image)) &&
            (strspn(uuid, allowedUUID) && strlen(uuid))) ? 0 : 1;
}

int downloadImage(const char *image) {
    char cmd[200];
    system("umount /dev/mmcblk0p1 ; umount /dev/mmcblk0p2");
    strcpy(cmd, "wget -O /dev/mmcblk0 ");
    strcat(cmd, image);
    // This blocks until the image is copied
    int res = system(cmd);
    system("mount /dev/mmcblk0p1 /media/mmcblk0p1 ; "
           "mount /dev/mmcblk0p2 /media/mmcblk0p2");
    return res || checkFile("/media/mmcblk0p1/boot/grub/grub.conf.backup");
}

int downloadConfig(const char *addr) {
    char cmd[200];
    strcpy(cmd, "wget -O config.yml http://");
    strcat(cmd, addr);
    strcat(cmd, "/img/config.yml");
    int res = system(cmd);
    return res || checkFile("config.yml");
}

int replaceHostname(const char *hostname) {
    char cmd[200];
    strcpy(cmd, "echo '");
    strcat(cmd, hostname);
    strcat(cmd, "' > /media/mmcblk0p2/etc/hostname");
    int res = system(cmd);
    return res;
}

int ifup() {
    char cmd[100];
    strcpy(cmd, "ifdown ");
    strcat(cmd, iface);
    strcat(cmd, "; ifup ");
    strcat(cmd, iface);
    int res = system(cmd);
    return res;
}

int checkFlag(const char *uuid) {
    char path[200];
    strcpy(path, "/media/mmcblk0p1/");
    strcat(path, uuid);
    int res = checkFile(path);
    if (res == 0) {
        system("cp /media/mmcblk0p1/boot/grub/grub.conf.backup "
               "/media/mmcblk0p1/boot/grub/grub.conf");
    }
    return res;
}

int writeFlag(const char *uuid) {
    system("cp /media/mmcblk0p1/boot/grub/grub.conf.backup "
           "/media/mmcblk0p1/boot/grub/grub.conf");
    char cmd[200];
    strcpy(cmd, "touch /media/mmcblk0p1/");
    strcat(cmd, uuid);
    return system(cmd);
}

int loop() {
    while (1) {
        int res = checkFile("/dev/mmcblk0");
        if (res) {
            printf("Could not find SD card: %d\n", res);
            continue;
        }
        res = ifup();
        if (res) {
            printf("Could not start the interface: %d\n", res);
            continue;
        }
        char ipAddress[16];
        res = getServerAddress(ipAddress);
        if (res) {
            printf("Could not get the server IP address: %d\n", res);
            continue;
        }
        printf("Server IP address:%s\n", ipAddress);
        res = downloadConfig(ipAddress);
        if (res) {
            printf("Could not download config.yml: %d\n", res);
            continue;
        }
        char macStr[18];
        res = getMacAddress(macStr);
        if (res) {
            printf("Could not get MAC address: %d\n", res);
            continue;
        }
        printf("MAC: %s\n", macStr);
        char hostname[100];
        char image[200];
        char uuid[100];
        res = getConfigs(macStr, hostname, image, uuid);
        if (res) {
            printf("Could not parse configs: %d\n", res);
            continue;
        }
        printf("Hostname: %s\nImage: %s\nUUID: %s\n", hostname, image, uuid);
        int flag = checkFlag(uuid);
        if (flag == 0) {
            printf("Will not download the image\n");
        } else {
            printf("Will try to download the image\n");
            res = downloadImage(image);
            if (res) {
                printf("Could not download the image: %d\n", res);
                continue;
            }
            res = writeFlag(uuid);
            if (res) {
                printf("Could not set reboot flag, will ignore: %d\n", res);
            }
        }
        res = replaceHostname(hostname);
        if (res) {
            printf("Could not set hostname, will ignore: %d\n", res);
        }
        system("reboot");
        break;
    }
    return 0;
}

int main() {
    loop();
    return 0;
}
