#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define PATH_SIZE 512

void reverse_line(char *line) {
    int len = strlen(line);
    for (int i = 0; i < len / 2; i++) {
        char temp = line[i];
        line[i] = line[len - i - 1];
        line[len - i - 1] = temp;
    }
}

void process_file(const char *in, const char *out) {
    FILE *fi = fopen(in, "r"), *fo = fopen(out, "w");
    if (!fi || !fo) return;
    char line[1024];
    while (fgets(line, sizeof(line), fi)) {
        reverse_line(line);
        fputs(line, fo);
    }
    fclose(fi); fclose(fo);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        return 1;
    }
    struct stat st;
    if (stat(argv[2], &st) == -1) {
        mkdir(argv[2], 0700);
    }
    DIR *d = opendir(argv[1]);
    struct dirent *e;
    while (e = readdir(d)) {
        if (strstr(e->d_name, ".txt")) {
            char in[PATH_SIZE], out[PATH_SIZE];
            snprintf(in, PATH_SIZE - 1, "%s/%s", argv[1], e->d_name);
            snprintf(out, PATH_SIZE - 1, "%s/%s", argv[2], e->d_name);
            in[PATH_SIZE - 1] = '\0';
            out[PATH_SIZE - 1] = '\0';
            process_file(in, out);
        }
    }
    closedir(d);
    return 0;
}
