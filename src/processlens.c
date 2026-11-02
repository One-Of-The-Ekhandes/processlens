/*
 * ProcessLens: inspect Linux processes straight from /proc.
 *
 *   processlens <pid>       state, resident memory (VmRSS) and thread count for one process
 *   processlens --top <n>   the n processes using the most resident memory
 *
 * Everything comes from /proc/<pid>/status, a text file the kernel generates on each read.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    long pid;
    char name[64];
    char state[32];   /* e.g. "S (sleeping)" */
    long rss_kb;      /* VmRSS; 0 for kernel threads, which have no user memory */
    long threads;
} ProcInfo;

/* Parse a strictly positive integer; returns -1 if the text is not one. */
static long parse_positive(const char *s) {
    if (s == NULL || *s == '\0') return -1;
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || v <= 0) return -1;
    return v;
}

/* Read /proc/<pid>/status into info. Returns 0 on success, -1 if the process is gone or unreadable. */
static int read_status(long pid, ProcInfo *info) {
    char path[64];
    snprintf(path, sizeof path, "/proc/%ld/status", pid);
    FILE *f = fopen(path, "r");
    if (f == NULL) return -1;

    memset(info, 0, sizeof *info);
    info->pid = pid;
    char line[256];
    while (fgets(line, sizeof line, f) != NULL) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line + 5, " %63[^\n]", info->name);
        } else if (strncmp(line, "State:", 6) == 0) {
            sscanf(line + 6, " %31[^\n]", info->state);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, " %ld", &info->rss_kb);
        } else if (strncmp(line, "Threads:", 8) == 0) {
            sscanf(line + 8, " %ld", &info->threads);
        }
    }
    fclose(f);
    return 0;
}

static int is_numeric(const char *s) {
    if (*s == '\0') return 0;
    for (; *s; s++) {
        if (!isdigit((unsigned char)*s)) return 0;
    }
    return 1;
}

static int by_rss_desc(const void *a, const void *b) {
    const ProcInfo *x = a, *y = b;
    if (x->rss_kb != y->rss_kb) return (y->rss_kb > x->rss_kb) - (y->rss_kb < x->rss_kb);
    return (x->pid > y->pid) - (x->pid < y->pid);
}

static int show_one(long pid) {
    ProcInfo p;
    if (read_status(pid, &p) != 0) {
        fprintf(stderr, "processlens: no such process %ld (or permission denied)\n", pid);
        return 1;
    }
    printf("PID:     %ld\n", p.pid);
    printf("Name:    %s\n", p.name);
    printf("State:   %s\n", p.state);
    printf("VmRSS:   %ld kB\n", p.rss_kb);
    printf("Threads: %ld\n", p.threads);
    return 0;
}

static int show_top(long n) {
    DIR *dir = opendir("/proc");
    if (dir == NULL) {
        perror("processlens: opendir /proc");
        return 1;
    }
    size_t cap = 256, count = 0;
    ProcInfo *list = malloc(cap * sizeof *list);
    if (list == NULL) {
        closedir(dir);
        fprintf(stderr, "processlens: out of memory\n");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!is_numeric(entry->d_name)) continue;
        if (count == cap) {
            cap *= 2;
            ProcInfo *grown = realloc(list, cap * sizeof *list);
            if (grown == NULL) {
                free(list);
                closedir(dir);
                fprintf(stderr, "processlens: out of memory\n");
                return 1;
            }
            list = grown;
        }
        /* A process can exit between readdir and fopen; just skip it. */
        if (read_status(strtol(entry->d_name, NULL, 10), &list[count]) == 0) count++;
    }
    closedir(dir);

    qsort(list, count, sizeof *list, by_rss_desc);
    printf("%-8s %-20s %-16s %12s %8s\n", "PID", "NAME", "STATE", "VMRSS_KB", "THREADS");
    for (size_t i = 0; i < count && i < (size_t)n; i++) {
        printf("%-8ld %-20.20s %-16.16s %12ld %8ld\n",
               list[i].pid, list[i].name, list[i].state, list[i].rss_kb, list[i].threads);
    }
    free(list);
    return 0;
}

static void usage(void) {
    fprintf(stderr, "usage: processlens <pid>\n       processlens --top <n>\n");
}

int main(int argc, char **argv) {
    if (argc == 2) {
        long pid = parse_positive(argv[1]);
        if (pid < 0) {
            usage();
            return 2;
        }
        return show_one(pid);
    }
    if (argc == 3 && strcmp(argv[1], "--top") == 0) {
        long n = parse_positive(argv[2]);
        if (n < 0) {
            usage();
            return 2;
        }
        return show_top(n);
    }
    usage();
    return 2;
}
