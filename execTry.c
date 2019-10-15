//valgrind
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<time.h>
#include<fcntl.h>

struct someProcess {
    int number;
    pid_t PID;
    int status;
    time_t startTime;
    time_t endTime;
    double diff_time;
    char* command;
    char** arguments;
    int arglength;
};

void execute(struct someProcess proc) {
    if((proc.PID = fork()) < 0) {
        printf("Error in forking");
        return;
    }
    else if(proc.PID == 0) {
        //char* outName = strcat(("%d", proc.number), ".out"
        //Count number of digits in the process number
        int procNumber = proc.number;
        int charSize = 1;
        while(procNumber != 0) {
            charSize++;
            procNumber /= 10;
        }
        //Outfile buffer with snprintf, then open the file and dup2 
        char outBuf[charSize + 5]; //+5 fpr .out\0
        snprintf(outBuf, sizeof(outBuf), "%d.out", proc.number);
        int outFile = open(outBuf , O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        dup2(outFile, 1);
        close(outFile);
        
        //Error file buffer with snprintf, then open the file and dup2 
        char errBuf[charSize + 5]; //+5 for .err\0
        snprintf(errBuf, sizeof(errBuf), "%d.err", proc.number);
        int errorFile = open(errBuf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        dup2(errorFile, 2);
        close(errorFile);
        

        int e = execvp(proc.command, proc.arguments);
        if(e == -1) {
            //TODO: Write to files
            printf("Execution error\n");
        }
        exit(0);
    }
}

int main(int argc, char** argv) {
    time_t start_t, end_t;
    double diff_t;
    time(&start_t);
    //Counting number of processes required and creating an array of that many structs
    int processCounter = 0;
    for(int i = 1; i<argc; i++) {
        if(strcmp(argv[i], ".") == 0) {
            if(i == argc - 1) {
                break;
            }
            if(strcmp(argv[i+1], ".") == 0) {
                break;
            }
            processCounter++;
        }
    }
    processCounter++; //Extra for last process
    const int noProcesses = processCounter;

    struct someProcess* proc = malloc(processCounter * sizeof(* proc));

    //Spawning processes
    int spawnedProcesses = -1;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], ".") == 0) {
            continue;
        }
        spawnedProcesses++;
        proc[spawnedProcesses].command = malloc(strlen(argv[i]) + 1);
        strcpy(proc[spawnedProcesses].command, argv[i]);
        i++;
        int argCount = 0;
        for (int j = i; j < argc && strcmp(argv[j], ".") != 0; j++) {
            argCount++;
        }
        char *arguments[argCount + 2];    //Since command is first argument and NULL is last
        arguments[0] = proc[spawnedProcesses].command;
        for (int j = 1; j < argCount + 1; j++) {
            arguments[j] = argv[i];
            i++;
        }
        arguments[argCount + 1] = NULL;
        proc[spawnedProcesses].arguments = malloc((argCount + 2) * sizeof(char*));
        for(int m = 0; m < argCount + 1; m++) {
            proc[spawnedProcesses].arguments[m] = malloc((strlen(arguments[m]) + 1) * sizeof(char));
            strcpy(proc[spawnedProcesses].arguments[m], arguments[m]);
        }
        proc[spawnedProcesses].arguments[argCount + 1] = NULL;
        proc[spawnedProcesses].arglength = argCount;
        time(&(proc[spawnedProcesses].startTime));
        proc[spawnedProcesses].number = spawnedProcesses;
        //printf("%f\n", (double)proc[spawnedProcesses].startTime);
        execute(proc[spawnedProcesses]);

    }

    //Wait for processes
    while(processCounter > 0) {
        proc[processCounter - 1].PID = wait(&(proc[processCounter - 1].status));
        time(&(proc[processCounter - 1].endTime));
        //printf("%f\n", (double)proc[processCounter - 1].endTime);
        proc[processCounter - 1].diff_time = (double)difftime(proc[processCounter - 1].endTime, proc[processCounter - 1].startTime);
        printf("Child with PID %ld exited with status 0x%x.\n", (long)proc[processCounter - 1].PID, proc[processCounter - 1].status);
        if(proc[processCounter - 1].diff_time > 2.0) {
            //Restart process
            execute(proc[processCounter - 1]);
            continue;
        }
        else {
            free(proc[processCounter - 1].command);
            for(int i = 0; i<proc[processCounter].arglength; i++) {
                free(proc[processCounter - 1].arguments[i]);
            }
            free(proc[processCounter - 1].arguments);
            --processCounter;
        }

    }
    free(proc);
    time(&end_t);
    diff_t = difftime(end_t, start_t);
    printf("%f\n", diff_t);
}