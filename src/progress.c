// Copyright 2020 Ben L. Titzer. All rights reserved.
// See LICENSE for details of Apache 2.0 license.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define RESET "\033[0m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define BUF_SIZE 4096
#define DEBUG 0

enum output_mode {
    INLINE,
    CHARACTER,
    LINES,
    SUMMARY
};

struct failure {
    char *name;
    char *error;
};

struct node {
    struct failure *val;
    struct node *next;
} *head, *tail;

int match_ok(char *, int, int);
char *str_slice(char *, int, int);
void process_line();
void report_start();
int report_finish();
void report_test_begin(char *);
void report_test_passed();
void report_test_failed(char *);

// Global state
static enum output_mode mode = CHARACTER;
static int indent = 0;
static int test_count = 0;
static int passed = 0;
static int failed = 0;
static char **current_test;
static char *line_buffer = NULL;
static int line_end = 0;
static size_t buf_size = 0;

// Used to implement clearing of the line by backing up one character at a time
int char_backup = 0;

// Used in implementing multiple pre-opened file descriptors (parallel mode)
struct input {
  FILE* file;
  char* current_test;
};
#define START_INPUT 3
#define MAX_INPUTS 128
struct input inputs[MAX_INPUTS];
int max_input = 1;
int parallel = 0;

void insert(struct failure *f) {
    struct node *node = malloc(sizeof(struct node));
    node->val = f;
    node->next = NULL;

    if (head == NULL) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }
}

#if DEBUG
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        int len = strlen(arg);
        for (int j = 0; j < len; j++) {
            switch (arg[j]) {
                case 'i':
                    mode = INLINE;
                    break;
                case 'c':
                    mode = CHARACTER;
                    break;
                case 'l':
                    mode = LINES;
                    break;
                case 's':
                    mode = SUMMARY;
                    break;
                case 't':
                    indent++;
                    break;
                case 'p':
         	    parallel = 1;
                    break;
	        default:
                    fprintf(stderr, "Usage: progress [iclpst]\n");
                    exit(EXIT_FAILURE);
            }
        }
    }

    if (parallel) {
      // Query all file descriptors between START_INPUT and MAX_INPUTS to see
      // if they are open and readable.
      for (int i = START_INPUT; i < MAX_INPUTS; i++) {
	int res = fcntl(i, F_GETFL) != -1 || errno != EBADF;
	if (res != 0) {
	  TRACE("fd %d = %d\n", i, res);
	  inputs[i].file = fdopen(i, "r");
	  inputs[i].current_test = NULL;
	  max_input = i + 1;
	}
      }
      current_test = NULL;
    } else {
      // Only read from stdin.
      inputs[0].file = stdin;
      inputs[0].current_test = NULL;
      current_test = &inputs[0].current_test;
    }

    if (mode == SUMMARY) indent = 0;

    report_start();

    if (parallel) {
      // Cycle through all inputs and read a line from each.
      int limit = max_input;
      while (limit > 0) {
	int next_limit = 0;
	for (int i = 0; i < limit; i++) {
	  if (inputs[i].file == NULL) continue; // input not open or finished
	  int read = getline(&line_buffer, &buf_size, inputs[i].file);
	  if (read > 0) {
	    current_test = &inputs[i].current_test;
	    line_end = strlen(line_buffer) - 1; // getline reads '\n' as well
	    process_line();
	    next_limit = i + 1;
	  } else {
	    inputs[i].file = NULL;
	  }
	}
	limit = next_limit;
      }
    } else {
      // Repeatedly read a line from stdin.
      while (getline(&line_buffer, &buf_size, stdin) > 0) {
        line_end = strlen(line_buffer) - 1; // getline reads '\n' as well
        process_line();
      }
    }
    return report_finish();
}

void process_line() {
    int pos = 3, end = line_end;
    if (line_end < pos)
        return;
    if (line_buffer[0] == '#' && line_buffer[1] == '#') {
        switch (line_buffer[2]) {
            case '+': ;
                // begin the next test
                char *test_name = str_slice(line_buffer, pos, end);
                report_test_begin(test_name);
                break;
            case '-': ;
                // end the current test
                int passed = match_ok(line_buffer, pos, end);
                if (passed) report_test_passed();
                else report_test_failed(str_slice(line_buffer, pos, end));
                break;
            case '>': ;
                // increase the total test count
                char *count_str = str_slice(line_buffer, pos, end);
                int count = strtol(count_str, NULL, 10);
                if (count > 0) test_count += count;
                free(count_str);
                break;
        }
    }
    line_end = 0;
}

void outc(char c) {
    printf("%c", c);
    char_backup++;
}

void outi(int i) {
    char_backup += printf("%d", i);
}

void outs(char *str) {
    char_backup += printf("%s", str);
}

void outindent() {
    for (int i = 0; i < indent; i++) printf(" ");
}

void outln() {
    printf("\n");
    outindent();
    char_backup = 0;
}

void outsp() {
    outc(' ');
}

void green(char *str) {
    printf(GREEN);
    outs(str);
    printf(RESET);
}

void red(char *str) {
    printf(RED);
    outs(str);
    printf(RESET);
}

void backup(int count) {
    while (count-- > 0) printf("\b \b");
}

void clearln() {
    backup(char_backup);
    char_backup = 0;
}

void output_failure(struct failure* f) {
    if (f->name != NULL) red(f->name);
    else red("<unknown>");
    outs(": ");
    if (f->error != NULL) outs(f->error);
    outln();
}

void output_count(int count) {
    outi(count);
    outs(" of ");
    int total = passed + failed;
    if (total < test_count)
        total = test_count;
    outi(total);
}

void output_passed_count() {
    output_count(passed);
    outsp();
    if (passed > 0) green("passed");
    else outs("passed");
    if (failed > 0) {
        outsp();
        printf(RED);
        outi(failed);
        outs(" failed");
        printf(RESET);
    }
}

void space() {
    int done = passed + failed;
    if (done % 10 == 0) {
        outc(' ');
    }
    if (done % 50 == 0) {
        output_count(done);
        outln();
    }
}

void report_start() {
    if (mode != INLINE) outindent();
    if (mode == SUMMARY) outs("##+\n");
}

void report_test_begin(char *name) {
    if (*current_test != NULL) report_test_failed("unterminated test case");
    *current_test = name;
    switch (mode) {
        case LINES:
            outs(name);
            outs("...");
            fflush(stdout);
            break;
        case INLINE:
            clearln();
            output_passed_count();
            outs(" | ");
            outs(name);
            fflush(stdout);
            break;
        default: ;
    }
}

void report_test_passed() {
    passed++;
    free(*current_test);
    *current_test = NULL;
    switch (mode) {
        case INLINE:
            clearln();
            output_passed_count();
            outs(" | ");
            fflush(stdout);
            break;
        case CHARACTER:
            green("o");
            space();
            fflush(stdout);
            break;
        case LINES:
            green("ok");
            outln();
            fflush(stdout);
            break;
        case SUMMARY: ;
    }
}

void report_test_failed(char *error) {
    failed++;

    struct failure *f = malloc(sizeof(struct failure));
    f->name = strdup(*current_test);
    f->error = error;
    insert(f);

    *current_test = NULL;
    switch (mode) {
        case INLINE:
            clearln();
            if (failed == 1) outln();
            output_failure(f);
            fflush(stdout);
            break;
        case CHARACTER:
            red("X");
            space();
            fflush(stdout);
            break;
        case LINES:
            red("failed");
            outln();
            fflush(stdout);
            break;
        case SUMMARY: ;
    }
}

int report_finish() {
  // Check all inputs for a current test.
  for (int i = 0; i < max_input; i++) {
    current_test = &inputs[i].current_test;
    if (*current_test != NULL) report_test_failed("abrupt output end");
  }

    int ok = (head == NULL) && (failed == 0);
    switch (mode) {
        case INLINE:
            clearln();
            output_passed_count();
            printf("\n");
            break;
        case SUMMARY:
	    if (ok) {
                outs("##-ok\n");
	    } else {
                outs("##-fail ");
                outi(failed);
                outs(" failed\n");
            }
            break;
        default:
            if (mode == CHARACTER) {
                int done = passed + failed;
                if (done % 50 != 0) {
                    if (done % 10 != 0) outsp();
                    output_count(done);
                    outln();
                }
            }
            struct node *current = head;
            while (current != NULL) {
                output_failure(current->val);
                current = current->next;
            }
            output_passed_count();
            printf("\n");
    }
    fflush(stdout);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

char *str_slice(char *arr, int start, int end) {
  return strndup(arr + start, end - start);
}

int match_ok(char *arr, int pos, int end) {
  int LEN = 2;
  if ((end - pos) < LEN) return 0;
  return strncmp("ok", arr + pos, LEN) == 0;
}
