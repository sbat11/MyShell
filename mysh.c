#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>


#define BUFFER_SIZE 512

int interactive = 0;

typedef struct Node {
    char* word;
    struct Node* next;
} Node;

Node* createNode(char* word) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (node == NULL) {
        perror("Failed to allocate memory for Node");
        return NULL;
    }
    
    node->word = strdup(word);
    if (node->word == NULL) { 
        perror("Failed to allocate memory for word");
        free(node); 
        return NULL;
    }

    node->next = NULL;
    return node;
}
bool matchesWildcard(const char* fileName, const char* pattern) {
    char* star = strchr(pattern, '*');
    if (!star) {
        return strcmp(fileName, pattern) == 0;  // Exact match
    }

    // Split into prefix and suffix
    int prefixLen = star - pattern;
    const char* suffix = star + 1;

    // Match prefix
    if (strncmp(fileName, pattern, prefixLen) != 0) {
        return false;
    }

    // Match suffix
    int fileNameLen = strlen(fileName);
    int suffixLen = strlen(suffix);
    if (fileNameLen < prefixLen + suffixLen) {
        return false;
    }

    return strcmp(fileName + fileNameLen - suffixLen, suffix) == 0;
}
Node* expandWildcard(const char* token) {
    char* star = strchr(token, '*');
    if (!star) {
        return createNode((char*)token);  // No wildcard, return original
    }

    // Separate directory and pattern
    char dirPath[BUFFER_SIZE] = ".";
    const char* pattern = token;

    char* slash = strrchr(token, '/');
    if (slash) {
        strncpy(dirPath, token, slash - token);
        dirPath[slash - token] = '\0';
        pattern = slash + 1;
    }

    DIR* dir = opendir(dirPath);
    if (!dir) {
        perror("opendir");
        return createNode((char*)token);  // Return token unchanged
    }

    struct dirent* entry;
    Node* head = NULL;
    Node* tail = NULL;

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files unless explicitly matched
        if (entry->d_name[0] == '.' && pattern[0] != '.') {
            continue;
        }

        if (matchesWildcard(entry->d_name, pattern)) {
            // Build fullPath using strcpy and strcat
            char fullPath[BUFFER_SIZE];
            if (strlen(dirPath) + strlen(entry->d_name) + 2 > BUFFER_SIZE) {  // Ensure it fits
                fprintf(stderr, "Path too long: %s/%s\n", dirPath, entry->d_name);
                continue;
            }

            fullPath[0] = '\0';  // Clear the buffer
            strcpy(fullPath, dirPath);  // Copy dirPath
            strcat(fullPath, "/");      // Append the slash
            strcat(fullPath, entry->d_name);  // Append the file name

            Node* newNode = createNode(fullPath);
            if (!newNode) {
                perror("Failed to create node");
                continue;
            }

            if (!head) {
                head = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
        }
    }

    closedir(dir);

    if (!head) {
        return createNode((char*)token);  // No matches, return token unchanged
    }

    return head;
}
Node* expandTokenList(Node* head) {
    Node* newHead = NULL;
    Node* newTail = NULL;

    while (head) {
        Node* expanded = expandWildcard(head->word);
        Node* temp = head;
        head = head->next;

        if (expanded) {
            if (!newHead) {
                newHead = expanded;
                newTail = expanded;
                while (newTail->next) {
                    newTail = newTail->next;
                }
            } else {
                newTail->next = expanded;
                while (newTail->next) {
                    newTail = newTail->next;
                }
            }
        }

        free(temp->word);
        free(temp);
    }

    return newHead;
}

int getListSize(Node* head) {
    Node* ptr = head;
    int length = 0;

    while (ptr != NULL) {
        length++;
        ptr = ptr->next;
    }

    return length;
}

Node* createTokenList(char* line) {
    Node* head = NULL;
    Node* ptr = NULL;
    int wordLength = 0;
    int wordCapacity = BUFFER_SIZE;
    char* word = (char*)malloc(wordCapacity * sizeof(char));

    if (!word) {
        perror("Failed to allocate memory for word");
        return NULL;
    }

    for (int i = 0; i <= strlen(line); i++) {
        char c = line[i];

        if (c == ' ' || c == '\0' || c == '\n') {
            if (wordLength > 0) { 
                word[wordLength] = '\0';
                Node* newNode = createNode(word);
                if (!newNode) {
                    free(word);
                    return NULL;
                }
                if (head == NULL) {
                    head = newNode;
                    ptr = head;
                } else {
                    ptr->next = newNode;
                    ptr = ptr->next;
                }

                wordLength = 0;
            }
        } else if (c == '<' || c == '>' || c == '|') {  // Special tokens
            if (wordLength > 0) {  // Add the previous token first
                word[wordLength] = '\0';
                Node* newNode = createNode(word);
                if (!newNode) {
                    free(word);
                    return NULL;
                }
                if (head == NULL) {
                    head = newNode;
                    ptr = head;
                } else {
                    ptr->next = newNode;
                    ptr = ptr->next;
                }
                wordLength = 0;
            }

            // Add the special token
            word[0] = c;
            word[1] = '\0';
            Node* newNode = createNode(word);
            if (!newNode) {
                free(word);
                return NULL;
            }
            
            if (head == NULL) {
                head = newNode;
                ptr = head;
            } else {
                ptr->next = newNode;
                ptr = ptr->next;
            }
        } else {  // Part of a regular token
            if (wordLength >= wordCapacity - 1) {  // Resize if needed
                wordCapacity *= 2;
                word = realloc(word, wordCapacity * sizeof(char));
                if (!word) {
                    perror("Failed to reallocate memory for word");
                    return NULL;
                }
            }
            word[wordLength++] = c;
        }
    }

    free(word);
    return head;
}

void freeTokenList(Node* head) {
    Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        if(temp -> word){
            free(temp->word);
        }
        free(temp);
    }
}

char* findExecutablePath(const char* command) {
    const char* searchPaths[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};
    static char fullPath[BUFFER_SIZE];  // Buffer to store the full path

    for (int i = 0; searchPaths[i] != NULL; i++) {
        fullPath[0] = '\0';
        strcpy(fullPath, searchPaths[i]);
        if (fullPath[strlen(fullPath) - 1] != '/') {
            strcat(fullPath, "/");
        }
        strcat(fullPath, command);
        if (access(fullPath, X_OK) == 0) {
            return fullPath;  // Return if executable is found
        }
    }
    return NULL;  // Command not found
}
void executeCommandNoFork(Node* head) {
    if (!head) return;

    char* inputFile = NULL;
    char* outputFile = NULL;
    // < hello.txt cat a.txt
    // parse redirection tokens

    Node* prev = NULL;
    Node* curr = head;
    while (curr != NULL) {
        if (strcmp(curr->word, "<") == 0) {  // input redirection
            if (curr->next) {
                inputFile = strdup(curr->next->word);

                Node* temp = curr;
                curr = curr->next->next;

                free(temp->next->word);
                free(temp->next);
                free(temp->word);
                free(temp);

                if (prev) {
                    prev->next = curr;
                } else {
                    head = curr;  
                }
                continue;
            } else {
                fprintf(stderr, "Error: Missing input file for redirection\n");
                exit(1);
            }
        } else if (strcmp(curr->word, ">") == 0) {  // Output redirection
            if (curr->next) {
                outputFile = strdup(curr->next->word);

                Node* temp = curr;
                curr = curr->next->next;

                free(temp->next->word);
                free(temp->next);
                free(temp->word);
                free(temp);

                if (prev) {
                    prev->next = curr;
                } else {
                    head = curr;  
                }
                continue;
            } else {
                fprintf(stderr, "Error: Missing output file for redirection\n");
                exit(1);
            }
        }

        prev = curr;
        curr = curr->next;
        
    }
    
    char* command = head->word;

    // build argument list
    int argc = getListSize(head);
    char** argv = malloc((argc + 1) * sizeof(char*));
    Node* current = head;
    for (int i = 0; i < argc; i++) {
        argv[i] = strdup(current->word);
        current = current->next;
    }
    argv[argc] = NULL;
    
    int originalStdin = dup(STDIN_FILENO);
    int originalStdout = dup(STDOUT_FILENO);

    // handle input redirection
    if (inputFile) {
        int fd = open(inputFile, O_RDONLY);
        if (fd < 0) {
            perror("Failed to open input file");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
        free(inputFile);
    }

    // handle output redirection
    if (outputFile) {
        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd < 0) {
            perror("Failed to open output file");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        free(outputFile);
    }

    char* executablePath = command;
    if (strchr(command, '/') == NULL) {  // barename
        executablePath = findExecutablePath(command);
    }

    if (!executablePath) {
        fprintf(stderr, "%s: command not found\n", command);
        for (int i = 0; i < argc; i++) {
            free(argv[i]);  
        }
        free(argv);
        exit(127);  
    }
    
    dup2(originalStdin, STDIN_FILENO);
    dup2(originalStdout, STDOUT_FILENO);
    close(originalStdin);
    close(originalStdout);

    execv(executablePath, argv);
    perror("execv failed");
    free(argv);
}
void executeCommand(Node* head) {
    if (!head) return;

    char* inputFile = NULL;
    char* outputFile = NULL;
    // < hello.txt cat a.txt
    // parse redirection tokens

    Node* prev = NULL;
    Node* curr = head;
    while (curr != NULL) {
        if (strcmp(curr->word, "<") == 0) {  // input 
            if (curr->next) {
                inputFile = strdup(curr->next->word);

                Node* temp = curr;
                curr = curr->next->next;

                free(temp->next->word);
                free(temp->next);
                free(temp->word);
                free(temp);

                if (prev) {
                    prev->next = curr;
                } else {
                    head = curr;  
                }
                continue;
            } else {
                fprintf(stderr, "Error: Missing input file for redirection\n");
                exit(1);
            }
        } else if (strcmp(curr->word, ">") == 0) {  // output
            if (curr->next) {
                outputFile = strdup(curr->next->word);

                Node* temp = curr;
                curr = curr->next->next;

                free(temp->next->word);
                free(temp->next);
                free(temp->word);
                free(temp);

                if (prev) {
                    prev->next = curr;
                } else {
                    head = curr;  // Update head if the first node is removed
                }
                continue;
            } else {
                fprintf(stderr, "Error: Missing output file for redirection\n");
                exit(1);
            }
        }

        prev = curr;
        curr = curr->next;
        
    }
    
    char* command = head->word;

    // build argument list
    int argc = getListSize(head);
    char** argv = malloc((argc + 1) * sizeof(char*));
    Node* current = head;
    for (int i = 0; i < argc; i++) {
        argv[i] = strdup(current->word);
        current = current->next;
    }
    argv[argc] = NULL;
    
    int originalStdin = dup(STDIN_FILENO);
    int originalStdout = dup(STDOUT_FILENO);

    // input file redirection
    if (inputFile) {
        int fd = open(inputFile, O_RDONLY);
        if (fd < 0) {
            perror("Failed to open input file");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
        free(inputFile);
    }

    // output file redirection
    if (outputFile) {
        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd < 0) {
            perror("Failed to open output file");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        free(outputFile);
    }

    // find path of executable

    char* executablePath = command;
    if (strchr(command, '/') == NULL) {  
        executablePath = findExecutablePath(command);
    }

    if (!executablePath) {
        fprintf(stderr, "%s: command not found\n", command);
        for (int i = 0; i < argc; i++) {
            free(argv[i]);  
        }
        free(argv);
        return; 
    }

    pid_t pid = fork();
    if (pid == 0) { 
        execv(executablePath, argv);
        perror("execv failed");  
        exit(1);
    } else if (pid > 0) {  
        int status;
        waitpid(pid, &status, 0);  
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Command exited with code %d\n", WEXITSTATUS(status));
        }
    } else {
        perror("fork failed");
    }
    
    dup2(originalStdin, STDIN_FILENO);
    dup2(originalStdout, STDOUT_FILENO);
    close(originalStdin);
    close(originalStdout);

    for (int i = 0; i < argc; i++) {
        free(argv[i]);  
    }
    free(argv);
}

void handleCommands(Node* head) {
    if (!head) return;

    char* command = head->word;

    // Check if the command is built-in
    if (strcmp(command, "cd") == 0) {
        if (getListSize(head) != 2) {
            fprintf(stderr, "cd: only one file name as argument.\n");
        } else {
            Node* argNode = head->next;
            if (chdir(argNode->word) != 0) {
                perror("cd");
            }
        }
    } else if (strcmp(command, "pwd") == 0) {
        char cwd[BUFFER_SIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
    } else if (strcmp(command, "which") == 0) {
        char* path = findExecutablePath(command);
        if (path != NULL) {
            printf("%s\n", path);
        }

    } else if (strcmp(command, "exit") == 0) {
        Node* ptr = head->next;
        while (ptr != NULL) {
            printf("%s ", ptr->word);
            ptr = ptr->next;
        }
        printf("\n");
        if (interactive) {
            printf("Exiting my shell.\n");
        }
        exit(0);
    } else {
        executeCommand(head);
    }
    
    freeTokenList(head);
}

void executePipe(Node* firstCommand, Node* secondCommand) {
    int pipefd[2];  
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {  
        close(pipefd[0]);          
        dup2(pipefd[1], STDOUT_FILENO);  
        close(pipefd[1]);           

        executeCommandNoFork(firstCommand); 
        exit(1);  
    }

    pid_t pid2 = fork();
    if (pid2 == 0) { 
        close(pipefd[1]);          
        dup2(pipefd[0], STDIN_FILENO); 
        close(pipefd[0]);           

        executeCommandNoFork(secondCommand);  
        exit(1);  
    }

    // parent process
    close(pipefd[0]);  
    close(pipefd[1]);

    int status1, status2;
    waitpid(pid1, &status1, 0);  
    waitpid(pid2, &status2, 0);  

    if (WIFEXITED(status2)) {
        int exitCode = WEXITSTATUS(status2);
        fprintf(stderr, "Last command exited with status %d\n", exitCode);
    } else if (WIFSIGNALED(status2)) {
        int signalNum = WTERMSIG(status2);
        fprintf(stderr, "Terminated by signal: %d\n", signalNum);
    }
}

// check if command has pipe and call function
void manageCommands(Node* head) {
    if (head == NULL) {
        return;
    }

    head = expandTokenList(head);

    // detect pipe in the command
    Node* prev = NULL;
    Node* pipeNode = head;
    while (pipeNode != NULL && strcmp(pipeNode->word, "|") != 0) {
        prev = pipeNode;
        pipeNode = pipeNode->next;
    }

    // if prev is null pipe is first token throw error
    if (prev == NULL) {
        fprintf(stderr, "Error: Pipe must connect two commands\n");
    }

    // handle pipe
    if (pipeNode != NULL) {  
        prev->next = NULL;
        Node* secondCommand = pipeNode->next;
        free(pipeNode->word);
        free(pipeNode);

        executePipe(head, secondCommand);
        return;
    }
    // if no pipe in the line handle normally
    else{
        handleCommands(head);
    }
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        exit(1);
    }

    int fd = STDIN_FILENO;

    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
    }

    interactive = isatty(fd);

    if (interactive) {
        printf("Welcome to my shell!\n");
    }

    char buffer[BUFFER_SIZE];
    int lineCapacity = BUFFER_SIZE;
    char* line = (char*)malloc(lineCapacity * sizeof(char));
    int lineLength = 0;
    
    while (1) {
        if (interactive) {
            printf("mysh> ");
            fflush(stdout);
        }

        ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            break;
        }

        buffer[bytes_read] = '\0';

        for (ssize_t i = 0; i < bytes_read; i++) {
            char c = buffer[i];
            if (c == '\n') { // End of line
                line[lineLength] = '\0';

                Node* head = createTokenList(line);
                if (head) {
                    manageCommands(head);
                }

                lineLength = 0;
            } else {
                if (lineLength >= lineCapacity - 1) {
                    lineCapacity *= 2;
                    line = realloc(line, lineCapacity * sizeof(char));
                    if (!line) {
                        perror("Failed to reallocate memory for line");
                        exit(1);
                    }
                }
                line[lineLength++] = c;
            }
        }
    }
    free(line);
    if (fd != STDIN_FILENO) {
        close(fd);
    }
    return EXIT_SUCCESS;
}
