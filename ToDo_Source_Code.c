#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_NAME "tasks.db"

/* ANSI Color Codes for UI Polish */
#define C_RED "\x1b[31m"
#define C_GRN "\x1b[32m"
#define C_YEL "\x1b[33m"
#define C_CYN "\x1b[36m"
#define C_RST "\x1b[0m"

typedef enum {
    TASK_PENDING = 0,
    TASK_COMPLETED = 1
} TaskStatus;

typedef struct {
    char description[150];
    char assignee[50];    /* NEW: For Boss vs Employee tracking */
    char resources[100];
    char assignedDate[11];
    char targetDate[11];
    char completedDate[11];
    TaskStatus status;
} Task;

/* Globals */
Task *tasks = NULL;
int taskCount = 0;
int capacity = 0;

/* Role Management Globals */
int isBoss = 0;
char currentUser[50] = "";

/* ---------- Memory ---------- */
void ensureCapacity() {
    if (taskCount >= capacity) {
        capacity = (capacity == 0) ? 10 : capacity * 2;
        tasks = realloc(tasks, capacity * sizeof(Task));
        if (!tasks) {
            printf(C_RED "Memory allocation failed\n" C_RST);
            exit(1);
        }
    }
}

/* ---------- Utility ---------- */
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void safeFgets(char *buffer, int size) {
    if (fgets(buffer, size, stdin)) {
        if (buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = '\0';
        else
            clearInputBuffer();
    }
}

void getTodayDate(char *buffer) {
    time_t t = time(NULL);
    strftime(buffer, 11, "%d-%m-%Y", localtime(&t));
}

time_t convertToTime(const char *date) {
    struct tm tm = {0};
    sscanf(date, "%2d-%2d-%4d", &tm.tm_mday, &tm.tm_mon, &tm.tm_year);
    tm.tm_mon -= 1;
    tm.tm_year -= 1900;
    tm.tm_hour = 0;
    return mktime(&tm);
}

/* ---------- Date Validation ---------- */
int isLeapYear(int y) {
    if (y % 400 == 0) return 1;
    if (y % 100 == 0) return 0;
    if (y % 4 == 0) return 1;
    return 0;
}

int isValidDate(const char *date) {
    int d, m, y;
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (sscanf(date, "%2d-%2d-%4d", &d, &m, &y) != 3) return 0;
    if (y < 1900 || m < 1 || m > 12) return 0;
    if (m == 2 && isLeapYear(y)) days[1] = 29;
    if (d < 1 || d > days[m - 1]) return 0;
    return 1;
}

int isPastDate(const char *date) {
    char today[11];
    getTodayDate(today);
    return difftime(convertToTime(date), convertToTime(today)) < 0;
}

/* ---------- File Handling ---------- */
void saveTasks() {
    FILE *fp = fopen(FILE_NAME, "w");
    if (!fp) {
        perror(C_RED "Save error" C_RST);
        exit(1);
    }

    for (int i = 0; i < taskCount; i++) {
        /* Added assignee to the format string */
        fprintf(fp, "%s|%s|%s|%s|%s|%s|%d\n",
                tasks[i].description,
                tasks[i].assignee,
                tasks[i].resources,
                tasks[i].assignedDate,
                tasks[i].targetDate,
                tasks[i].completedDate,
                tasks[i].status);
    }
    fclose(fp);
}

void loadTasks() {
    if (taskCount > 0) return; /* BUG FIX: Prevents duplicate loading */

    FILE *fp = fopen(FILE_NAME, "a+");
    if (!fp) {
        perror(C_RED "Open error" C_RST);
        exit(1);
    }

    rewind(fp);
    Task temp;

    /* Added assignee to the fscanf format string */
    while (fscanf(fp, "%149[^|]|%49[^|]|%99[^|]|%10[^|]|%10[^|]|%10[^|]|%d\n",
                  temp.description,
                  temp.assignee,
                  temp.resources,
                  temp.assignedDate,
                  temp.targetDate,
                  temp.completedDate,
                  (int *)&temp.status) == 7) 
    {
        ensureCapacity();
        tasks[taskCount++] = temp;
    }
    fclose(fp);
}

/* ---------- Reminder System ---------- */
void checkReminders() {
    char today[11];
    getTodayDate(today);

    printf(C_CYN "\n================ REMINDERS ================\n" C_RST);

    int reminderDays[] = {14, 10, 7, 4, 3, 2, 1, 0};
    int printed = 0;

    for (int i = 0; i < taskCount; i++) {
        /* Employee restriction: Only see own reminders */
        if (!isBoss && strcmp(tasks[i].assignee, currentUser) != 0) continue;

        if (tasks[i].status == TASK_COMPLETED) continue;

        if (difftime(convertToTime(today), convertToTime(tasks[i].assignedDate)) < 0) continue;

        int days = (int)(difftime(convertToTime(tasks[i].targetDate), convertToTime(today)) / (60 * 60 * 24));

        if (days < 0) {
            printf(C_RED "TASK OVERDUE:" C_RST " %s (Due %d day(s) ago)\n", tasks[i].description, -days);
            printed = 1;
        } else {
            for (int j = 0; j < 8; j++) {
                if (days == reminderDays[j]) {
                    if (days == 0)
                        printf(C_YEL " DUE TODAY:" C_RST " %s\n", tasks[i].description);
                    else
                        printf(" Reminder: %s (Due in %d day(s))\n", tasks[i].description, days);
                    printed = 1;
                }
            }
        }
    }

    if (!printed) printf("No pending reminders.\n");
    printf(C_CYN "===========================================\n\n" C_RST);
}

/* ---------- Display ---------- */
void printTasks(Task *list) {
    /* Progress Dashboard Calculation */
    int total = 0, comp = 0, pend = 0;
    for (int i = 0; i < taskCount; i++) {
        if (!isBoss && strcmp(list[i].assignee, currentUser) != 0) continue;
        total++;
        if (list[i].status == TASK_COMPLETED) comp++; else pend++;
    }
    
    printf(C_CYN "\n--- DASHBOARD [ %s ] ---\n" C_RST, isBoss ? "BOSS VIEW" : currentUser);
    printf("Total Tasks: %d | " C_GRN "Completed: %d" C_RST " | " C_YEL "Pending: %d\n" C_RST, total, comp, pend);

    /* Table Header updated for Assignee */
    printf("+----+------------+------------+------------+---------------+-----------------------------------+\n");
    printf("| %-2s | %-10s | %-10s | %-10s | %-13s | %-33s |\n",
           "No", "Assigned", "Target", "Status", "Assignee", "Description");
    printf("+----+------------+------------+------------+---------------+-----------------------------------+\n");

    for (int i = 0; i < taskCount; i++) {
        /* Filter out tasks that don't belong to the employee */
        if (!isBoss && strcmp(list[i].assignee, currentUser) != 0) continue;

        printf("| %-2d | %-10s | %-10s | %-10s | %-13.13s | %-33.33s |\n",
               i + 1,
               list[i].assignedDate,
               list[i].targetDate,
               list[i].status == TASK_COMPLETED ? "Completed" : "Pending",
               list[i].assignee,
               list[i].description);
    }
    printf("+----+------------+------------+------------+---------------+-----------------------------------+\n");
}

/* ---------- Sorting ---------- */
int compareDue(const void *a, const void *b) {
    return difftime(convertToTime(((Task *)a)->targetDate), convertToTime(((Task *)b)->targetDate));
}

int compareStatus(const void *a, const void *b) {
    Task *t1 = (Task *)a;
    Task *t2 = (Task *)b;
    if (t1->status != t2->status) return t1->status - t2->status;
    return 0;
}

/* ---------- Core ---------- */
void addTask() {
    /* Role restriction */
    if (!isBoss) {
        printf(C_RED "Access Denied: Only the Boss can assign tasks.\n" C_RST);
        return;
    }

    ensureCapacity();
    Task *t = &tasks[taskCount];
    t->status = TASK_PENDING;
    strcpy(t->completedDate, "-");

    /* Buffer for safe inputs */
    char inputBuffer[10]; 
    int option;
    char today[11];
    getTodayDate(today);

    printf("1. Use current date\n2. Schedule for later\nChoice: ");
    safeFgets(inputBuffer, 10);
    option = atoi(inputBuffer); /* BUG FIX: Prevents infinite loop on char input */

    if (option == 1) {
        strcpy(t->assignedDate, today);
    } else {
        while (1) {
            printf("Enter activation date (DD-MM-YYYY): ");
            safeFgets(t->assignedDate, 11);
            if (!isValidDate(t->assignedDate)) { printf(C_RED "Invalid date.\n" C_RST); continue; }
            if (isPastDate(t->assignedDate)) { printf(C_RED "Past date not allowed.\n" C_RST); continue; }
            break;
        }
    }

    printf("Assign to (Employee Name): ");
    safeFgets(t->assignee, 50);

    printf("Task Description: ");
    safeFgets(t->description, 150);

    printf("Resources Required: ");
    safeFgets(t->resources, 100);

    while (1) {
        printf("Due Date (DD-MM-YYYY): ");
        safeFgets(t->targetDate, 11);

        if (!isValidDate(t->targetDate)) { printf(C_RED "Invalid date.\n" C_RST); continue; }
        if (difftime(convertToTime(t->targetDate), convertToTime(t->assignedDate)) < 0) {
            printf(C_RED "Due date must be after activation.\n" C_RST);
            continue;
        }
        break;
    }

    taskCount++;
    saveTasks();
    printf(C_GRN "Task added successfully.\n" C_RST);
}

void viewTasks() {
    if (taskCount == 0) { printf("No tasks available.\n"); return; }

    if (taskCount == 1) { printTasks(tasks); return; }

    char inputBuffer[10];
    int option;
    printf("1. Unsorted\n2. Sorted\nChoice: ");
    safeFgets(inputBuffer, 10);
    option = atoi(inputBuffer);

    Task *displayList = malloc(taskCount * sizeof(Task));
    memcpy(displayList, tasks, taskCount * sizeof(Task));

    if (option == 2) {
        int type;
        printf("1. Assigned Order\n2. Due Date\n3. Status\nChoice: ");
        safeFgets(inputBuffer, 10);
        type = atoi(inputBuffer);

        if (type == 2)
            qsort(displayList, taskCount, sizeof(Task), compareDue);
        else if (type == 3)
            qsort(displayList, taskCount, sizeof(Task), compareStatus);
    }

    printTasks(displayList);
    free(displayList);
}

void markTaskCompleted() {
    if (taskCount == 0) { printf("No tasks available.\n"); return; }

    viewTasks();
    
    char inputBuffer[10];
    int s;
    printf("Enter Serial Number to mark completed: ");
    safeFgets(inputBuffer, 10);
    s = atoi(inputBuffer);

    if (s < 1 || s > taskCount) { printf(C_RED "Invalid serial number.\n" C_RST); return; }

    /* Security Check: Employee can only complete their own assigned work */
    if (!isBoss && strcmp(tasks[s - 1].assignee, currentUser) != 0) {
        printf(C_RED "Access Denied: You can only complete your own tasks.\n" C_RST);
        return;
    }

    if (tasks[s - 1].status == TASK_COMPLETED) {
        printf(C_YEL "Task is already marked as completed.\n" C_RST);
        return;
    }

    tasks[s - 1].status = TASK_COMPLETED;
    getTodayDate(tasks[s - 1].completedDate);
    saveTasks();
    printf(C_GRN "Task marked completed.\n" C_RST);
}

void removeTask() {
    /* Role restriction */
    if (!isBoss) {
        printf(C_RED "Access Denied: Only the Boss can delete tasks.\n" C_RST);
        return;
    }

    if (taskCount == 0) { printf("No tasks available.\n"); return; }

    viewTasks();
    char inputBuffer[10];
    int s;
    printf("Enter Serial Number to remove: ");
    safeFgets(inputBuffer, 10);
    s = atoi(inputBuffer);

    if (s < 1 || s > taskCount) { printf(C_RED "Invalid serial number.\n" C_RST); return; }

    for (int i = s - 1; i < taskCount - 1; i++)
        tasks[i] = tasks[i + 1];

    taskCount--;
    saveTasks();
    printf(C_GRN "Task removed.\n" C_RST);
}

/* ---------- MAIN ---------- */
int main() {
    /* Step 1: Login System */
    char inputBuffer[10];
    printf("Login as:\n1. Boss\n2. Employee\nChoice: ");
    safeFgets(inputBuffer, 10);
    int loginChoice = atoi(inputBuffer);

    if (loginChoice == 1) {
        isBoss = 1;
        printf(C_GRN "Logged in as Boss.\n" C_RST);
    } else {
        isBoss = 0;
        printf("Enter your exact Employee Name: ");
        safeFgets(currentUser, 50);
        printf(C_GRN "Logged in as Employee: %s\n" C_RST, currentUser);
    }

    loadTasks();
    checkReminders();

    int choice;

    while (1) {
        printf(C_CYN "\n===== TASK OS =====\n" C_RST);
        if (isBoss) printf("1. Add Task\n");
        printf("2. View Tasks & Dashboard\n");
        printf("3. Mark Completed\n");
        if (isBoss) printf("4. Remove Task\n");
        printf("5. Exit\n");
        printf("Choice: ");

        /* BUG FIX: Safe input to prevent infinite looping on character inputs */
        safeFgets(inputBuffer, 10);
        choice = atoi(inputBuffer);

        switch (choice) {
            case 1: addTask(); break;
            case 2: viewTasks(); break;
            case 3: markTaskCompleted(); break;
            case 4: removeTask(); break;
            case 5:
                saveTasks();
                free(tasks);
                printf("Saving and exiting...\n");
                return 0;
            default:
                /* BUG FIX: Handles invalid numbers like 9 */
                printf(C_RED "Invalid choice. Please pick 1-5.\n" C_RST); 
                break; 
        }
    }
}
