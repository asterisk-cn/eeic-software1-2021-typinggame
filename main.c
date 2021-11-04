#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

//警告の回避のため
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

#define MAX_PROBLEM_NUM 4000
#define MAX_PROBLEM_SIZE 100

char problem[MAX_PROBLEM_NUM][MAX_PROBLEM_SIZE];
int loaded_problem_num;
int current_problem_iter;
int current_char_iter;
int current_char_state;
int score;
int problem_score;
int typed_char_cnt, miss_cnt;

#define DEFAULT_TPS 2.0
#define MAX_TPS 5.0
#define NORMAL_SIGMA 0.2

enum {
    CHAR_STATE_DEFAULT,
    CHAR_STATE_ACTIVE,
    CHAR_STATE_WRONG,
    CHAR_STATE_WHITE,
    CHAR_STATE_GREEN,
    CHAR_STATE_RED,
    CHAR_STATE_MAX
};

char char_state[CHAR_STATE_MAX][10] = {
    "\x1b[0m",       // CHAR_STATE_DEFAULT(default)
    "\x1b[1;7m",     // CHAR_STATE_ACTIVE(inverted)
    "\x1b[1;7;41m",  // CHAR_STATE_WRONG(red_inverted)
    "\x1b[1;97m",    // CHAR_STATE_WHITE(white)
    "\x1b[1;32m",    // CHAR_STATE_GREEN(green)
    "\x1b[1;31m",    // CHAR_STATE_RED(red)
};

#define TIME_LIMIT 60
#define PAUSE_TIME_LIMIT 15
struct timespec game_start, problem_start, current_time;
double spent_time;
int spent_time_ratio;
int time_penalty;
double time_offset;

void new_game();
int kbhit();
int compare_problem();
int get_problem_list();
double measure_spent_time(struct timespec, struct timespec);
double uniform();
double rand_normal(double, double);
int choose_problem();
int calc_score();
void print_problem();
void print_screen();
void print_pause_screen();
int pause_game();
void print_end_screen();
int end_game();

int main() {
    if (get_problem_list() == -1) return -1;
    new_game();

    while (1) {
        if (kbhit()) {
            char input = getchar();
            if (input == '\n') {
                int is_resume = pause_game();
                if (!is_resume) {
                    int is_continue = end_game();
                    if (is_continue) {
                        new_game();
                        continue;
                    }
                    return 0;
                }
            } else if (input == problem[current_problem_iter][current_char_iter]) {
                current_char_state = CHAR_STATE_ACTIVE;
                ++current_char_iter;
            } else {
                current_char_state = CHAR_STATE_WRONG;
                miss_cnt++;
                time_penalty++;
            }

            // DEBUG MODE
            // if (input == 'q') {
            //     current_char_iter = strlen(problem[current_problem_iter]);
            //     time_penalty--;
            // }

            // 点数の加算をワンテンポ遅らせる
            if (problem_score > 0) {
                score += problem_score;
                problem_score = 0;
            }

            // 最後まで入力されたとき
            if (strlen(problem[current_problem_iter]) == current_char_iter) {
                problem_score = calc_score();
                typed_char_cnt += strlen(problem[current_problem_iter]);
                current_char_state = CHAR_STATE_ACTIVE;
                current_problem_iter = choose_problem();
                current_char_iter = 0;
            }

            print_screen();
        }

        clock_gettime(CLOCK_REALTIME, &current_time);
        spent_time = measure_spent_time(game_start, current_time) - time_offset;

        // 時間表示の更新
        int current_time_ratio = (spent_time + time_penalty) / TIME_LIMIT * 31;
        if (spent_time_ratio != current_time_ratio) {
            spent_time_ratio = current_time_ratio;
            print_screen();
        }

        // 終了
        if ((spent_time + time_penalty) > TIME_LIMIT) {
            int is_continue = end_game();
            if (is_continue) {
                new_game();
                continue;
            }
            return 0;
        }
    }

    return 0;
}

void new_game() {
    srand((unsigned int)time(NULL));
    current_problem_iter = choose_problem();
    current_char_iter = 0;
    current_char_state = CHAR_STATE_ACTIVE;
    clock_gettime(CLOCK_REALTIME, &game_start);
    clock_gettime(CLOCK_REALTIME, &problem_start);
    score = 0;
    problem_score = 0;
    typed_char_cnt = 0;
    miss_cnt = 0;
    spent_time_ratio = 0;
    time_penalty = 0;
    time_offset = 0;
    print_screen();
}

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int compare_problem(const void *a, const void *b) {
    if (strlen(a) > strlen(b)) {
        return 1;
    } else if (strlen(a) < strlen(b)) {
        return -1;
    } else {
        return 0;
    }
}

int get_problem_list() {
    loaded_problem_num = 0;

    FILE *fp;
    char fname[] = "problem.txt";
    char str[MAX_PROBLEM_SIZE];

    fp = fopen(fname, "r");
    if (fp == NULL) {
        printf("%s file not open!\n", fname);
        return -1;
    }

    for (int i = 0; fgets(str, MAX_PROBLEM_SIZE, fp) != NULL; ++i) {
        strcpy(problem[i], str);
        problem[i][strlen(str) - 1] = '\0';
        loaded_problem_num++;
    }

    // 長さで昇順ソート
    qsort(problem, loaded_problem_num, MAX_PROBLEM_SIZE, compare_problem);

    fclose(fp);
    return 0;
}

double measure_spent_time(struct timespec start, struct timespec end) {
    double diff = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
    return fabs(diff);
}

double uniform(void) {
    return ((double)rand() + 1.0) / ((double)RAND_MAX + 2.0);
}

double rand_normal(double mu, double sigma) {
    double z = sqrt(-2.0 * log(uniform())) * sin(2.0 * M_PI * uniform());
    return mu + sigma * z;
}

int choose_problem() {
    double type_per_sec = DEFAULT_TPS;
    if (spent_time != 0) {
        type_per_sec = typed_char_cnt / spent_time;
        if (type_per_sec > MAX_TPS) type_per_sec = MAX_TPS;
    }
    double temp;
    while (1) {
        temp = rand_normal(type_per_sec / MAX_TPS, NORMAL_SIGMA);
        if ((0 <= temp && temp < 1)) break;
    }
    return (int)(temp * loaded_problem_num);
}

int calc_score() {
    double time = measure_spent_time(problem_start, current_time);
    return (int)(pow(strlen(problem[current_problem_iter]), 2) / sqrt(time));
    // 問題が長いときに点数が低くなりやすい問題を修正
}

void print_problem() {
    // printf("\t");
    for (int i = 0; i < strlen(problem[current_problem_iter]); ++i) {
        if (i < current_char_iter) {
            printf("%s%c", char_state[CHAR_STATE_GREEN], problem[current_problem_iter][i]);
        } else if (i == current_char_iter) {
            printf("%s%c", char_state[current_char_state], problem[current_problem_iter][current_char_iter]);
        } else {
            printf("%s%c", char_state[CHAR_STATE_WHITE], problem[current_problem_iter][i]);
        }
        printf("%s", char_state[CHAR_STATE_DEFAULT]);
    }
    printf("\n");
}

void print_screen() {
    system("clear");
    fprintf(stderr, "Start                         Finish\n");
    if (current_char_state == CHAR_STATE_WRONG) {
        fprintf(stderr, "+---------+---------+---------+\t%s-1.0sec\n", char_state[CHAR_STATE_RED]);
        printf("%s", char_state[CHAR_STATE_DEFAULT]);
    } else {
        fprintf(stderr, "+---------+---------+---------+\n");
    }
    for (int i = 0; i < 31; ++i) {
        if (i < spent_time_ratio) {
            printf("*");
        } else {
            printf(".");
        }
    }
    printf("\n\n\t\t\t");
    if (problem_score > 0 && current_char_iter == 0) {
        printf("Score \t%d %s+%d\n", score, char_state[CHAR_STATE_GREEN], problem_score);
        printf("%s", char_state[CHAR_STATE_DEFAULT]);
    } else {
        printf("Score \t%d\n", score);
    }
    printf("\n");
    for (int i = 0; i < strlen(problem[current_problem_iter]); ++i) {
        printf("*");
    }
    printf("\n\n");

    print_problem();

    printf("\n");
    for (int i = 0; i < strlen(problem[current_problem_iter]); ++i) {
        printf("*");
    }
    printf("\n");

    // DEBUG MODE
    // printf("\n%d/%d\n", current_problem_iter, loaded_problem_num);
}

void print_pause_screen() {
    system("clear");
    printf("%sPause!\n\n", char_state[CHAR_STATE_WHITE]);
    printf("%sPress %sEnter %sto Resume.\n", char_state[CHAR_STATE_DEFAULT], char_state[CHAR_STATE_GREEN], char_state[CHAR_STATE_DEFAULT]);
    printf("Press  '%sr%s'  to Reset.\n", char_state[CHAR_STATE_GREEN], char_state[CHAR_STATE_DEFAULT]);
}

int pause_game() {
    struct timespec pause_start;
    clock_gettime(CLOCK_REALTIME, &pause_start);
    while (1) {
        print_pause_screen();

        clock_gettime(CLOCK_REALTIME, &current_time);
        double time = measure_spent_time(pause_start, current_time);

        if (kbhit()) {
            char input = getchar();
            time_offset += time;
            if (input == '\n') {
                time_offset += time;
                return 1;
            }
            if (input == 'r') {
                time_offset += time;
                return 0;
            }
        }
        if (time > PAUSE_TIME_LIMIT) {
            time_offset += time;
            return 0;
        }
    }
}

void print_end_screen() {
    system("clear");
    printf("%sFinish!\n", char_state[CHAR_STATE_WHITE]);
    printf("%sYour Score is ...\n\n", char_state[CHAR_STATE_DEFAULT]);
    printf("%s%d points", char_state[CHAR_STATE_GREEN], score);
    printf("\t%s%d misses\n", char_state[CHAR_STATE_RED], miss_cnt);
    printf("%s%.2f keys/min\n\n", char_state[CHAR_STATE_WHITE], typed_char_cnt * 60 / spent_time);
    printf("%sPlay Again? [%sy%s/%sn%s]", char_state[CHAR_STATE_DEFAULT], char_state[CHAR_STATE_GREEN], char_state[CHAR_STATE_DEFAULT], char_state[CHAR_STATE_RED], char_state[CHAR_STATE_DEFAULT]);
}

int end_game() {
    score += problem_score;
    typed_char_cnt += current_char_iter;
    while (1) {
        print_end_screen();
        if (kbhit()) {
            system("clear");
            char input = getchar();
            if (input == 'y') return 1;
            if (input == 'n') return 0;
        }
    }
}