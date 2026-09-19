#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

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
    long long orders;
    double revenue;
} PartialResult;

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

void worker_process(Dataset* dataset, int start_idx, int end_idx,
                    int choice, const char* search_term, PartialResult* out) {
    out->orders = 0;
    out->revenue = 0.0;
    for (int i = start_idx; i < end_idx; i++) {
        if (field_matches(&dataset->records[i], choice, search_term)) {
            out->orders++;
            out->revenue += dataset->records[i].revenue;
        }
    }
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char* argv[]) {
    const char* data_dir = (argc > 1) ? argv[1] : "./data";
    printf("=== MULTIPROCESSING APPROACH ===\n\n");
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
    int num_processes[] = {2, 4, 8, 12, 16, 20};
    for (int p = 0; p < 6; p++) {
        int n_proc = num_processes[p];
        printf("\n--- Testing with %d processes ---\n", n_proc);
        double start_time = get_time();
        pid_t pids[n_proc];
        int pipes[n_proc][2];
        for (int i = 0; i < n_proc; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(1);
            }
        }
        int records_per_process = dataset.count / n_proc;
        for (int i = 0; i < n_proc; i++) {
            pids[i] = fork();
            if (pids[i] < 0) {
                perror("fork");
                exit(1);
            }
            if (pids[i] == 0) {
                close(pipes[i][0]);
                int start_idx = i * records_per_process;
                int end_idx = (i == n_proc - 1) ? dataset.count : (i + 1) * records_per_process;
                PartialResult partial;
                worker_process(&dataset, start_idx, end_idx, choice, search, &partial);
                write(pipes[i][1], &partial, sizeof(PartialResult));
                close(pipes[i][1]);
                _exit(0);
            }
        }
        Results final_result = {0, 0.0};
        for (int i = 0; i < n_proc; i++) {
            close(pipes[i][1]);
            PartialResult partial;
            ssize_t r = read(pipes[i][0], &partial, sizeof(PartialResult));
            if (r == sizeof(PartialResult)) {
                final_result.orders += partial.orders;
                final_result.revenue += partial.revenue;
            }
            close(pipes[i][0]);
            waitpid(pids[i], NULL, 0);
        }
        double end_time = get_time();
        double process_time = end_time - start_time;
        printf("Total Orders: %lld\n", final_result.orders);
        printf("Total Revenue: $%.2f\n", final_result.revenue);
        printf("Processing Time: %.6f seconds\n", process_time);
    }
    free(dataset.records);
    return 0;
}
