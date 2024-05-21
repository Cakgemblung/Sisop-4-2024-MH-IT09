#define FUSE_USE_VERSION 35

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

// Path to the source directory
static const char *dirpath = "/path/to/soal_2/sensitif/pesan";

// Function to decode base64
char* decode_base64(const char* input) {
    size_t len = strlen(input);
    size_t output_len = len * 3 / 4;
    char *output = malloc(output_len + 1);
    if (!output) return NULL;

    // Simplified base64 decoding for example purposes
    // You should replace this with a robust base64 decoding function
    for (size_t i = 0, j = 0; i < len;) {
        uint32_t sextet_a = input[i] == '=' ? 0 & i++ : input[i++] - 'A';
        uint32_t sextet_b = input[i] == '=' ? 0 & i++ : input[i++] - 'A';
        uint32_t sextet_c = input[i] == '=' ? 0 & i++ : input[i++] - 'A';
        uint32_t sextet_d = input[i] == '=' ? 0 & i++ : input[i++] - 'A';

        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        if (j < output_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < output_len) output[j++] = (triple >> 8) & 0xFF;
        if (j < output_len) output[j++] = triple & 0xFF;
    }

    output[output_len] = '\0';
    return output;
}

// Function to decode ROT13
void decode_rot13(char* input) {
    for (int i = 0; input[i] != '\0'; i++) {
        if ((input[i] >= 'a' && input[i] <= 'z') || (input[i] >= 'A' && input[i] <= 'Z')) {
            if ((input[i] >= 'a' && input[i] <= 'm') || (input[i] >= 'A' && input[i] <= 'M')) {
                input[i] += 13;
            } else {
                input[i] -= 13;
            }
        }
    }
}

// Function to decode hexadecimal
void decode_hex(const char* input, char* output) {
    size_t len = strlen(input);
    for (size_t i = 0; i < len; i += 2) {
        sscanf(input + i, "%2hhx", &output[i / 2]);
    }
    output[len / 2] = '\0';
}

// Function to reverse a string
void decode_reverse(char* input) {
    int len = strlen(input);
    for (int i = 0; i < len / 2; i++) {
        char temp = input[i];
        input[i] = input[len - i - 1];
        input[len - i - 1] = temp;
    }
}

// FUSE operations

static int xmp_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi; // Unused parameter warning
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    DIR *dp;
    struct dirent *de;
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);

    (void) offset;
    (void) fi;
    (void) flags;

    dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0)) // Corrected filler call
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);

    res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res); // Use close to close the file descriptor
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);

    fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);

    // Decode based on prefix
    if (strncmp(path, "/base64_", 8) == 0) {
        char *decoded = decode_base64(buf);
        if (decoded) {
            strncpy(buf, decoded, size);
            free(decoded);
        }
    } else if (strncmp(path, "/rot13_", 7) == 0) {
        decode_rot13(buf);
    } else if (strncmp(path, "/hex_", 5) == 0) {
        char *decoded = malloc(size / 2 + 1);
        if (decoded) {
            decode_hex(buf, decoded);
            strncpy(buf, decoded, size / 2);
            free(decoded);
        }
    } else if (strncmp(path, "/rev_", 5) == 0) {
        decode_reverse(buf);
    }

    return res;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
