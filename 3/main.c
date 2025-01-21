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

// Структура для передачи данных в потоки
typedef struct {
    char src[MAX_PATH];
    char dest[MAX_PATH];
} thread_data_t;

// Счетчик активных потоков и мьютекс для его защиты
static int active_threads = 0;
static pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t thread_count_cond = PTHREAD_COND_INITIALIZER;

void decrease_active_threads() {
    pthread_mutex_lock(&thread_count_mutex);
    active_threads--;
    pthread_cond_signal(&thread_count_cond);
    pthread_mutex_unlock(&thread_count_mutex);
}

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
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;

    // Получаем права доступа исходного файла
    if (stat(data->src, &src_stat) == -1) {
        perror(RED"Failed to stat source file");
        fprintf(stderr, "Source: %s\n", data->src);
        printf(NOCOLOR);
        free(data);
        decrease_active_threads();
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
        decrease_active_threads();
        pthread_exit(NULL);
    }

    //printf(YELLOW"Start copy file: %s"NOCOLOR"\n", data->dest);

    // Создаем целевой файл
    while ((dest_fd = open(data->dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode)) == -1 && errno == EMFILE) {
        sleep(1);
    }

    if (dest_fd == -1) {
        perror(RED"Failed to create destination file");
        printf(NOCOLOR);
        close(src_fd);
        free(data);
        decrease_active_threads();
        pthread_exit(NULL);
    }

    // Копируем данные
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror(RED"Write error");
            printf(NOCOLOR);
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

    decrease_active_threads();

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

    // Получаем права доступа исходного каталога
    if (stat(data->src, &src_stat) == -1) {
        perror(RED"Failed to stat source directory");
        fprintf(stderr, "Source directory: %s\n", data->src);
        printf(NOCOLOR);
        free(data);
        decrease_active_threads();
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
        decrease_active_threads();
        pthread_exit(NULL);
    }

    // Создаем целевой каталог
    if (mkdir(data->dest, src_stat.st_mode) == -1 && errno != EEXIST) {
        perror(RED"Failed to create destination directory");
        printf(NOCOLOR);
        closedir(src_dir);
        free(data);
        free(entry_buf);
        decrease_active_threads();
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

        pthread_mutex_lock(&thread_count_mutex);
        active_threads++;
        pthread_mutex_unlock(&thread_count_mutex);

        if (S_ISDIR(statbuf.st_mode)) {
            pthread_create(&thread, NULL, copy_directory, new_data);
            pthread_detach(thread);
        } else if (S_ISREG(statbuf.st_mode)) {
            pthread_create(&thread, NULL, copy_file, new_data);
            pthread_detach(thread);
        } else {
            free(new_data); // Игнорируем другие типы файлов
            decrease_active_threads();
        }
    }

    closedir(src_dir);
    free(data);
    free(entry_buf);

    decrease_active_threads();

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

    create_new_path(argv[2], get_directory_name(argv[1]), dst_path);

    // Создаем структуру данных для потока
    thread_data_t *data = malloc(sizeof(thread_data_t));
    snprintf(data->src, MAX_PATH, "%s", argv[1]);
    snprintf(data->dest, MAX_PATH, "%s", dst_path);

    pthread_mutex_lock(&thread_count_mutex);
    active_threads++;
    pthread_mutex_unlock(&thread_count_mutex);

    pthread_t thread;
    pthread_create(&thread, NULL, copy_directory, data);
    pthread_detach(thread);

    // Ожидание завершения всех потоков
    pthread_mutex_lock(&thread_count_mutex);
    while (active_threads > 0) {
        pthread_cond_wait(&thread_count_cond, &thread_count_mutex);
    }
    pthread_mutex_unlock(&thread_count_mutex);

    //printf("All threads completed. Copy operation finished.\n");

    return EXIT_SUCCESS;
}
