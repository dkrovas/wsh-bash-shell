#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#define HISTORY_SIZE 5

char (*history)[100];
int history_count = 0;
int history_capacity = HISTORY_SIZE;

typedef struct Local{
    char *name;
    char *val;
} LOCAL;
typedef struct LocalArray {
    LOCAL *locals;
    size_t size;
} LocalArray;
// Batch file case
void processBatchFile(FILE *inputFile);
// Local Array initiation
void initLocalArray(LocalArray *localArray);
void addLocalVariable(LocalArray *localArray, const char *name, const char *value);


//Local variable builtin command functions
void clearLocalVariable(LocalArray *localArray, const char *name);
void printLocalVars(const LocalArray *localArray);
char* getVarVal(const LocalArray *localArray, const char *name);
void local(int argc, char *argv[], LocalArray *localArray);
void executeVar(int argc, char *argv[], LocalArray *localArray);
int isValidVarNameChar(char c);
// Export variable builtin command function
void exportVariable(LocalArray *localArray, const char *arg);

void processWshCommands(int argc, char *argv[], LocalArray *localArray);

// History builtin command functions
void addToHistory(int argc, char *argv[]);
void historyfunc(int argc, char *argv[], LocalArray *localArray);
void setHistory(int n);
void executeFromHistory(int n, LocalArray *localArray);

// Exec func
void execFunc(int argc, char *argv[]);
void execNormal(int argc, char *argv[]);
void execPipe(int argc, char *argv[], int pipeIndex, int numPipes);


// CD builtin command functions
void cdFunc(int argc, char *argv[], LocalArray *localArray);

int main(int argc, char *argv[]) {
    char *bash_input = NULL;
    size_t len = 0;
    if (argc > 2) {
        printf("ERROR: too many arguments\n");
        return 1;
    }
    if (argc == 2) {
        // Handle script.
        FILE *batchFile = fopen(argv[1], "r");
        if (!batchFile){
            printf("ERROR: Can't open batch file\n");
            return 1;
        }
        if(fgetc(batchFile) == EOF){
            fclose(batchFile);
            printf("ERROR: BatchFile Empty\n");
            return 1;
        }
        rewind(batchFile);
        processBatchFile(batchFile);
        fclose(batchFile);
    }
    else{
         LocalArray localArray;
        initLocalArray(&localArray);
        history = malloc(HISTORY_SIZE * sizeof(char[100]));
        // Check if memory allocation is successful
        if (history== NULL) {
            fprintf(stderr, "Failed to allocate memory for history.\n");
            return 1;
        }
        while (1) {
            printf("wsh> ");
            if (getline(&bash_input, &len, stdin) > 0) {
                len = strlen(bash_input);
                if (len > 0 && bash_input[len - 1] == '\n') {
                    bash_input[len - 1] = '\0';
                }

                // Check for exit condition
                if (strcmp(bash_input, "exit") == 0) {
                    break;
                }

                // Process the command
                int currargc = 0;
                char *currargv[45];
                char *token = strtok(bash_input, " \n");
                // hsndling error use helper function or macro function
                // for failed allocation
                while (token != NULL) {
                    currargv[currargc++] = strdup(token);
                    token = strtok(NULL, " \n");
                }

                currargv[currargc] = NULL;

                processWshCommands(currargc, currargv, &localArray);

                // Free currargv
                for (int j = 0; j < currargc; j++) {
                    free(currargv[j]);
                }
            } else {
                break; // getline failed or reached EOF
            }
        }
        // Free if not getting freed inside loop
        free(bash_input);
        return 0;   
    }
}
void processBatchFile(FILE *inputFile){
    LocalArray localArray;
    initLocalArray(&localArray);
    history = malloc(HISTORY_SIZE * sizeof(char[100]));
    char line[256];
    // Check if memory allocation is successful
    if (history == NULL) {
        fprintf(stderr, "Failed to allocate memory for history.\n");
    return;
    }
    // code taken from p1
    while(fgets(line, sizeof(line), inputFile) != NULL){
        int currargc = 0;
        char *currargv[45];
        char *token = strtok(line, " \t\n");
        while(token != NULL) {
            currargv[currargc++] = strdup(token);
            token = strtok(NULL, " \t\n");
        }
        currargv[currargc] = NULL;
        processWshCommands(currargc, currargv, &localArray);
        for(int j = 0; j < currargc; ++j){
            free(currargv[j]);
        }
    }
}
void initLocalArray(LocalArray *localArray){
    localArray->locals = NULL;
    localArray->size = 0;
}

void addLocalVariable(LocalArray *localArray, const char *name, const char *value) {
    localArray->size++;
    localArray->locals = realloc(localArray->locals, localArray->size * sizeof(LOCAL));

    localArray->locals[localArray->size - 1].name = strdup(name);
    localArray->locals[localArray->size - 1].val = strdup(value);
}

void clearLocalVariable(LocalArray *localArray, const char *name){
    for (size_t i = 0; i < localArray->size; i++){
        if(strcmp(localArray->locals[i].name, name)==0){
            free(localArray->locals[i].val);
            free(localArray->locals[i].name);
            for(size_t j = i; j < localArray->size -1; j++){
                localArray->locals[j] = localArray->locals[j+1];
            }
            localArray->size--;
            break;
        }
    }
}

void printLocalVars(const LocalArray *localArray){
    if(localArray->size > 0){
        for (size_t i = 0; i < localArray->size; i++){
            printf("%s=%s\n", localArray->locals[i].name, localArray->locals[i].val);
        }
    }
}

char* getVarVal(const LocalArray *localArray, const char *name){
    const char* envValue = getenv(name);
    if (envValue != NULL) {
        return strdup(envValue);
    }
    for(size_t i = 0; i < localArray->size; i++){
        if (strcmp(localArray->locals[i].name, name) == 0) {
            return localArray->locals[i].val;
        }
    }
    return "";
}
int isValidVarNameChar(char c) {
    return !isspace(c);
}
void executeVar(int argc, char *argv[], LocalArray *localArray){
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '$') {
            // Remove the '$' from the variable name
            const char *varNameStart = argv[i] + 1;
            const char *varNameEnd = varNameStart;
            while (*varNameEnd != '\0' && isValidVarNameChar(*varNameEnd)) {
                varNameEnd++;
            }
            char *varName = strndup(varNameStart, varNameEnd - varNameStart);
            const char *varValue = getVarVal(localArray, varName);
            free(varName);
            if (strcmp(varValue, "") != 0) {
                // Substitute the variable value
                free(argv[i]);
                argv[i] = strdup(varValue);
            } else {
                free(argv[i]);
                argc--;

                // Shift the remaining elements in argv
                for (int j = i; j < argc; j++) {
                    argv[j] = argv[j + 1];
                }
                // Set the last element to NULL
                argv[argc] = NULL;

                // Decrement i to reprocess the current index, as it now contains a new argument
                i--;
            }
        }
        else{
            continue;
        }
    }
    // Submit for processing and execution of command
    processWshCommands(argc, argv, localArray);
}
void exportVariable(LocalArray *localArray, const char *arg) {
    char *equalSign = strchr(arg, '=');
    if (equalSign != NULL){
        *equalSign = '\0';
        const char *name = arg;
        const char *value = equalSign + 1;

        if(value == NULL){
            unsetenv(name);
        }
        setenv(name, value, 1);
    }
    else {
        printf("Usage: export VARNAME=VALUE\n");
    }
}

void processWshCommands(int argc, char *argv[], LocalArray *localArray){
    for(int i = 0; i < argc; ++i){
        for(int j = 0; j < strlen(argv[i]); ++j){
            if(argv[i][j] == '$'){
                if (j == 0){
                    executeVar(argc, argv, localArray);
                    return;
                }
                else if (j > 0 && argv[i][j-1] == ' '){
                    executeVar(argc, argv, localArray);
                    return;
                }
            }
        }
    }
    if (argc < 1){
        return;
    } else if (strcmp(argv[0], "export") == 0) {
        if (argc == 2){
            exportVariable(localArray, argv[1]);
        }
        else {
            printf("Usage: export VARNAME=VALUE\n");
        }
        return;
    } else if (strcmp(argv[0], "cd") == 0) {
        cdFunc(argc, argv, localArray);
        return;
    } else if (strcmp(argv[0], "local") == 0) {
        local(argc, argv, localArray);
        return;
    } else if (strcmp(argv[0], "history") == 0) {
        historyfunc(argc, argv, localArray); 
        return;
    } else if (strcmp(argv[0], "vars") == 0){
        printLocalVars(localArray);
        return;
    } else if(strcmp(argv[0], "exit") == 0){
        // This is mostly for the case where it is input from a batchfile
        exit(0);
    } else {
        addToHistory(argc, argv);
        execFunc(argc, argv);
        return;
    }   
}
void local(int argc, char *argv[], LocalArray *localArray){
    if (argc != 2){
        printf("Usage: local <name=value>\n");
        return;
    }
    char *input = argv[1];
    char *equalSign = strchr(input, '=');
    if(equalSign == NULL){
        printf("Usage: local <name=value>\n");
        return;
    }
    else {
        char *name = strndup(input, equalSign - input);
        char *value = strdup(equalSign + 1);
        if(strlen(value) == 0){
            clearLocalVariable(localArray, name);
            return;
        }
        for (size_t i = 0; i < localArray->size; i++){
            if(strcmp(localArray->locals[i].name, name) == 0) {
                free(localArray->locals[i].val);
                localArray->locals[i].val = strdup(value);
                free(name);
                free(value);
                return;
            }
        }
        addLocalVariable(localArray, name, value);
    }
}

void historyfunc(int argc, char *argv[], LocalArray *localArray){
    if (argc == 3){
        if (strcmp(argv[1], "set") == 0){
            setHistory(atoi(argv[2]));
            return;
        }
    }
    else if (argc == 2){
        int n = atoi(argv[1]);
        executeFromHistory(n, localArray);
        return;
    }
    else if (argc==1) {
        if(history){
            for (int i = 0; i < history_count; i++) {
                if(strcmp(history[i], "") != 0){
                    printf("%d) %s\n", i + 1, history[i]);
                }
            }
        }
        return;
    }
    return;
}
void setHistory(int n) {
    if (n >= 0) {
        if(n == 0){
            free(history);
            history = NULL;
            history_capacity = 0;
            history_count = 0;
        }
        else if(n < history_capacity) {
            char (*new_history)[100] = realloc(history, n * sizeof(char[100]));

            if(new_history != NULL) {
                history = new_history;
                history_capacity = n;
                if(history_count > n){
                    history_count = n;
                }
            }
        }
        else if (n>history_capacity) {
            char (*new_history)[100] = realloc(history, n * sizeof(char[100]));
        
            if(new_history != NULL){
                history = new_history;
                history_capacity = n;
            }
        }
    }
}
void executeFromHistory(int n, LocalArray *localArray){
    if (n >= 1 && n <= history_count){
        //execute command from history by transforming content from history into argv
        // and argc, so that you can place it in a function.
        for (int i = 0; i < history_count; i++) {
            if(i +1 == n){
                int currargc = 0;
                char *currargv[45];
                char *token = strtok(history[i], " \t\n");
                // process bash_input in here into a new argv and argc
                while(token != NULL) {
                    currargv[currargc++] = strdup(token);
                    token = strtok(NULL, " \t\n");
                }
                currargv[currargc] = NULL;
                processWshCommands(currargc, currargv, localArray);
                // Free currargv
                for (int j = 0; j < currargc; j++){
                    free(currargv[j]);
                }
            }
        }
    }
    return;
}

void addToHistory(int argc, char *argv[]) {
    int total_length = 0;
    for (int i = 0; i < argc; i++) {
        total_length += strlen(argv[i]) + 1; // +1 for space or null terminator
    }
    char *command_str = malloc(total_length);
    if (command_str == NULL) {
        // Handle allocation failure
        return;
    }

    command_str[0] = '\0'; // Initialize an empty string
    for (int i = 0; i < argc; i++) {
        strcat(command_str, argv[i]);
        if (i < argc - 1) {
            strcat(command_str, " ");
        }
    }
    for (int i = 0; i < history_count; i++) {
        if (strcmp(history[0], command_str) == 0) {
            free(command_str);
            return;
        }
    }
    if(history_capacity == 0){
        free(command_str);
        return;
    }
    else if (history_count < history_capacity) {
        // If there is space in the history array, simply add the new command at the end
        for (int i = history_count; i > 0; i--) {
            strcpy(history[i], history[i - 1]);
        }
        strcpy(history[0], command_str);
        history_count++;
    } else {
        // If the history array is full, shift all commands to the right
        for (int i = history_capacity - 1; i > 0; i--) {
            strcpy(history[i], history[i - 1]);
        }
        strcpy(history[0], command_str);
    }
    free(command_str);
    return;
}
void execFunc(int argc, char *argv[]) {
    int hasPipe = 0;
    // Find the index of the first pipe character
    int pipeIndex = -1;
    int numPipes = 0;

    // could parse argv here in advance! 
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0 && numPipes != 0) {
            ++numPipes;
        }
        else if (strcmp(argv[i], "|") == 0 && numPipes == 0) {
            hasPipe = 1;
            pipeIndex = i;
            ++numPipes;
        }
    }
    
    if (!hasPipe) {
        // No pipe, execute the command as a single process
        execNormal(argc, argv);
        return;
    } else {
        execPipe(argc, argv, pipeIndex, numPipes);
        return;
    }
}
void execNormal(int argc, char *argv[]){
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        if (execvp(argv[0], argv) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    return;
}

void execPipe(int argc, char *argv[], int pipeIndex, int numPipes) {
    int pipefds[numPipes][2];
    pid_t pid;
    // Create pipes
    for (int i = 0; i < numPipes; ++i) {
        if (pipe(pipefds[i]) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i <= numPipes; ++i) {
        pid = fork();

        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            int cmd_start = 0;
            int cmd_end = 0;

            for (int j = 0; j <= i; ++j) {
                cmd_start = cmd_end;
                while (cmd_end < argc && strcmp(argv[cmd_end], "|") != 0) {
                cmd_end++;
                }
                cmd_end++;  // Move past the pipe symbol
            }
            
            int cmd_argc = cmd_end - cmd_start - 1;
            char **cmd_argv = malloc(sizeof(char*)*(cmd_argc+1));
            if (cmd_argv == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }

            for (int j = 0, k = cmd_start; j < cmd_argc && k < cmd_end; j++, k++) {
                if(k < cmd_end){
                    cmd_argv[j] = strdup(argv[k]);
                    if(cmd_argv[j]==NULL){
                        perror("Memory allocation failed");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            cmd_argv[cmd_argc] = NULL;
            
            // Close unused read end of previous pipe (except for the first command)
            if (i > 0) {
                dup2(pipefds[i - 1][0], STDIN_FILENO);
            }

            // Close unused write end of subsequent pipes (except for the last command)
            if (i < numPipes) {
                dup2(pipefds[i][1], STDOUT_FILENO);
            }

            // Close all pipe file descriptors
            for (int j = 0; j < numPipes; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            // Execute the command

            if (execvp(cmd_argv[0], cmd_argv) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }

            // The child process should not reach here
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            // Close write end of the current pipe (used by child process)
            if (i > 0 ) {
                close(pipefds[i-1][1]);
            }
        }
    }

    close(pipefds[numPipes - 1][0]);
    close(pipefds[numPipes - 1][1]);

    // Wait for all child processes to finish
    int status;
    for (int i = 0; i <= numPipes; i++) {
        waitpid(-1, &status, 0);
    }
}
void cdFunc(int argc, char *argv[], LocalArray *localArray){
    if (argc > 2){
        return;
    }
    else if (argc == 1){
        return;
    }
    else {
        char *arg;
        char *dir;
        arg = argv[1];
        char *currentDir = getcwd(NULL, 0);
        if (arg[0] == '$'){
            char *varName = arg + 1;
            char *varValue = getVarVal(localArray, varName);
            if(strlen(varValue)>0){
                dir = varValue;
            }
            else {
                return;
            }
        }
        else{
            dir = arg;
        }
        if(currentDir == NULL){
            perror("cd");
            return;
        }
        if (chdir(dir)==-1) {
            perror("cd");
            chdir(currentDir);
        }
        free(currentDir);
    }
    return;
}
