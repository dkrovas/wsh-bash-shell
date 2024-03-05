# wsh-bash-shell
Custom Bash shell program incorporating advanced functionalities such as cd, export, local, vars, and history commands. Implemented chdir system call for the cd command, handling errors effectively with seamless navigation and interaction within the shell environment. This program Utilizes fork/exec and piping techniques to enhance usability and efficiency. It also has the capability to create and managed environment variables using export and shell variables using local commands.
NAME: Dante Katz Andrade
CS LOGIN: katz-andrade
WISCID: 9083567322
WISCEMAIL: dkatzandrade@wisc.edu
STATUS: working
CHANGES:
Function to process batchFile
        void processBatchFile(FILE *inputFile);
Initiating arrays:
        void initLocalArray(LocalArray *localArray);
        void addLocalVariable(LocalArray *localArray, const char *name, const char *value);


Local variable functions
        void clearLocalVariable(LocalArray *localArray, const char *name);
        void printLocalVars(const LocalArray *localArray);
        char* getVarVal(const LocalArray *localArray, const char *name);
        void local(int argc, char *argv[], LocalArray *localArray);
        void executeVar(int argc, char *argv[], LocalArray *localArray);
        int isValidVarNameChar(char c);

Export variable builtin command function
        void exportVariable(LocalArray *localArray, const char *arg);
Function for processing commands
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



