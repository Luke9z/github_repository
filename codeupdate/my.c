#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

//  你可能需要其他标准头文件

//  CITS2002 2023年第1个项目
//  学生1：STUDENT-NUMBER1 NAME-1
//  学生2：STUDENT-NUMBER2 NAME-2

//  myscheduler (v1.0)
//  使用以下命令编译：cc -std=c11 -Wall -Werror -o myscheduler myscheduler.c

// cc -std=c11 -Wall -Werror -o my my.c

// ./my sysconfig-file.txt command-file.txt


//  这些常量定义了您的程序需要支持的sysconfig和command的最大大小。
//  定义任何所需数据结构的最大尺寸时，您需要这些常量。

#define MAX_DEVICES                     4
#define MAX_DEVICE_NAME                 20
#define MAX_COMMANDS                    10
#define MAX_COMMAND_NAME                20
#define MAX_SYSCALLS_PER_PROCESS        40
#define MAX_RUNNING_PROCESSES           50

//  注意，设备的数据传输率以字节/秒为单位，
//  所有时间都以微秒（usecs）为单位，
//  总的进程完成时间不会超过2000秒
//  (因此你可以安全地使用"标准"的32位整数来存储时间)。

#define DEFAULT_TIME_QUANTUM            100

#define TIME_CONTEXT_SWITCH             5
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_ACQUIRE_BUS                20

//  ----------------------------------------------------------------------

#define CHAR_COMMENT                    '#'
#define BOOL int
#define TRUE 1
#define FALSE 0

// int globalArray[5] = {1, 2, 3, 4, 5};

typedef struct {
    char deviceName[50];  // 设备名称
    long long int inputRate;  // 输入速率 readspeed
    long long int outputRate; // 输出速率 writespeed
} DeviceInfo;

//在全局定义一个结构体数组 devices 和一个整数 deviceCount 用于记录当前已存储的设备数量：
DeviceInfo devices[MAX_DEVICES]; 
int deviceCount = 0;

//定义时间量子
int timequantum = 0;

/// @brief /////

typedef struct {
    long long time;  // 时间
    char command[MAX_COMMAND_NAME]; // 命令
    char deviceName[MAX_DEVICE_NAME];  // 参数1
    char arg2[MAX_DEVICE_NAME];  // 参数2 记录子程序的名称
    long long dataSize;  // 如果有的话，数据的大小
    long long Spendingtime;  // 用于给sleep的时间, 默认是0
} CommandInfo;

// 定义进程状态的枚举类型
typedef enum {
    RUNNING,
    READY,
    BLOCKED,
    WAITING,
    EXIT
} ProcessState;

typedef struct ProgramInfo ProgramInfo;  // 首先前向声明ProgramInfo

struct ProgramInfo {
    char programName[MAX_DEVICE_NAME];  // 程序名称
    CommandInfo commands[MAX_SYSCALLS_PER_PROCESS];  // 该程序的命令列表
    int commandCount;  // 当前程序的命令数量
    //char name[50];                  // 进程名称
    struct ProgramInfo *children[MAX_SYSCALLS_PER_PROCESS];          // 子进程（假设每个进程只有一个子进程，如果可以有多个子进程，可以使用链表）
    int totalRunTime;               // 总运行时间
    int elapsedRunTime;             // 已运行的时间
    ProcessState state;             // 进程的当前状态
    int pid;                        // pid 序号
    int inlist;                     // 程序是否进入了list里
};

ProgramInfo programs[MAX_RUNNING_PROCESSES];  // 全局的程序数组
int programCount = 0;  // 当前已存储的程序数量

typedef struct {
    int pid;
    long long int rate;
    long long dataSize;
} BlockQueueExitResult;

//execute
//定义一个总时间戳
int total_time=0;
int running_timequantum = 0;

/// @brief //////
void read_sysconfig(char argv0[], char filename[])
{

    FILE *file = fopen(filename, "r");
    if(!file) {
        printf("Error opening file: %s\n", filename);
        perror("Error opening file dabukai");
        exit(EXIT_FAILURE); 
    }
   
    // DeviceInfo device;

    char   line[BUFSIZ];

    while( fgets(line, sizeof line, file) != NULL ) {  

        // 跳过以 # 开头的注释行
        if(line[0] == CHAR_COMMENT) {
        continue;
        }
        // 定义临时变量存储解析结果
        char deviceKeyword[50];
        char tempDeviceName[50];
        long long tempInputRate;
        long long tempOutputRate;
        if(strncmp(line, "timequantum", 11) == 0) {
        // 提取"timequantum"后面的数字部分
        sscanf(line + 11, "%dusec", &timequantum);
        // 输出提取的值
        printf("Extracted value: %d\n", timequantum);
        }  
        running_timequantum = timequantum;

        // 使用 sscanf 解析行
        if(sscanf(line, "%s %s %lldBps %lldBps", 
            deviceKeyword, 
            tempDeviceName, 
            &tempInputRate, 
            &tempOutputRate) == 4) {
            // 检查第一个解析到的字符串是否为 "device"
            if(strcmp(deviceKeyword, "device") == 0) {
                if(deviceCount < MAX_DEVICES) {
                // 存储到全局的设备信息结构体数组中
                strncpy(devices[deviceCount].deviceName, tempDeviceName, sizeof(devices[deviceCount].deviceName) - 1);
                devices[deviceCount].inputRate = tempInputRate;
                devices[deviceCount].outputRate = tempOutputRate;
                deviceCount++;
                //输出检查结果
                printf("%s\t%lldBps\t%lldBps\n", 
                devices[deviceCount-1].deviceName, 
                devices[deviceCount-1].inputRate, 
                devices[deviceCount-1].outputRate);
                } else {
                // 行格式不正确或不符合预期
                printf("Failed to parse line: %s", line);
                }   
            }
        }
    }
    fclose(file);
}

void read_commands(char argv0[], char filename[])
{

    FILE *file = fopen(filename, "r");
    if(!file) {
        printf("Error opening file: %s\n", filename);
        perror("Error opening file dabukai");
        exit(EXIT_FAILURE); 
    }

    printf("成功打开\n");
    char   line[BUFSIZ];
    BOOL   readingProgram = FALSE; // 标记是否正在读取某个程序

    // 尝试读取命令
            long long tempTime =0 ;
            char tempCommand[MAX_COMMAND_NAME]="";
            char tempDeviceName[MAX_DEVICE_NAME]="";
            char tempArg2[MAX_DEVICE_NAME]="";
            long long tempDataSize = 0;
            long long tempSpendingtime = 0;
            int tokenCount = 0;
            char* tokens[4]; //最多4个参数

    while(fgets(line, sizeof line, file) != NULL) {
        // 尝试读取命令
            tempTime = 0;
            tempCommand[0] = '\0';
            tempDeviceName[0]= '\0' ;
            tempArg2[0]= '\0' ;
            tempDataSize = 0;
            tempSpendingtime = 0;
            // char* tokens[4]; //最多4个参数
            tokenCount = 0;
            for (int i = 0; i < 4; i++) {
              tokens[i] = NULL;
            }   

    
        // 跳过以 # 开头的注释行
        if(line[0] == CHAR_COMMENT) {
            readingProgram = TRUE;
            printf("有一行注释符\n");
            continue;
        }

        if(readingProgram) {
            // 尝试读取程序名称
            char tempProgramName[MAX_DEVICE_NAME];
            if(sscanf(line, "%s", tempProgramName) == 1) {
                // 保存到程序数组
                strncpy(programs[programCount].programName, tempProgramName, sizeof(programs[programCount].programName) - 1);
                programs[programCount].commandCount = 0;
                readingProgram = FALSE;
                printf("程序名称:%s\n",tempProgramName);
            }
        } else {

            char* token = strtok(line, " \t"); // 使用空格或制表符作为分隔符
            while(token != NULL) {
                char* pos;
                if ((pos = strchr(token, '\n')) != NULL) {
                    *pos = '\0';
                }
                char* pos2;
                if ((pos2 = strchr(token, '\r')) != NULL) {
                    *pos2 = '\0';
                }
                if(tokenCount < 4) {
                    tokens[tokenCount++] = token;
                    printf("token是：%s \n",token);
                }
                token = strtok(NULL, " \t");
            }
            printf("tokencount是：%d\n", tokenCount);
           

            if(tokenCount == 3 && strcmp(tokens[1], "sleep") == 0) {
                //token 1 的赋值
                printf("正确：行'%s'的格式正确\n", line);
                if(sscanf(tokens[0], "%lldusecs", &tempTime) != 1) {
                    printf("Error parsing time from: %s\n", tokens[0]);
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("success parsing time from: %s\n", tokens[0]);
                }
                //token 2 的赋值
                strncpy(tempCommand, tokens[1], sizeof(tempCommand) - 1);
                tempCommand[sizeof(tempCommand) - 1] = '\0'; // 确保字符串以null结尾
                printf("tempCommand是%s\n",tempCommand);
                //token 3 的赋值
                if(sscanf(tokens[2], "%lldusecs", &tempSpendingtime) != 1) {
                    printf("Error parsing data size from: %s\n", tokens[2]);
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("success parsing time from: %s\n", tokens[2]);
                }

            } else if (tokenCount == 2) {
                //  && (strcmp(tokens[1], "wait") == 0 || strcmp(tokens[1], "exit") == 0)
                //token 1 的赋值
                printf("正确：行'%s'的格式正确\n", line);
                if(sscanf(tokens[0], "%lldusecs", &tempTime) != 1) {
                    printf("Error parsing time from: %s\n", tokens[0]);
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("success parsing time from: %s\n", tokens[0]);
                }
                //token 2 的赋值
                strncpy(tempCommand, tokens[1], sizeof(tempCommand) - 1);
                tempCommand[sizeof(tempCommand) - 1] = '\0'; // 确保字符串以null结尾
                printf("tempCommand是%s\n",tempCommand);
            } else if (tokenCount == 4 && (strcmp(tokens[1], "read") == 0 || strcmp(tokens[1], "write") == 0)) {
                printf("正确：行'%s'的格式正确\n", line);
                 //token 1 的赋值
                if(sscanf(tokens[0], "%lldusecs", &tempTime) != 1) {
                    printf("Error parsing time from: %s\n", tokens[0]);
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("success parsing time from: %s\n", tokens[0]);
                }
                //token 2 的赋值
                strncpy(tempCommand, tokens[1], sizeof(tempCommand) - 1);
                tempCommand[sizeof(tempCommand) - 1] = '\0'; // 确保字符串以null结尾
                printf("tempCommand是%s\n",tempCommand);
                //token 3 的赋值
                strncpy(tempDeviceName, tokens[2], sizeof(tempDeviceName) - 1);
                tempDeviceName[sizeof(tempDeviceName) - 1] = '\0'; // 确保字符串以null结尾
                printf("tempDeviceName是%s\n",tempDeviceName);
                //token 4 的赋值
                if(sscanf(tokens[3], "%lldB", &tempDataSize) != 1) {
                    printf("Error parsing data size from: %s\n", tokens[3]);
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("success parsing time from: %s\n", tokens[3]);
                }

            } else if(tokenCount == 3 && strcmp(tokens[1], "spawn") == 0) {
                printf("正确：行'%s'的格式正确\n", line);
                 //token 1 的赋值
                if(sscanf(tokens[0], "%lldusecs", &tempTime) != 1) {
                    printf("Error parsing time from: %s\n", tokens[0]);
                    exit(EXIT_FAILURE);
                }
                else{
                    printf("success parsing time from: %s\n", tokens[0]);
                }
                //token 2 的赋值
                strncpy(tempCommand, tokens[1], sizeof(tempCommand) - 1);
                tempCommand[sizeof(tempCommand) - 1] = '\0'; // 确保字符串以null结尾
                printf("tempCommand是%s\n",tempCommand);
                 //token 3 的赋值
                strncpy(tempArg2, tokens[2], sizeof(tempArg2) - 1);
                tempArg2[sizeof(tempArg2) - 1] = '\0'; // 确保字符串以null结尾
                printf("tempArg2是%s\n",tempArg2);

            } else {
                printf("错误：行'%s'的格式不正确\n", line);
            }



            // 存储命令到当前程序的命令列表中
            CommandInfo *currentCommand = &programs[programCount].commands[programs[programCount].commandCount];
            currentCommand->time = tempTime;
            strncpy(currentCommand->command, tempCommand, sizeof(currentCommand->command) - 1);
            strncpy(currentCommand->deviceName, tempDeviceName, sizeof(currentCommand->deviceName) - 1);
            strncpy(currentCommand->arg2, tempArg2, sizeof(currentCommand->arg2) - 1);
            if(tempDataSize) {
                currentCommand->dataSize = tempDataSize;
            }
            if(tempSpendingtime) {
                currentCommand->Spendingtime = tempSpendingtime;
            }
            printf("Time: %llu, Command: %s, Device Name: %s, Arg2: %s, Data Size: %lld, Spending Time: %lld\n", 
            currentCommand->time, currentCommand->command, currentCommand->deviceName, 
            currentCommand->arg2, currentCommand->dataSize, currentCommand->Spendingtime);

            programs[programCount].commandCount++;
            

            // 检查是否读取完一个程序的所有命令


            printf("Value of tempCommand: '%s'\n", tempCommand);

            printf("Length of tempCommand: %ld\n", strlen(tempCommand));

            for(int i = 0; i < strlen(tempCommand); i++) {
            printf("%d ", tempCommand[i]);
            }
            printf("\n");

            if(strcmp(tempCommand, "exit") == 0) {
                printf("tempCommand: %s\n", tempCommand);
                programCount++;
                readingProgram = TRUE;
            }
            printf("programCount:%d\n",programCount);
        }

    }

    for (int i = 0; i < programCount; i++) {
        if (programs[i].commandCount > 0) {
            programs[i].totalRunTime = programs[i].commands[programs[i].commandCount - 1].time;
        } else {
            programs[i].totalRunTime = 0;
        }
        programs[i].elapsedRunTime=0;
    }
    
    if (programs[0].commandCount > 0) {
        programs[0].pid = 0 ;
    }
    

}

//  ----------------------------------------------------------------------



//-------------- execute 的全局变量 --------------
//模拟block队列
// int blockedProcessesQueue[2][MAX_RUNNING_PROCESSES];

//-------------- -------------- --------------



//-------------- -------------- --------------
//四条队列 queue

//新结构体__process的运行状态和时间

//



// 全局进程数组
ProgramInfo* processList[MAX_RUNNING_PROCESSES];


int currentProcessCount = 0;
int currentPid=0;

//-------------- -------------- --------------
// 辅助函数，作用添加一个RunProcess进入到ready队列 和 processList里面
void createAndAddProcess(ProgramInfo *progInfo) {
    if (currentProcessCount >= MAX_RUNNING_PROCESSES) {
        printf("Error: Process list is full!\n");
        return;
    }
    progInfo->state = READY;
     // 将进程添加到全局进程列表中
    processList[currentProcessCount] = progInfo;
    currentProcessCount++;
    
}
//
// 辅助函数，检查所有进程是否都已退出
BOOL areAllProcessesExited() {
    for (int i = 0; i < currentProcessCount; i++) {
        if (processList[i]->state != EXIT) {
            return FALSE;
        }
    }
    return TRUE;
}
// 从 ready 到 running
void setProcessFromReadyToRunning(ProgramInfo *progInfo) {
    if (progInfo) {
        progInfo->state = RUNNING;
        total_time = total_time + 5;
    } else {
        printf("Error: Null pointer passed to setProcessToRunning.\n");
    }
}
// 从 running 到 ready
void setProcessFromRunningToReady(ProgramInfo *progInfo) {
    if (progInfo) {  // 检查progInfo是否为非NULL
        progInfo->state = READY;
        total_time = total_time + 10;
    } else {
        printf("Error: Null pointer passed to setToReadyIfValid.\n");
    }
}

//递归
void setExitState(ProgramInfo* program, int targetPid) 
{
    if (!program) 
    {
        return; // 如果program为null，直接返回
    }

    // 如果找到匹配的PID，更新状态并返回
    if (program->pid == targetPid) 
    {
        program->state = EXIT;
        return;
    }

    // 否则，递归地在每个子进程上调用此函数
    for (int i = 0; i < MAX_SYSCALLS_PER_PROCESS && program->children[i]; i++) 
    {
        setExitState(program->children[i], targetPid);
    }
}

ProgramInfo* findProgramInfoRecursive(ProgramInfo* program, int targetPid) {
    if (!program) {
        return NULL;
    }

    if (program->pid == targetPid) {
        return program;
    }

    for (int i = 0; i < MAX_SYSCALLS_PER_PROCESS && program->children[i]; i++) {
        ProgramInfo* found = findProgramInfoRecursive(program->children[i], targetPid);
        if (found) {
            return found;
        }
    }

    return NULL;
}

//ready_queue --------
#define INVALID_PID 9999
int readyQueue[MAX_RUNNING_PROCESSES] = { [0 ... MAX_RUNNING_PROCESSES-1] = INVALID_PID };
// 入ready队列
void enqueueToReady(int pid) {
    // 查找队尾
    int i = 0;
    while (i < MAX_RUNNING_PROCESSES && readyQueue[i] != INVALID_PID) {
        i++;
    }

    if (i == MAX_RUNNING_PROCESSES) {
        // 队列满了
        return;
    }

    // 将新的pid加到队尾
    readyQueue[i] = pid;
}
// 出ready队列
int dequeueFromReady() {
    if (readyQueue[0] == INVALID_PID) {
        // 队列为空
        return INVALID_PID;
    }

    int dequeuedPid = readyQueue[0];
    
    // 将其他元素前移
    for (int i = 0; i < MAX_RUNNING_PROCESSES - 1; i++) {
        readyQueue[i] = readyQueue[i + 1];
    }
    readyQueue[MAX_RUNNING_PROCESSES - 1] = INVALID_PID;
    return dequeuedPid;
}
// check null
int isReadyQueueEmpty() {
    return readyQueue[0] == INVALID_PID;
}
//ready_queue --------

//running_queue --------
int runningQueue = INVALID_PID;
void enterRunningQueue(int pid) {
    if (runningQueue == INVALID_PID) {
        runningQueue = pid;
    }
    else{
        printf("error_node1\n");
    }  
}
// 从running中移除当前pid，并返回该pid
int exitRunningQueue() {
    int previousRunning = runningQueue;
    runningQueue = INVALID_PID;
    return previousRunning;
}
//
int isRunningQueueEmpty() {
    return runningQueue == INVALID_PID;
}
// peek
int peekRunningQueue(){
    return runningQueue;
}
// 这个函数检查running队列的第一个进程是否已经完成
int isRunningQueueFirstFinished() {
    int firstPid = peekRunningQueue(); // 假设这个函数返回running队列中的第一个PID，而不移除它
    if (firstPid == INVALID_PID) {
        return 0; // 如果running队列为空，返回0
    }

    ProgramInfo* currentProgram = findProgramInfoRecursive(processList[0], firstPid);
    if (!currentProgram) {
        return 0;
    }
    
    return (currentProgram->elapsedRunTime >= currentProgram->totalRunTime);
}
//running_queue --------

//block_queue --------
// int blockQueue[3][MAX_RUNNING_PROCESSES] = {
//     {INVALID_PID}, {INVALID_PID}, {INVALID_PID}
// };
int blockQueue[3][MAX_RUNNING_PROCESSES] = {
    [0 ... 2][0 ... MAX_RUNNING_PROCESSES-1] = INVALID_PID
};


long long getDeviceRate(char *deviceName, char *command) {
    for (int i = 0; i < deviceCount; i++) {
        if (strcmp(devices[i].deviceName, deviceName) == 0) {
            if (strcmp(command, "write") == 0) {
                return devices[i].outputRate;
            } else if (strcmp(command, "read") == 0) {
                return devices[i].inputRate;
            }
        }
    }
    return INVALID_PID;  // 如果没找到设备或命令无效
}

void enterBlockQueue(int pid, CommandInfo command) {
    long long rate = getDeviceRate(command.deviceName, command.command);
    int dataSize = command.dataSize;

    int index = MAX_RUNNING_PROCESSES - 1;
    for (; index > 0; index--) {
        if (rate <= blockQueue[1][index - 1]) {
            break;
        }
        blockQueue[0][index] = blockQueue[0][index - 1];
        blockQueue[1][index] = blockQueue[1][index - 1];
        blockQueue[2][index] = blockQueue[2][index - 1];
    }
    blockQueue[0][index] = pid;
    blockQueue[1][index] = rate;
    blockQueue[2][index] = dataSize;
}

BlockQueueExitResult exitBlockQueue() { //xu yao xiu gai fan hui san ge zhi
    BlockQueueExitResult result;
    result.pid = blockQueue[0][0];
    result.rate = blockQueue[1][0];
    result.dataSize = blockQueue[2][0];

    for (int i = 0; i < MAX_RUNNING_PROCESSES - 1; i++) {
        blockQueue[0][i] = blockQueue[0][i + 1];
        blockQueue[1][i] = blockQueue[1][i + 1];
        blockQueue[2][i] = blockQueue[2][i + 1];
    }
    blockQueue[0][MAX_RUNNING_PROCESSES - 1] = INVALID_PID;
    blockQueue[1][MAX_RUNNING_PROCESSES - 1] = INVALID_PID;
    blockQueue[2][MAX_RUNNING_PROCESSES - 1] = INVALID_PID;
    
    return result;
}
// peek
BlockQueueExitResult peekBlockQueue() {
    BlockQueueExitResult data;
    data.pid = blockQueue[0][0];
    data.rate = blockQueue[1][0];
    data.dataSize = blockQueue[2][0];
    return data;
}
// check
int isBlockQueueEmpty() {
    return blockQueue[0][0] == INVALID_PID;
}
//block_queue --------
//waiting_queue --------
int waitingQueue[MAX_RUNNING_PROCESSES] = { [0 ... MAX_RUNNING_PROCESSES-1] = INVALID_PID };

void enterWaitingQueue(int pid) {
    for (int i = 0; i < MAX_RUNNING_PROCESSES; i++) {
        if (waitingQueue[i] == INVALID_PID) {
            waitingQueue[i] = pid;
            break;
        }
    }
}

// 辅助函数：检查进程及其所有子进程的状态是否为EXIT
int areAllChildrenExited(ProgramInfo* program) {
    if (program->state != EXIT) {
        return 0;
    }
    for (int i = 0; i < MAX_SYSCALLS_PER_PROCESS && program->children[i] != NULL; i++) {
        if (program->children[i]->inlist != 1) {
            return 0;
        }
        if (!areAllChildrenExited(program->children[i])) {
            return 0;
        }
    }
    return 1;
}
//waiting to ready
int areAllDescendantsExited(ProgramInfo* program) {
    if (!program) {
        return 1; // 如果给定的结构为空，返回1表示“是”（此处可以根据上下文考虑是否更改为0）
    }

     // 添加调试输出以查看进程状态
    printf("Checking PID: %d, State: %d\n", program->pid, program->state);

    // if (program->state != EXIT) {
    //     printf("breakpoint324\n");
    //     printf("THIS program name = %s\n",program->programName);
    //     return 0; // 如果当前进程的状态不是EXIT，则返回0
    // }

    // 遍历所有子进程，并递归检查它们的状态
    for (int i = 0; i < MAX_SYSCALLS_PER_PROCESS && program->children[i]; i++) {
        if (program->children[i]) {
            printf("Checking child at index %d, with PID: %d\n", i, program->children[i]->pid);
            
            // 如果子进程的状态不是EXIT
            if (program->children[i]->state != EXIT) {
                printf("breakpoint324\n");
                printf("THIS program name = %s\n",program->children[i]->programName);
                return 0;
            }
            
            // 递归检查子进程的子进程
            if (!areAllDescendantsExited(program->children[i])) {
                return 0; // 如果任何子进程或子进程的子进程的状态不是EXIT，则返回0
            }
        }
    }

    return 1; // 如果所有子进程及其后代的状态都是EXIT，则返回1
}

ProgramInfo* findProgramByPid(ProgramInfo* program, int targetPid) {
    if (!program) {
        return NULL;
    }

    if (program->pid == targetPid) {
        return program;
    }

    for (int i = 0; i < MAX_SYSCALLS_PER_PROCESS && program->children[i]; i++) {
        ProgramInfo* found = findProgramByPid(program->children[i], targetPid);
        if (found) {
            return found;
        }
    }

    return NULL;
}

int shouldExitWaitingQueue(ProgramInfo* processList[], int targetPid) {
    ProgramInfo* targetProgram = findProgramByPid(processList[0], targetPid);  // Assuming processList[0] is the root
    if (!targetProgram) {
        printf("Program with PID %d not found!\n", targetPid);
        return 0;
    }
    return areAllDescendantsExited(targetProgram);
}

// int exitWaitingQueue(ProgramInfo* processList[],int targetPid) {
//     ProgramInfo* targetProgram = processList[targetPid];
//     if (areAllDescendantsExited(targetProgram)) {
//         for (int i = 0; i < MAX_RUNNING_PROCESSES; i++) {
//             if (waitingQueue[i] == targetPid) {
//                 waitingQueue[i] = INVALID_PID;
//                 return targetPid;
//             }
//         }
//     }
//     return INVALID_PID;  // 如果没有合适的进程可退出，返回invalid_pid
// }
//check
int isWaitingQueueEmpty() {
    for (int i = 0; i < MAX_RUNNING_PROCESSES; i++) 
    {
        if (waitingQueue[i]!=INVALID_PID)
        {
           return 0;
        }   
    }
    return 1;
}
//waiting_queue --------

//DataBus --------
int DataBus[2][1] = {{INVALID_PID}, {INVALID_PID}};

void enterDataBus(int pid, long long rate, int dataSize) {
    if (DataBus[0][0] == INVALID_PID) {
        DataBus[0][0] = pid;
        double endTime = total_time + (double)dataSize / rate * 1000000;
       
        // 自定义向上取整操作
        DataBus[1][0] = (int)endTime;
        if (endTime > (int)endTime) {
            DataBus[1][0]++;
        }
        //每次进入数据总线花费的20s
        DataBus[1][0] = DataBus[1][0] + 20;
        //extra second?
        // DataBus[1][0] = DataBus[1][0] + 1;
    } else {
        printf("DataBus is occupied.\n");
    }
}

int exitDataBus() {
    int pid = DataBus[0][0];
    DataBus[0][0] = INVALID_PID;
    DataBus[1][0] = INVALID_PID;
    return pid;
}
//check null
int isDataBusEmpty() {
    return DataBus[0][0] == INVALID_PID;
}
//DataBus --------

CommandInfo* findMatchingCommand(int pid) {
    ProgramInfo* program = findProgramInfoRecursive(processList[0], pid);
    if (program) {
        for (int j = 0; j < program->commandCount; j++) {
            if (program->commands[j].time == program->elapsedRunTime) {
                return &program->commands[j];
            }
        }
    }
    return NULL;
}
//
int visited[50][50];


void recordVisit(int pid, int time) {
    for (int i = 0; i < 50; i++) {
        if (visited[pid][i] == INVALID_PID) {
            visited[pid][i] = time;
            break;
        }
    }
}

int findVisit(int pid, int time) {
    for (int i = 0; i < 50; i++) {
        if (visited[pid][i] == time) {
            return 1;  // 找到了
        }
    }
    return 0;  // 没找到
}

//bug test print void
    void printAllPids(struct ProgramInfo* program) {
    if (!program) {
        return;
    }

    // 打印当前程序的pid
    printf("PID: %d\n", program->pid);

    // 递归地遍历所有子进程并打印它们的pid
    for (int i = 0; i < MAX_SYSCALLS_PER_PROCESS && program->children[i]; i++) {
        printAllPids(program->children[i]);
    }
}
//
//-------------- -------------- --------------
void execute_commands(void)
{   
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 50; j++) {
            visited[i][j] = INVALID_PID;
        }
    }
    //initial
    for (int i = 0; i < MAX_RUNNING_PROCESSES; i++) {
    processList[i] = NULL;
    }
    // 默认将第一个program放进ready队列
    processList[0] = &programs[0];
    // 设置programs[0]的pid为0, 和 inlist 设置为1
    processList[0]->pid = 0;
    processList[0]->inlist = 1;
    // 将pid 0加入到ready队列中
    enqueueToReady(0);
    int cpu = 0;// cpu node
    while (!isReadyQueueEmpty() || !isBlockQueueEmpty() || !isWaitingQueueEmpty() || !isRunningQueueEmpty()) {
        
        int tempid = INVALID_PID;
        if (total_time> 250)
        {   
            printf("total_time: %d\n", total_time);
            break;
        }
        printf("Block Queue:\n");
        for (int i = 0; i < 3; i++) {
            printf("Row %d: ", i + 1);
            for (int j = 0; j < 2; j++) {
                printf("%d ", blockQueue[i][j]);
            }
            printf("\n");
            }

        
        
        // 在此处放置你的逻辑
        if (!isDataBusEmpty())
        {   
            // int peekpid = peekRunningQueue();
            // 检查DataBus的endtime是否等于total_time
            if(cpu == 0){
                if ((DataBus[1][0] == total_time) || (DataBus[1][0] < total_time)) {
                    int pidFromDataBus = exitDataBus();
                    // 检查该PID是否在block队列中
                    for (int i = 0; i < MAX_RUNNING_PROCESSES; i++) {
                        if (blockQueue[0][i] == pidFromDataBus) {
                        // 如果找到，则将它从block队列移除并加入到ready队列中
                        BlockQueueExitResult data = exitBlockQueue();
                        enqueueToReady(data.pid);
                        
                        total_time = total_time + 10;
                        printf("当前pid：%d 从block 进入了ready队列，时间+10s, total = %d \n",data.pid, total_time);
                        // 考虑是否需要利用 data.rate 和 data.datasize 做其他的操作
                        break;
                        }
                    }
                goto start_of_while;  // 这会直接跳到下一次while循环
                }
            }
        }

        if (isRunningQueueEmpty()){
            
            tempid = dequeueFromReady();
            
            if (tempid !=INVALID_PID){
                
                enterRunningQueue(tempid);
                running_timequantum = timequantum;
                total_time = total_time + 5;
                printf("当前pid：%d 从ready 进入了running队列，时间+5s, total:%d\n",tempid,total_time);
                goto start_of_while;  // 这会直接跳到下一次while循环
            }
        }
        
        if (!isWaitingQueueEmpty())
        {   
            printf("breakpoint321\n");
           for (int i = 0; i < MAX_RUNNING_PROCESSES; i++)
           {    
                if (waitingQueue[i]!= INVALID_PID)
                {
                    /* code */
                    int pid = waitingQueue[i];
                    printf("pid in waiting: %d\n",pid);
                    if (shouldExitWaitingQueue(processList, pid)) {
                         // 从waitingQueue中删除该PID
                        waitingQueue[i] = INVALID_PID;

                        // 将该程序加入到ready队列
                        printf("breakpoint323\n");
                        enqueueToReady(pid);
                        printf("当前pid：%d 从waiting 进入了ready队列，时间+10s\n",pid);
                        total_time = total_time + 10;
                        goto start_of_while;  // 这会直接跳到下一次while循环
                    }
                } 
            } 
            printf("breakpoint322\n");
        }
        

        if (isDataBusEmpty())
        {
            if (!isBlockQueueEmpty())
            {
                // Peek the first process from blockQueue
                BlockQueueExitResult data = peekBlockQueue();
        
                // Put the first PID from blockQueue into DataBus and calculate the end time
                enterDataBus(data.pid, data.rate, data.dataSize);
            }
        }
        
        if (!isRunningQueueEmpty())
        {
            // if (isRunningQueueFirstFinished())
            // {
            //     int finishedPid = exitRunningQueue();
            //     // printf("当前pid：%d 从running里退出了\n",finishedPid);

            //     ProgramInfo* currentProgram = findProgramInfoRecursive(processList[0], finishedPid);
                
            //     if (currentProgram) {
            //         currentProgram ->state = EXIT;
            //     }
        
            // goto start_of_while;  // 这会直接跳到下一次while循环
            // }
            
            if (running_timequantum > 0)
            {
                int runningPid = peekRunningQueue();

                // printf("当前pid：%d 在进行任务判断\n",runningPid);

                ProgramInfo* matchedProgram = NULL;
                ProgramInfo* currentProgram = findProgramInfoRecursive(processList[0], runningPid);
                if (currentProgram) {
                    printf("当前pid：%d 找到了对应的pid的结构体\n",runningPid);
                    matchedProgram = currentProgram;
                }


                if (!matchedProgram) {
                    printf("当前pid：%d 没有找到对应的pid的结构体\n",runningPid);
                    // printf("Error: No matching program found for pid: %d\n", runningPid);
                    // Handle this error accordingly
                }


                BOOL foundTimeMatch = FALSE;

                if (matchedProgram)
                {   
                    // printf("found matched program");
                    for (int i = 0; i < matchedProgram->commandCount; i++) {
                        if ((matchedProgram->commands[i].time == (long long int)matchedProgram->elapsedRunTime)&&!findVisit(runningPid,matchedProgram->commands[i].time)) {
                            printf("visited shuzhu: %d\n", visited[0][3]);
                            // matchedProgram->commands[i].time--;

                            foundTimeMatch = true;
                            for (int i = 0; i < matchedProgram->commandCount; i++) {
                            // 这里假设CommandInfo有一个用于描述的字段叫做desc，如果没有，请修改为实际字段
                            printf("\tCommand %d: %lld\n", i, matchedProgram->commands[i].time); 
                            }
                            printf("%d,%lld\n",i,matchedProgram->commands[i].time);
                            printf("%d\n",matchedProgram->elapsedRunTime);
                            printf("duandian1\n");
                            break;
                        } 
                    }
                    
                    if (!foundTimeMatch) {
                        cpu = 1;// cpu node2
                        matchedProgram->elapsedRunTime++;
                        running_timequantum--;
                        total_time++;
                        printf("在进行正常的cpu运行, totaltime= %d, pid0 elapsedRunTime= %d\n, running_timequantum = %d ",total_time,processList[0]->elapsedRunTime, running_timequantum);
                        // //pid1 print
                        // if (total_time>345)
                        // {
                        //     printf("pid1的elapsedRunTime = %d\n",processList[0]->children[0]->elapsedRunTime);
                        // }
                        // //pid2 print
                        // if (total_time<803 && total_time> 505){
                        //     printf("pid2的elapsedRunTime = %d\n",processList[0]->children[1]->elapsedRunTime);
                        // }
                        // //
                        goto start_of_while;
                    }
                }
                
                
            }

            if (running_timequantum <= 0)
            {   
                int pidtomove = exitRunningQueue();
                printf("当前pid：%d 的时间量子不够了, total: %d\n",pidtomove,total_time);
                enqueueToReady(pidtomove);
                total_time = total_time + 10;
                running_timequantum = timequantum;
                printf("将pid： %d从running移动到ready 花10s ，当前total是：%d\n",pidtomove,total_time);
                goto start_of_while;
            }
            
            int runningPid = peekRunningQueue();
            CommandInfo* matchedCommand = findMatchingCommand(runningPid);

            
            if (matchedCommand)
            {
                printf("当前pid：%d 在进行任务判断, 当前的时间是: %d\n",runningPid, total_time);
                cpu = 0;
                if (strcmp(matchedCommand->command, "read") == 0 || strcmp(matchedCommand->command, "write") == 0) 
                {   
                    printf("当前pid：%d 进入了read和write的判断\n",runningPid);
                    int TempPid = exitRunningQueue();
                   
                    enterBlockQueue(TempPid, *matchedCommand);

                    printf("matchedCommand time= %lld\n", matchedCommand->time);

                    int temptime = (int)matchedCommand->time;
                    printf("temp time = %d\n", temptime);

                    recordVisit(TempPid, temptime); //lld -> int ???
                    printf("在read判断里输出visit ：%d\n",visited[0][0]);
                    
                    total_time = total_time + 10;
                    total_time++; //extra second 3
                    
                    
                    goto start_of_while;  // 这会直接跳到下一次while循环
                }

                if (strcmp(matchedCommand->command, "spawn") == 0)
                {   
                    printf("当前pid：%d 进入了spawn的判断\n",runningPid);
                    /* code */
                    for (int i = 0; i < MAX_RUNNING_PROCESSES; i++) 
                    {  
                        if (strcmp(programs[i].programName, matchedCommand->arg2) == 0)
                        {
                            // 找到目标programinfo
                            ProgramInfo targetProgram = programs[i];
                            
                            // 2. 新建一个与这个目标programinfo一模一样的programinfo 结构体
                            // ProgramInfo a2 = targetProgram;
                            ProgramInfo* a2 = (ProgramInfo*)malloc(sizeof(ProgramInfo));
                            if (!a2) {
                                // 内存分配失败
                                exit(1);
                            }
                            *a2 = targetProgram;  // 复制内容
                            // 3. 将它的pid标为currentPid + 1
                            // a2.pid = currentPid + 1;
                            a2->pid = currentPid + 1;
                            
                            // 4. 把a2加入到在processList中与running队列中pid一样的programinfo结构体里的子程序数组children里
                            ProgramInfo* runningProgram = processList[runningPid];
                            for (int j = 0; j < MAX_SYSCALLS_PER_PROCESS; j++)
                            {
                                if (runningProgram->children[j] == NULL)
                                {
                                    // runningProgram->children[j] = &a2;
                                    runningProgram->children[j] = a2;
                                    break;
                                }
                            }

                            printAllPids(processList[0]);

                            // 5. 将这个a2的 pid 加入到ready队列中
                            enqueueToReady(a2->pid);  // 请替换为真实的函数调用

                            // 6. 将当前running队列中的pid放入ready队列中
                            int tempPid = exitRunningQueue();
                            enqueueToReady(tempPid);

                            int temptime = (int)matchedCommand->time;
                            printf("temp time = %d\n", temptime);

                            recordVisit(tempPid, temptime);

                            total_time = total_time + 10;
                            total_time ++;   // extra second 2
                            printf("当前pid：%d 从running移动到ready +10s, total = %d\n",tempPid,total_time);
                            // 7. 然后让currentPid++
                            currentPid++;

                            // currentProcessCount++;

                            // 8. 跳到while循环开始
                            goto start_of_while;
                        }
                    }
                }
                
                if (strcmp(matchedCommand->command, "sleep") == 0)
                {
                    /* code */
                    printf("当前pid：%d 进入了sleep的判断\n",runningPid);
                }

                if (strcmp(matchedCommand->command, "wait") == 0)
                {   
                    printf("当前pid：%d 进入了wait的判断\n",runningPid);
                    int tempPid = exitRunningQueue();
                    enterWaitingQueue(tempPid);

                    int temptime = (int)matchedCommand->time;
                    printf("temp time = %d\n", temptime);

                    recordVisit(tempPid, temptime); //lld -> int ???

                    total_time = total_time + 10;
                    total_time++; //extra second 4
                    printf("当前pid：%d 从running移动到waiting +10s, total = %d\n",runningPid,total_time);
                    goto start_of_while;
                }

                if (strcmp(matchedCommand->command, "exit") == 0)
                {   
                    printf("breakpoint22\n");
                    int tempPid = exitRunningQueue();
                     printf("breakpoint23\n");
                    setExitState(processList[0], tempPid);  // 从processList的第一个元素开始递归搜索
                     printf("breakpoint24\n");
                    printf("当前pid：%d 从running移动到exit +0s, total = %d\n",runningPid,total_time);
                    // total_time= total_time+1; //extra time 7
                }


            }
            

        }
        

        total_time++;
        printf("total_time: %d\n", total_time);
        
        start_of_while:  // 这是一个标签，你可以使用goto跳转到这里
    }

    printf("breakpoint 3\n");


    
}

//  ----------------------------------------------------------------------

int main(int argc, char *argv[])
{
//  确保我们有正确数量的命令行参数
    if(argc != 3) {
        printf("使用方法：%s sysconfig文件 command文件\n", argv[0]);
        exit(EXIT_FAILURE);
    }
//  读取系统配置文件
    read_sysconfig(argv[0], argv[1]);

    //verify
    printf("\n");
    printf("timequantum value: %d\n", timequantum);
    for(int i = 0; i < deviceCount; i++) {
    printf("设备名称: %s\t读取速率: %lldBps\t写入速率: %lldBps\n", 
           devices[i].deviceName, devices[i].inputRate, devices[i].outputRate);
    }
    printf("\n");

//  读取命令文件
    read_commands(argv[0], argv[2]);
    printf("\n");
    printf("programCount的数值：%d\n",programCount);
    for(int i = 0; i < programCount; i++) {
    printf("程序名称: %s\n", programs[i].programName);

    for(int j = 0; j < programs[i].commandCount; j++) {
        printf("\t时间: %lld, 命令: %s, 设备名称: %s, 参数2: %s, 数据大小: %lld, 等待时间: %lld, 总运行时间: %d, 当前运行时间: %d \n", 
               programs[i].commands[j].time,
               programs[i].commands[j].command,
               programs[i].commands[j].deviceName,
               programs[i].commands[j].arg2,
               programs[i].commands[j].dataSize,
               programs[i].commands[j].Spendingtime,
               programs[i].totalRunTime,
               programs[i].elapsedRunTime
               );
    }
    printf("\n");
    }




//  执行命令，从command文件的第一个开始，直到没有剩余命令
    execute_commands();

//  打印程序的结果
    printf("测量结果  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}

//  vim: ts=8 sw=4
