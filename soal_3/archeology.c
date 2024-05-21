#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *relics_path = "/path/to/relics"; 

static int relic_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s%s.000", relics_path, path);
        if (stat(fullpath, stbuf) == -1)
            return -errno;
        stbuf->st_mode = S_IFREG | 0644;
    }
    return 0;
}

static int relic_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    DIR *dp;
    struct dirent *de;
    dp = opendir(relics_path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        if (strstr(de->d_name, ".000")) {
            char name[256];
            strncpy(name, de->d_name, strlen(de->d_name) - 4);
            name[strlen(de->d_name) - 4] = '\0';
            filler(buf, name, NULL, 0, 0);
        }
    }

    closedir(dp);
    return 0;
}

static int relic_open(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int relic_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", relics_path, path);
    FILE *fp;
    size_t len = 0;
    size_t part_size;
    char part_path[1024];

    int i = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    while (1) {
        snprintf(part_path, sizeof(part_path), "%s.%d", fullpath, i);
        fp = fopen(part_path, "rb");
        if (!fp)
            break;
        fseek(fp, 0, SEEK_END);
        part_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (offset < len + part_size) {
            size_t to_read = part_size - (offset - len);
            if (to_read > size)
                to_read = size;
            fseek(fp, offset - len, SEEK_SET);
            fread(buf, 1, to_read, fp);
            size -= to_read;
            buf += to_read;
            offset += to_read;
        }

        len += part_size;
        fclose(fp);
        i++;
    }
#pragma GCC diagnostic pop

    return size;
}

static const struct fuse_operations relic_oper = {
    .getattr = relic_getattr,
    .readdir = relic_readdir,
    .open = relic_open,
    .read = relic_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &relic_oper, NULL);
}
