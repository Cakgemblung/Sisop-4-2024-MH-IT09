# Sisop-4-2024-MH-IT09

 Nama          | NRP          |
| ------------- | ------------ |
| Kevin Anugerah Faza | 5027231027 |
| Muhammad Hildan Adiwena | 5027231077 |
| Nayyara Ashila | 5027231083 |


## Soal 1
Berikut adalah code yang kami buat untuk mengerjakan soal 1
```
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
```
Bagian tersebut mendeklarasikan beberapa library yang digunakan, dimana kami menggunakan `FUSE_USE_VERSION 30`. Selain itu juga didefinisikan beberapa hal penting misalnya path directory dengan `dirpath` dan beberapa prefix menggunakan `static const char`
```
void add_watermark_and_rename(const char *src, const char *dest) {
    char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "%s.temp", dest);

    char command[2048];
    snprintf(command, sizeof(command), "convert '%s' -gravity South -pointsize 36 -fill white -annotate +0+100 'inikaryakita.id' '%s'", src, temp_path);
    system(command);

    rename(temp_path, dest);
}
```
Pada fungsi `add_Watermark_and_rename` digunakan untuk menambahkan watermark pada gambar yang diberikan (`src`) dan mengganti namanya menjadi `dest`. Fungsi ini menggunakan alat baris perintah `convert` dari ImageMagick untuk menambahkan watermark.
```
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
```
Fungsi ini digunakan untuk membalik isi file yang diberikan `path`. Fungsi ini membuka file, membaca isinya ke buffer, membalikkan isi buffer, lalu menulisnya kembali ke file.
```
static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = lstat(full_path, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}
```
Fungsi ini digunakan untuk mendapatkan atribut file. Ini adalah implementasi dari operasi `getattr` pada FUSE.
```
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
```
Fungsi ini digunakan untuk membaca isi direktori. Ini adalah implementasi dari operasi `readdir` pada FUSE.
```
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
```
Fungsi ini digunakan untuk membuka file. Ini adalah implementasi dari operasi `open` pada FUSE.
```
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
```
Fungsi ini digunakan untuk membaca isi file. Jika file memiliki prefix "test", isi file akan dibalik sebelum dikembalikan. Ini adalah implementasi dari operasi `read` pada FUSE.
```
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
```
Fungsi ini digunakan untuk menulis isi ke file. Jika file memiliki prefix "test", isi buffer akan dibalik sebelum ditulis. Ini adalah implementasi dari operasi `write` pada FUSE.
```
static int xmp_truncate(const char *path, off_t size) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = truncate(full_path, size);
    if (res == -1)
        return -errno;

    return 0;
}
```
Fungsi ini digunakan untuk memangkas ukuran file. Ini adalah implementasi dari operasi `truncate` pada FUSE.
```
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
```
Fungsi ini digunakan untuk membuat hard link dari `from` ke `to`. Ini adalah implementasi dari operasi `link` pada FUSE.
```
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
```
Fungsi ini digunakan untuk mengubah nama file dari `from` ke `to`. Jika file dipindahkan ke direktori yang berisi prefix "wm" dalam namanya, fungsi ini akan menambahkan watermark pada file tersebut.
```
static int xmp_unlink(const char *path) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = unlink(full_path);
    if (res == -1)
        return -errno;

    return 0;
}
```
Fungsi ini digunakan untuk menghapus file. Ini adalah implementasi dari operasi `unlink` pada FUSE.
```
static int xmp_mkdir(const char *path, mode_t mode) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = mkdir(full_path, mode);
    if (res == -1)
        return -errno;

    return 0;
}
```
Fungsi ini digunakan untuk membuat direktori
```
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
```
Fungsi ini digunakan untuk membuat file baru. Jika file memiliki prefix "test", isi file akan dibalik menggunakan fungsi reverse_content. Ini adalah implementasi dari operasi `create` pada FUSE.
```
static int xmp_utimens(const char *path, const struct timespec ts[2]) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = utimensat(0, full_path, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
        return -errno;

    return 0;
}
```
Fungsi ini digunakan untuk mengubah waktu akses dan modifikasi file. Ini adalah implementasi dari operasi `utimens` pada FUSE.
```
static int xmp_chmod(const char *path, mode_t mode) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = chmod(full_path, mode);
    if (res == -1)
        return -errno;

    if (strcmp(dirpath, "/bahaya/script.sh") == 0) {
        mode_t exec_mode = mode | S_IXUSR | S_IXGRP | S_IXOTH;
        res = chmod(full_path, exec_mode);
        if (res == -1)
            return -errno;
    }

    return 0;
}
```
Fungsi ini digunakan untuk mengubah mode file. Jika file adalah `script.sh` dalam direktori "/bahaya", fungsi ini akan memberikan izin eksekusi. Ini adalah implementasi dari operasi `chmod` pada FUSE.
```
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
```
Struct diatas mendefinisikan operasi FUSE yang didukung oleh filesystem yang diimplementasikan dalam kode ini. Kemudian fungsi main adalah titik masuk dari program ini. Fungsi ini menginisialisasi umask dan memanggil fuse_main untuk memulai filesystem FUSE dengan operasi yang telah didefinisikan di atas.

Screenshot hasil code
![image](https://github.com/Cakgemblung/Sisop-4-2024-MH-IT09/assets/144968322/b06153f8-29a1-47e1-ad08-b77196e66d30)
![image](https://github.com/Cakgemblung/Sisop-4-2024-MH-IT09/assets/144968322/ef7112f3-6935-4d8f-ad9f-a38e9907529b)
![image](https://github.com/Cakgemblung/Sisop-4-2024-MH-IT09/assets/144968322/f07ab740-f2a0-4d82-babc-b7ef78773ae2)




*REVISI*

## Soal 1

Karena program tidak dapat secara otomatis menjalankan dan merubah permission pada script, maka approachment yang dilakukan kali ini berbeda. Berikut adalah hasil modifikasi yang dilakukan pada bagian fungsi untuk melakukan operasi `chmod`
```
static int xmp_chmod(const char *path, mode_t mode) {
    int res;
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);
    res = chmod(full_path, mode);
    if (res == -1)
        return -errno;

    if (strcmp(full_path, "/bahaya/script.sh") == 0) {
        char command[1024];
        snprintf(command, sizeof(command), "chmod +x '%s'", full_path);
        res = system(command);
        if (res == -1)
            return -errno;
    }

    return 0;
}
```
Dengan modifikasi ini, ketika fungsi `xmp_chmod` dipanggil untuk mengubah mode file /bahaya/script.sh, perintah `chmod +x script.sh` akan dieksekusi secara otomatis. Sebagai rincian jika path file adalah /bahaya/script.sh, maka perintah chmod +x akan dijalankan menggunakan fungsi system. Ini akan mengeksekusi perintah chmod +x pada file tersebut.


## Soal 2

Berikut fungsi untuk decode algoritma Base64
```
char* decode_base64(const char* input) {
    size_t len = strlen(input);
    size_t output_len = len * 3 / 4;
    char *output = malloc(output_len + 1);
    if (!output) return NULL;

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
```

berikut fungsi untuk decode dengan algoritma ROT13
```
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
```

berikut fungsi untuk decode algoritma hexadecimal
```
void decode_hex(const char* input, char* output) {
    size_t len = strlen(input);
    for (size_t i = 0; i < len; i += 2) {
        sscanf(input + i, "%2hhx", &output[i / 2]);
    }
    output[len / 2] = '\0';
}
```

berikut fungsi untuk decode reverse
```
void decode_reverse(char* input) {
    int len = strlen(input);
    for (int i = 0; i < len / 2; i++) {
        char temp = input[i];
        input[i] = input[len - i - 1];
        input[len - i - 1] = temp;
    }
}
```

berikut code untuk fuse
```
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
```

## Soal 3

Buat path yang akan menjadi tujuan  ```static const char *relics_path = "/home/nayyara/sisop4/relics"; ```

```
static int xmp_getattr(const char *path, struct stat *stbuf) {
   memset(stbuf, 0, sizeof(struct stat));
```
Digunakan untuk mendapatkan atribut file. Pertama, buffer stbuf yang diinisialisasi dengan nol.

```
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
```
Jika path adalah root ``/`` maka diatur atributnya sebagai direktori dengan permission 0755 dan jumlah link 2.

```
                while (1) {
            snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, i++);
            fp = fopen(part_path, "rb");
            if (!fp) break;
```
Fungsi diatas adalah loop untuk membuka setiap bagian file (part file). `snprintf` digunakan untuk membuat path dari setiap bagian file dengan format fpath.xxx. Jika file tidak bisa dibuka, maka loop berhenti.

```
if (i == 1) return -ENOENT;
    }
    return 0;
}
```
Jika tidak ada potongan file yang ditemukan (i == 1), fungsi tersebut mengembalikan `-ENOENT` atau file tidak ditemukan. Jika terdapat potongan file, fungsi tersebut akan mengembalikan `0` atau sukses.

```static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {```
Fungsi tersebut akan membaca isi dari direktori.

```
while ((de = readdir(dp)) != NULL) {
        if (strstr(de->d_name, ".000") != NULL) {
            char base_name[256];
            strncpy(base_name, de->d_name, strlen(de->d_name) - 4);
            base_name[strlen(de->d_name) - 4] = '\0';
            filler(buf, base_name, NULL, 0);
        }
    }
    closedir(dp);
    return 0;
}
```
Diatas adalah Loop untuk membaca setiap entri dalam direktori. Jika nama file memiliki `.000` itu berarti file tersebut adalah bagian pertama dari suatu file multipart. Nama pertama file akan diekstraksi kemudian ditambahkan ke buffer menggunakan `filler`

```static int xmp_open(const char *path, struct fuse_file_info *fi) {```
Fungsi diatas digunakan untuk membuka file

```
while (size > 0) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, i++);
        FILE *fp = fopen(part_path, "rb");
        if (!fp) break;
```
Loop tersebut digunakan untuk membaca setiap bagian di dalam file. Path bagian file kemudian dibuat lalu file tersebut akan dibuka. Jika file tidak dapat dibuka, maka loop tersebut akan berhenti.

```static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {``` Berfungsi untuk untuk menulis data ke dalam file. 

```
static int xmp_unlink(const char *path) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int part_num = 0;
    char part_path[1100];
    int res = 0;
```
Fungsi `xmp_unlink` untuk menghapus file. Kemudian path lengkap file dibuat lalu dilakukan inisialisasi variabel untuk hasilnya. Fungsi ini juga akan menghapus setiap pecahan file sampai tidak ada lagi pecahan yang ditemukan.

```
static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s.000", root_path, path);

    int res = creat(fpath, mode);
    if (res == -1) return -errno;

    close(res);
    return 0;
}
```
Fungsi `xmp_create` untuk membuat file baru kemudian Path lengkap untuk bagian pertama file dibuat. Jika proses tersebut gagal, maka akan mengembalikan nilai kesalahan. Jika proses tersebut berhasil dilakukan, maka file akan ditutup dan meneembalikan nilai `0`


```
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .unlink = xmp_unlink,
    .create = xmp_create,
    .truncate = xmp_truncate,
};
```
Fungsi tersebut akan mendefinisikan struktur fuse_operations dengan mengisi setiap operasi melalui fungsi yang telah diimplementasikan.

```
int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
```
Fungsi `main` diatas merupakan proses masuk ke dalam program. Mengatur umask ke 0 agar permission file tidak dipengaruhi oleh umask proses. Fungsi ini akan memanggil `fuse_main` dari data yang sudah dimasukkan dari pengguna.



