#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#define MAX_PATH 4096

#define NOCOLOR "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"

#define handle_error_en(en, msg) do { errno = en; perror(msg); pthread_exit(NULL); } while (0)
#define handle_error(msg) do { perror(msg); pthread_exit(NULL); } while (0)

// Структура для передачи данных в потоки
typedef struct {
    char src[MAX_PATH];
    char dest[MAX_PATH];
} thread_data_t;

char* get_directory_name(const char* path) {
    // Создаем копию строки, так как basename модифицирует исходный путь
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);

    // Используем basename для получения имени каталога
    return basename(path_copy);
}

void create_new_path(const char* base_path, const char* new_folder, char* result_path) {
    // Проверяем, заканчивается ли base_path на '/'
    if (base_path[strlen(base_path) - 1] == '/') {
        // Если заканчивается, просто добавляем новое имя каталога
        snprintf(result_path, 1024, "%s%s", base_path, new_folder);
    } else {
        // Если не заканчивается, добавляем '/' и новое имя каталога
        snprintf(result_path, 1024, "%s/%s", base_path, new_folder);
    }
}

// Функция копирования файла
void *copy_file(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;

    int src_fd, dest_fd;
    char buffer[1024*16];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;

    // Получаем права доступа исходного файла
    if (stat(data->src, &src_stat) == -1) {
        perror(RED"Failed to stat source file");
        fprintf(stderr, "Source: %s\n", data->src);
        printf(NOCOLOR);
        free(data);
        pthread_exit(NULL);
    }

    // Открываем исходный файл
    while ((src_fd = open(data->src, O_RDONLY)) == -1 && errno == EMFILE) {
        sleep(1);
    }

    if (src_fd == -1) {
        perror(RED"Failed to open source file");
        printf(NOCOLOR);
        free(data);
        pthread_exit(NULL);
    }

    //printf(YELLOW"Start copy file: %s"NOCOLOR"\n", data->dest);

    // Создаем целевой файл
    while ((dest_fd = open(data->dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode)) == -1 && errno == EMFILE) {
        sleep(1);
    }

    if (dest_fd == -1) {
        fprintf(stderr, "Failed to create destination file '%s'", data->dest);
        perror(" ");
        close(src_fd);
        free(data);
        pthread_exit(NULL);
    }

    // Копируем данные
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;

        while (total_written < bytes_read) {
            bytes_written = write(dest_fd, &(buffer[total_written]), bytes_read - total_written);
            if (bytes_written == -1) {
                perror(RED"Write error");
                printf(NOCOLOR);
                break;
            }

            total_written += bytes_written;
        }

        if (total_written != bytes_read) {
            break;
        }
    }

    if (bytes_read == -1) {
        perror(RED"Read error");
        printf(NOCOLOR);
    }

    close(src_fd);
    close(dest_fd);
    free(data);

    pthread_exit(NULL);
}

// Функция копирования каталога
void *copy_directory(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    DIR *src_dir;
    struct dirent *entry;
    struct stat statbuf;
    struct stat src_stat;
    pthread_t thread;
    int err;

    // Получаем права доступа исходного каталога
    if (stat(data->src, &src_stat) == -1) {
        perror(RED"Failed to stat source directory");
        fprintf(stderr, "Source directory: %s\n", data->src);
        printf(NOCOLOR);
        free(data);
        pthread_exit(NULL);
    }

    // Открываем исходный каталог
    size_t dirent_buf_size = sizeof(struct dirent) + pathconf(data->src, _PC_NAME_MAX) + 1;
    struct dirent *entry_buf = malloc(dirent_buf_size);

    while ((src_dir = opendir(data->src)) == NULL && errno == EMFILE) {
        sleep(1);
    }

    if (!src_dir) {
        perror(RED"Failed to open source directory");
        printf(NOCOLOR);
        free(data);
        free(entry_buf);
        pthread_exit(NULL);
    }

    // Создаем целевой каталог
    if (mkdir(data->dest, src_stat.st_mode) == -1 && errno != EEXIST) {
        perror(RED"Failed to create destination directory");
        printf("%s", data->dest);
        printf(NOCOLOR);
        closedir(src_dir);
        free(data);
        free(entry_buf);
        pthread_exit(NULL);
    }

    //printf(BLUE"Start copy directory: %s"NOCOLOR"\n", data->dest);

    // Обходим содержимое каталога
    //(entry = readdir(src_dir)) != NULL
    while (readdir_r(src_dir, entry_buf, &entry) == 0 && (entry != NULL)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        thread_data_t *new_data = malloc(sizeof(thread_data_t));

        if (snprintf(new_data->src, MAX_PATH, "%s/%s", data->src, entry->d_name) >= MAX_PATH) {
            fprintf(stderr, RED"Source path is too long: %s/%s"NOCOLOR"\n", data->src, entry->d_name);
            free(new_data);
            continue;
        }

        if (snprintf(new_data->dest, MAX_PATH, "%s/%s", data->dest, entry->d_name) >= MAX_PATH) {
            fprintf(stderr, RED"Destination path is too long: %s/%s"NOCOLOR"\n", data->dest, entry->d_name);
            free(new_data);
            continue;
        }

        // snprintf(new_data->src, MAX_PATH, "%s/%s", data->src, entry->d_name);
        // snprintf(new_data->dest, MAX_PATH, "%s/%s", data->dest, entry->d_name);

        if (stat(new_data->src, &statbuf) == -1) {
            perror(RED"Failed to stat file");
            printf(NOCOLOR);
            free(new_data);
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            err = pthread_create(&thread, NULL, copy_directory, new_data);
            if (err != 0) {
                closedir(src_dir);
                free(data);
                free(entry_buf);
                handle_error_en(err, "dir create pthread create error");
            }

            err = pthread_detach(thread);
            if (err != 0) {
                closedir(src_dir);
                free(data);
                free(entry_buf);    
                handle_error_en(err, "dir create pthread detach error");
            }
        } else if (S_ISREG(statbuf.st_mode)) {
            err = pthread_create(&thread, NULL, copy_file, new_data);
            if (err != 0) {
                closedir(src_dir);
                free(data);
                free(entry_buf);
                handle_error_en(err,"reg create pthread create error");
            }
            err = pthread_detach(thread);
            if (err != 0) {
                closedir(src_dir);
                free(data);
                free(entry_buf);
                handle_error_en(err, "reg create pthread detach error");
            }
        } else {
            free(new_data); // Игнорируем другие типы файлов
        }
    }

    closedir(src_dir);
    free(data);
    free(entry_buf);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, RED"Usage: %s <source_directory> <destination_directory>"NOCOLOR"\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Проверяем существование исходного каталога
    struct stat statbuf;
    if (stat(argv[1], &statbuf) == -1 || !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, RED"Source directory does not exist or is not a directory"NOCOLOR"\n");
        return EXIT_FAILURE;
    }

    char dst_path[MAX_PATH];

    // Проверяем существование целевого каталога
    if (access(argv[2], F_OK) != 0) {
        strcpy(dst_path, argv[2]);
    } else {
        create_new_path(argv[2], get_directory_name(argv[1]), dst_path);
    }

    // Создаем структуру данных для потока
    thread_data_t *data = malloc(sizeof(thread_data_t));
    snprintf(data->src, MAX_PATH, "%s", argv[1]);
    snprintf(data->dest, MAX_PATH, "%s", dst_path);

    pthread_t thread;
    int err = pthread_create(&thread, NULL, copy_directory, data);
    if (err != 0) {
        handle_error_en(err, "main pthread_create error");
    }
    err = pthread_detach(thread);
    if (err != 0) {
        handle_error_en(err, "main pthread_detach error");
    }

    pthread_exit(NULL);
}
