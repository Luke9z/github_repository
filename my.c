#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/// @brief /////

typedef struct {
    long long time;  // 时间
    char command[MAX_COMMAND_NAME]; // 命令
    char deviceName[MAX_DEVICE_NAME];  // 参数1
    char arg2[MAX_DEVICE_NAME];  // 参数2 记录子程序的名称
    long long dataSize;  // 如果有的话，数据的大小
    long long Spendingtime;  // 用于给sleep的时间, 默认是0
} CommandInfo;

typedef struct {
    char programName[MAX_DEVICE_NAME];  // 程序名称
    CommandInfo commands[MAX_SYSCALLS_PER_PROCESS];  // 该程序的命令列表
    int commandCount;  // 当前程序的命令数量
} ProgramInfo;

ProgramInfo programs[MAX_RUNNING_PROCESSES];  // 全局的程序数组
int programCount = 0;  // 当前已存储的程序数量



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

}

//  ----------------------------------------------------------------------

void execute_commands(void)
{
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
        printf("\t时间: %lld, 命令: %s, 设备名称: %s, 参数2: %s, 数据大小: %lld, 等待时间: %lld\n", 
               programs[i].commands[j].time,
               programs[i].commands[j].command,
               programs[i].commands[j].deviceName,
               programs[i].commands[j].arg2,
               programs[i].commands[j].dataSize,
               programs[i].commands[j].Spendingtime);
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
