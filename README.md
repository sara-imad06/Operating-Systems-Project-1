# Operating Systems — Project 1

<p align="center">
  <img src="https://img.shields.io/badge/Operating%20Systems-Project%201-6C63FF?style=for-the-badge" alt="Operating Systems">
  <img src="https://img.shields.io/badge/C-Programming-00599C?style=for-the-badge&logo=c" alt="C">
  <img src="https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black" alt="Linux">
  <img src="https://img.shields.io/badge/Processes%20%26%20Threads-00A896?style=for-the-badge" alt="Processes and Threads">
</p>

---

## Project Overview

This project processes a dataset containing **1,000,000 records divided into 20 files**.

The program allows the user to search by:

1. Country
2. Item Type
3. Sales Channel

After entering a search value, the program calculates:

- Total number of orders
- Total revenue
- Processing time

The same task is implemented using three different approaches:

- Naive
- Multiprocessing
- Multithreading

---

## Dataset

- **Records:** 1,000,000
- **Files:** 20
- **Format:** CSV
- **Data fields used:** Country, Item Type, Sales Channel, Revenue

The program loads the 20 files and stores the records in memory before processing the search.

---

## User Input

The program displays the following menu:

```text
Search by:
1. Country
2. Item Type
3. Sales Channel

Choice (1-3):
```

The user then enters the value to search for.

Example:

```text
Choice: 1
Enter search term: Ireland
```

The search is case-insensitive.

---

# Approaches

## 1. Naive Approach

The Naive implementation processes all records sequentially using a single process.

```text
Main Process
     |
     +-- Record 1
     +-- Record 2
     +-- Record 3
     +-- ...
     +-- Record 1,000,000
```

No child processes or threads are used.

The processing time is measured using `clock_gettime()`.

---

## 2. Multiprocessing Approach

The Multiprocessing implementation uses multiple child processes created with `fork()`.

The tested configurations are:

- 2 processes
- 4 processes
- 8 processes
- 12 processes
- 16 processes
- 20 processes

Each process handles a portion of the dataset. Pipes are used to send the partial results back to the parent process, and `waitpid()` is used to wait for the child processes.

```text
              Parent Process
                    |
        +-----------+-----------+
        |           |           |
      Child       Child       Child
        |           |           |
     Records     Records     Records
        |           |           |
        +-----------+-----------+
                    |
              Final Results
```

---

## 3. Multithreading Approach

The Multithreading implementation uses POSIX threads.

The tested configurations are:

- 2 threads
- 4 threads
- 8 threads
- 12 threads
- 16 threads
- 20 threads

Each thread processes part of the dataset.

`pthread_create()` is used to create the threads, while `pthread_join()` waits for all threads to finish. A mutex is used to safely update the shared result.

```text
                Main Thread
                     |
        +------------+------------+
        |            |            |
     Thread 1     Thread 2     Thread 3
        |            |            |
     Records      Records      Records
        |            |            |
        +------------+------------+
                     |
                Final Results
```

---

# Performance Comparison

The processing time is measured for each approach.

### Multiprocessing

| Processes | Time |
|----------:|-----:|
| 2 | — |
| 4 | — |
| 8 | — |
| 12 | — |
| 16 | — |
| 20 | — |

### Multithreading

| Threads | Time |
|--------:|-----:|
| 2 | — |
| 4 | — |
| 8 | — |
| 12 | — |
| 16 | — |
| 20 | — |

### Comparison

| Approach | Parallelism | Configurations |
|---|---|---|
| Naive | None | 1 process |
| Multiprocessing | Processes | 2, 4, 8, 12, 16, 20 |
| Multithreading | Threads | 2, 4, 8, 12, 16, 20 |

The measured times can be added to the tables after running the programs.

> **Note:** The processing time measured by the programs starts after the dataset has been loaded into memory. Therefore, the reported time represents the search/processing stage rather than the file-loading stage.

---

# Compilation

### Naive

```bash
gcc naive.c -o naive
```

### Multiprocessing

```bash
gcc multiprocessing.c -o multiprocessing
```

### Multithreading

```bash
gcc multithreading.c -o multithreading -pthread
```

---

# Running

```bash
./naive
```

```bash
./multiprocessing
```

```bash
./multithreading
```

The data directory can also be provided as a command-line argument:

```bash
./naive ./data
```

---

# Technologies

- C
- Linux
- POSIX Processes
- POSIX Threads
- `fork()`
- Pipes
- `waitpid()`
- `pthread_create()`
- `pthread_join()`
- Mutex
- Performance Measurement

---

# Requirements

- Linux operating system
- C compiler
- At least 4 CPU cores
- Dataset containing 1,000,000 records in 20 files

---

# Project Report

The project includes a report and discussion covering the implementation and performance comparison of:

- Naive processing
- Multiprocessing
- Multithreading

---
