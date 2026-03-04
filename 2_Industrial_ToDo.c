#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_NAME "tasks.db"

typedef enum {
    TASK_PENDING = 0,
    TASK_COMPLETED = 1
} TaskStatus;

typedef struct {
    char description[200];
    char resources[200];
    char assignedDate[11];
    char targetDate[11];
    char completedDate[11];
    TaskStatus status;
} Task;

Task *tasks = NULL;
int taskCount = 0;
int capacity = 0;

/* ---------- Memory ---------- */
void ensureCapacity() {
    if (taskCount >= capacity) {
        capacity = (capacity == 0) ? 10 : capacity * 2;
        tasks = realloc(tasks, capacity * sizeof(Task));
        if (!tasks) {
            printf("Memory allocation failed\n");
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
    sscanf(date, "%2d-%2d-%4d",
           &tm.tm_mday, &tm.tm_mon, &tm.tm_year);
    tm.tm_mon -= 1;
    tm.tm_year -= 1900;
    tm.tm_hour = 0;
    return mktime(&tm);
}

/* ---------- Date Validation ---------- */
int isLeapYear(int y){
    if(y%400==0) return 1;
    if(y%100==0) return 0;
    if(y%4==0) return 1;
    return 0;
}

int isValidDate(const char *date){
    int d,m,y;
    int days[]={31,28,31,30,31,30,31,31,30,31,30,31};

    if(sscanf(date,"%2d-%2d-%4d",&d,&m,&y)!=3) return 0;
    if(y<1900||m<1||m>12) return 0;
    if(m==2&&isLeapYear(y)) days[1]=29;
    if(d<1||d>days[m-1]) return 0;
    return 1;
}

int isPastDate(const char *date){
    char today[11];
    getTodayDate(today);
    return difftime(convertToTime(date),convertToTime(today))<0;
}

/* ---------- File Handling ---------- */
void saveTasks(){
    FILE *fp=fopen(FILE_NAME,"w");
    if(!fp){ perror("Save error"); exit(1); }

    for(int i=0;i<taskCount;i++){
        fprintf(fp,"%s|%s|%s|%s|%s|%d\n",
                tasks[i].description,
                tasks[i].resources,
                tasks[i].assignedDate,
                tasks[i].targetDate,
                tasks[i].completedDate,
                tasks[i].status);
    }
    fclose(fp);
}

void loadTasks(){
    FILE *fp=fopen(FILE_NAME,"a+");
    if(!fp){ perror("Open error"); exit(1); }

    rewind(fp);
    Task temp;

    while(fscanf(fp,"%199[^|]|%199[^|]|%10[^|]|%10[^|]|%10[^|]|%d\n",
                 temp.description,
                 temp.resources,
                 temp.assignedDate,
                 temp.targetDate,
                 temp.completedDate,
                 (int*)&temp.status)==6){
        ensureCapacity();
        tasks[taskCount++]=temp;
    }
    fclose(fp);
}

/* ---------- Reminder System ---------- */
void checkReminders(){
    char today[11];
    getTodayDate(today);

    printf("\n================ REMINDERS ================\n");

    int reminderDays[] = {14,10,7,4,3,2,1,0};
    int printed = 0;

    for(int i=0;i<taskCount;i++){

        if(tasks[i].status==TASK_COMPLETED)
            continue;

        if(difftime(convertToTime(today),
                    convertToTime(tasks[i].assignedDate)) < 0)
            continue;

        int days=(int)(difftime(
            convertToTime(tasks[i].targetDate),
            convertToTime(today))/(60*60*24));

        if(days < 0){
            printf("⚠ OVERDUE: %s (Due %d day(s) ago)\n",
                   tasks[i].description, -days);
            printed = 1;
        }
        else{
            for(int j=0;j<8;j++){
                if(days == reminderDays[j]){
                    if(days == 0)
                        printf(" DUE TODAY: %s\n", tasks[i].description);
                    else
                        printf(" Reminder: %s (Due in %d day(s))\n",
                               tasks[i].description, days);
                    printed = 1;
                }
            }
        }
    }

    if(!printed)
        printf("No pending reminders.\n");

    printf("===========================================\n\n");
}

/* ---------- Display ---------- */
void printTasks(Task *list){

    printf("+----+------------+------------+------------+-------------------------+-----------------------------------+\n");
    printf("| %-2s | %-10s | %-10s | %-10s | %-23s | %-33s |\n",
           "S.No","Assigned","Target","Status","Resources","Description");
    printf("+----+------------+------------+------------+-------------------------+-----------------------------------+\n");

    for(int i=0;i<taskCount;i++){
        printf("| %-2d | %-10s | %-10s | %-10s | %-23.23s | %-33.33s |\n",
               i+1,
               list[i].assignedDate,
               list[i].targetDate,
               list[i].status==TASK_COMPLETED?"Completed":"Pending",
               list[i].resources,
               list[i].description);
    }

    printf("+----+------------+------------+------------+-------------------------+-----------------------------------+\n");
}

/* ---------- Sorting ---------- */
int compareDue(const void *a,const void *b){
    return difftime(
        convertToTime(((Task*)a)->targetDate),
        convertToTime(((Task*)b)->targetDate));
}

int compareStatus(const void *a,const void *b){
    Task *t1=(Task*)a;
    Task *t2=(Task*)b;
    if(t1->status!=t2->status)
        return t1->status - t2->status;
    return 0;
}

/* ---------- Core ---------- */
void addTask(){
    ensureCapacity();

    Task *t=&tasks[taskCount];
    t->status=TASK_PENDING;
    strcpy(t->completedDate,"-");

    int option;
    char today[11];
    getTodayDate(today);

    printf("1. Use current date\n2. Schedule for later\nChoice: ");
    scanf("%d",&option);
    clearInputBuffer();

    if(option==1){
        strcpy(t->assignedDate,today);
    }else{
        while(1){
            printf("Enter activation date (DD-MM-YYYY): ");
            safeFgets(t->assignedDate,11);
            if(!isValidDate(t->assignedDate)){ printf("Invalid date.\n"); continue; }
            if(isPastDate(t->assignedDate)){ printf("Past date not allowed.\n"); continue; }
            break;
        }
    }

    printf("Task Description: ");
    safeFgets(t->description,200);

    printf("Resources Required: ");
    safeFgets(t->resources,200);

    while(1){
        printf("Due Date (DD-MM-YYYY): ");
        safeFgets(t->targetDate,11);

        if(!isValidDate(t->targetDate)){ printf("Invalid date.\n"); continue; }
        if(difftime(convertToTime(t->targetDate),
                    convertToTime(t->assignedDate))<0){
            printf("Due date must be after activation.\n");
            continue;
        }
        break;
    }

    taskCount++;
    saveTasks();
    printf("Task added successfully.\n");
}

void viewTasks(){
    if(taskCount==0){ printf("No tasks available.\n"); return; }

    if(taskCount==1){ printTasks(tasks); return; }

    int option;
    printf("1. Unsorted\n2. Sorted\nChoice: ");
    scanf("%d",&option);
    clearInputBuffer();

    Task *displayList=malloc(taskCount*sizeof(Task));
    memcpy(displayList,tasks,taskCount*sizeof(Task));

    if(option==2){
        int type;
        printf("1. Assigned Order\n2. Due Date\n3. Status\nChoice: ");
        scanf("%d",&type);
        clearInputBuffer();

        if(type==2)
            qsort(displayList,taskCount,sizeof(Task),compareDue);
        else if(type==3)
            qsort(displayList,taskCount,sizeof(Task),compareStatus);
    }

    printTasks(displayList);
    free(displayList);
}

void markTaskCompleted(){
    if(taskCount==0){ printf("No tasks available.\n"); return; }

    viewTasks();
    int s;
    printf("Enter Serial Number to mark completed: ");
    scanf("%d",&s);
    clearInputBuffer();

    if(s<1||s>taskCount){ printf("Invalid serial number.\n"); return; }

    if(tasks[s-1].status==TASK_COMPLETED){
        printf("Already completed.\n");
        return;
    }

    tasks[s-1].status=TASK_COMPLETED;
    getTodayDate(tasks[s-1].completedDate);
    saveTasks();
    printf("Task marked completed.\n");
}

void removeTask(){
    if(taskCount==0){ printf("No tasks available.\n"); return; }

    viewTasks();
    int s;
    printf("Enter Serial Number to remove: ");
    scanf("%d",&s);
    clearInputBuffer();

    if(s<1||s>taskCount){ printf("Invalid serial number.\n"); return; }

    for(int i=s-1;i<taskCount-1;i++)
        tasks[i]=tasks[i+1];

    taskCount--;
    saveTasks();
    printf("Task removed.\n");
}

/* ---------- MAIN ---------- */
int main(){

    loadTasks();
    checkReminders();

    int choice;

    while(1){
        printf("\n===== TO DO LIST =====\n");
        printf("1. Add Task\n");
        printf("2. View Tasks\n");
        printf("3. Mark Completed\n");
        printf("4. Remove Task\n");
        printf("5. Exit\n");
        printf("Choice: ");

        scanf("%d",&choice);
        clearInputBuffer();

        switch(choice){
            case 1: addTask(); break;
            case 2: viewTasks(); break;
            case 3: markTaskCompleted(); break;
            case 4: removeTask(); break;
            case 5:
                saveTasks();
                free(tasks);
                return 0;
        }
    }
}
