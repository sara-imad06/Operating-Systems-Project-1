#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <pthread.h>

#define MAX_RECORDS 1000000
#define NUM_FILES 20

typedef struct {
    char country[64];
    char item_type[64];
    char sales_channel[32];
    double revenue;
} Record;

typedef struct {
    Record* records;
    int count;
} Dataset;

typedef struct {
    long long orders;
    double revenue;
} Results;

typedef struct {
    int thread_id;
    int num_threads;
    int choice;
    char search_term[100];
    Dataset* dataset;
} ThreadArgs;

pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
Results global_result;

int split_csv_line(char* line, char* fields[], int max_fields) {
    int count = 0;
    char* p = line;
    while (count < max_fields) {
        int quoted = 0;
        if (*p == '"') { quoted = 1; p++; }
        fields[count] = p;
        if (quoted) {
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
            while (*p && *p != ',') p++;
        } else {
            while (*p && *p != ',') p++;
        }
        count++;
        if (*p == ',') { *p = '\0'; p++; }
        else break;
    }
    return count;
}

void parse_line_into_record(char* line, Record* rec) {
    char* fields[20];
    int n = split_csv_line(line, fields, 20);
    rec->country[0] = '\0';
    rec->item_type[0] = '\0';
    rec->sales_channel[0] = '\0';
    rec->revenue = 0.0;
    if (n > 1) strncpy(rec->country, fields[1], sizeof(rec->country) - 1);
    if (n > 2) strncpy(rec->item_type, fields[2], sizeof(rec->item_type) - 1);
    if (n > 3) strncpy(rec->sales_channel, fields[3], sizeof(rec->sales_channel) - 1);
    if (n > 11) rec->revenue = atof(fields[11]);
}

Dataset read_all_files(const char* data_dir) {
    Dataset dataset;
    dataset.records = malloc((size_t)MAX_RECORDS * sizeof(Record));
    dataset.count = 0;
    if (!dataset.records) {
        fprintf(stderr, "Failed to allocate memory for records\n");
        exit(1);
    }
    char file_letters[] = "abcdefghijklmnopqrst";
    for (int i = 0; i < NUM_FILES; i++) {
        char filename[512];
        snprintf(filename, sizeof(filename), "%s/xa%c.csv", data_dir, file_letters[i]);
        FILE* file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "Warning: could not open %s (skipped)\n", filename);
            continue;
        }
        char line[2048];
        if (fgets(line, sizeof(line), file) == NULL) { fclose(file); continue; }
        while (fgets(line, sizeof(line), file) && dataset.count < MAX_RECORDS) {
            line[strcspn(line, "\r\n")] = 0;
            if (line[0] == '\0') continue;
            parse_line_into_record(line, &dataset.records[dataset.count]);
            dataset.count++;
        }
        fclose(file);
    }
    return dataset;
}

int field_matches(const Record* r, int choice, const char* term) {
    switch (choice) {
        case 1: return strcasecmp(r->country, term) == 0;
        case 2: return strcasecmp(r->item_type, term) == 0;
        case 3: return strcasecmp(r->sales_channel, term) == 0;
    }
    return 0;
}

void* worker_thread(void* arg) {
    ThreadArgs* data = (ThreadArgs*)arg;
    Results local_result = {0, 0.0};
    for (int i = data->thread_id; i < data->dataset->count; i += data->num_threads) {
        if (field_matches(&data->dataset->records[i], data->choice, data->search_term)) {
            local_result.orders++;
            local_result.revenue += data->dataset->records[i].revenue;
        }
    }
    pthread_mutex_lock(&result_mutex);
    global_result.orders += local_result.orders;
    global_result.revenue += local_result.revenue;
    pthread_mutex_unlock(&result_mutex);
    pthread_exit(NULL);
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char* argv[]) {
    const char* data_dir = (argc > 1) ? argv[1] : "./data";
    printf("=== MULTITHREADING APPROACH ===\n\n");
    printf("Reading files into memory...\n");
    Dataset dataset = read_all_files(data_dir);
    printf("Loaded %d records\n\n", dataset.count);
    int choice;
    char search[100];
    while (1) {
        printf("Search by:\n1. Country\n2. Item Type\n3. Sales Channel\n");
        printf("Choice (1-3): ");
        if (scanf("%d", &choice) == 1 && choice >= 1 && choice <= 3) break;
        printf("Invalid input! Please enter 1-3.\n\n");
        int c; while ((c = getchar()) != '\n' && c != EOF);
    }
    getchar();
    printf("Enter search term: ");
    fgets(search, sizeof(search), stdin);
    search[strcspn(search, "\n")] = 0;
    int num_threads_list[] = {2, 4, 8, 12, 16, 20};
    for (int t = 0; t < 6; t++) {
        int n_threads = num_threads_list[t];
        printf("\n--- Testing with %d threads ---\n", n_threads);
        double start_time = get_time();
        pthread_t threads[n_threads];
        ThreadArgs thread_args[n_threads];
        global_result.orders = 0;
        global_result.revenue = 0.0;
        for (int i = 0; i < n_threads; i++) {
            thread_args[i].thread_id = i;
            thread_args[i].num_threads = n_threads;
            thread_args[i].choice = choice;
            strncpy(thread_args[i].search_term, search, sizeof(thread_args[i].search_term) - 1);
            thread_args[i].search_term[sizeof(thread_args[i].search_term) - 1] = '\0';
            thread_args[i].dataset = &dataset;
            int rc = pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]);
            if (rc != 0) {
                fprintf(stderr, "pthread_create failed: %d\n", rc);
                exit(1);
            }
        }
        for (int i = 0; i < n_threads; i++) pthread_join(threads[i], NULL);
        double end_time = get_time();
        double process_time = end_time - start_time;
        printf("Total Orders: %lld\n", global_result.orders);
        printf("Total Revenue: $%.2f\n", global_result.revenue);
        printf("Processing Time: %.6f seconds\n", process_time);
    }
    pthread_mutex_destroy(&result_mutex);
    free(dataset.records);
    return 0;
}
