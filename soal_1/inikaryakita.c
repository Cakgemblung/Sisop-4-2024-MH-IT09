#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <libgen.h>


static const char *dirpath = "/home/azrael/sisop/modul4/soal1/portofolio";
static const char *watermarked_prefix = "wm";
static const char *test_prefix = "test";

// Fungsi untuk menambahkan watermark pada gambar dan mengganti namanya
void add_watermark_and_rename(const char *src, const char *dest) {
    char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "%s.temp", dest);

    char command[2048];
    snprintf(command, sizeof(command), "convert '%s' -gravity South -pointsize 36 -fill white -annotate +0+100 'inikaryakita.id' '%s'", src, temp_path);
    system(command);

    rename(temp_path, dest);
}

void reverse_content(const char *path) {
    int fd = open(path, O_RDWR);
    if (fd == -1) return; // Gagal membuka file

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return; // Gagal mendapatkan informasi file
    }

    off_t size = st.st_size;
    char *buffer = malloc(size);
    if (buffer == NULL) {
        close(fd);
        return; // Gagal mengalokasikan memori
    }

    if (read(fd, buffer, size) != size) {
        free(buffer);
        close(fd);
        return; // Gagal membaca file
    }

    // Membalikkan isi buffer
    for (off_t i = 0; i < size / 2; i++) {
        char temp = buffer[i];
        buffer[i] = buffer[size - i - 1];
        buffer[size - i - 1] = temp;
    }

    // Menulis isi yang telah dibalikkan kembali ke file
    if (lseek(fd, 0, SEEK_SET) == -1 || write(fd, buffer, size) != size) {
        free(buffer);
        close(fd);
        return; // Gagal menulis file
    }

    free(buffer);
    close(fd);
}

// Fungsi untuk mendapatkan atribut file
static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = lstat(full_path, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}

// Fungsi untuk membaca direktori
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    dp = opendir(full_path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

// Fungsi untuk membuka file
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = open(full_path, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

// Fungsi untuk membaca file
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    fd = open(full_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    } else if (strncmp(basename(full_path), test_prefix, strlen(test_prefix)) == 0) {
        // Jika file memiliki prefix "test", balikkan isi buffer sebelum mengembalikannya
        for (size_t i = 0; i < res / 2; i++) {
            char temp = buf[i];
            buf[i] = buf[res - i - 1];
            buf[res - i - 1] = temp;
        }
    }
    close(fd);
    return res;
}

// Fungsi untuk menulis ke file
static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);

    // Mengecek apakah file memiliki prefix "test"
    if (strncmp(basename(full_path), test_prefix, strlen(test_prefix)) == 0) {
        // Membalikkan isi buffer sebelum menulis
        char *reverse_buf = malloc(size);
        if (!reverse_buf) {
            return -ENOMEM;
        }

        for (size_t i = 0; i < size; i++) {
            reverse_buf[i] = buf[size - i - 1];
        }

        fd = open(full_path, O_WRONLY);
        if (fd == -1) {
            free(reverse_buf);
            return -errno;
        }

        res = pwrite(fd, reverse_buf, size, offset);
        free(reverse_buf);

        if (res == -1) {
            close(fd);
            return -errno;
        }

        close(fd);
        return res;
    } else {
        // Jika bukan file "test", menulis seperti biasa
        fd = open(full_path, O_WRONLY);
        if (fd == -1)
            return -errno;

        res = pwrite(fd, buf, size, offset);
        if (res == -1) {
            close(fd);
            return -errno;
        }

        close(fd);
        return res;
    }
}

// Fungsi untuk memangkas ukuran file
static int xmp_truncate(const char *path, off_t size) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = truncate(full_path, size);
    if (res == -1)
        return -errno;

    return 0;
}

// Fungsi untuk membuat hard link
static int xmp_link(const char *from, const char *to) {
    int res;
    char full_from[1024], full_to[1024];

    snprintf(full_from, sizeof(full_from), "%s%s", dirpath, from);
    snprintf(full_to, sizeof(full_to), "%s%s", dirpath, to);

    res = link(full_from, full_to);
    if (res == -1)
        return -errno;

    return 0;
}

// Fungsi untuk mengubah nama file
static int xmp_rename(const char *from, const char *to) {
    int res;
    char full_from[1024], full_to[1024];

    snprintf(full_from, sizeof(full_from), "%s%s", dirpath, from);
    snprintf(full_to, sizeof(full_to), "%s%s", dirpath, to);

    // Menambahkan watermark jika dipindahkan ke direktori yang berisi "wm" dalam namanya
    if (strstr(to, watermarked_prefix) != NULL) {
        add_watermark_and_rename(full_from, full_to);
    } else {
        res = rename(full_from, full_to);
        if (res == -1)
            return -errno;
    }

    return 0;
}

// Fungsi untuk menghapus file
static int xmp_unlink(const char *path) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = unlink(full_path);
    if (res == -1)
        return -errno;

    return 0;
}

// Fungsi untuk membuat direktori
static int xmp_mkdir(const char *path, mode_t mode) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = mkdir(full_path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

// Fungsi untuk membuat file
static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    int res = open(full_path, fi->flags, mode);
    if (res == -1)
        return -errno;

    fi->fh = res;

    // Memanggil fungsi reverse_content jika file memiliki prefix "test"
    if (strncmp(basename(full_path), test_prefix, strlen(test_prefix)) == 0) {
        reverse_content(full_path);
    }

    return 0;
}

// Fungsi untuk mengubah waktu akses dan modifikasi file
static int xmp_utimens(const char *path, const struct timespec ts[2]) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = utimensat(0, full_path, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
        return -errno;

    return 0;
}

// Fungsi untuk mengubah mode file
static int xmp_chmod(const char *path, mode_t mode) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = chmod(full_path, mode);
    if (res == -1)
        return -errno;

    // Jika file yang dimodifikasi adalah script.sh di dalam direktori /bahaya
    if (strcmp(dirpath, "/bahaya/script.sh") == 0) {
        // Berikan izin eksekusi pada file script.sh
        mode_t exec_mode = mode | S_IXUSR | S_IXGRP | S_IXOTH;
        res = chmod(full_path, exec_mode);
        if (res == -1)
            return -errno;
    }

    return 0;
}

// Inisialisasi operasi FUSE
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .chmod = xmp_chmod,
    .truncate = xmp_truncate,
    .link = xmp_link,
    .rename = xmp_rename,
    .unlink = xmp_unlink,
    .mkdir = xmp_mkdir,
    .create = xmp_create,
    .utimens = xmp_utimens,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}

