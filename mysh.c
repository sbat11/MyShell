#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

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

        if (c == ' ' || c == '\0') {  // End of a token
            if (wordLength > 0) {  // Only add non-empty tokens
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
                wordLength = 0;  // Reset for the next word
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
        free(temp->word);
        free(temp);
    }
}
char* findExecutablePath(const char* command) {
    // List of directories to search
    const char* searchPaths[] = {"/usr/local/bin", "/usr/bin", "/bin"};
    static char fullPath[BUFFER_SIZE];  // To store the full path of the executable

    for (int i = 0; i < 3; i++) {
        snprintf(fullPath, BUFFER_SIZE, "%s/%s", searchPaths[i], command);
        if (access(fullPath, X_OK) == 0) {  // Check if the file is executable
            return fullPath;
        }
    }
    return NULL;  // Command not found
}
void manageCommands(Node* head) {
    if (head == NULL) {
        printf("head is NULL\n");
        return;
    }
    printf("Command is: %s\n", head->word);

    char* command = head->word;

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
    } else if (strcmp(command, "exit") == 0) {
        exit(0);
    } else {  // Handle external commands
        char* executablePath = NULL;

        // If the command contains a slash, treat it as a direct path
        if (strchr(command, '/') != NULL) {
            executablePath = command;
        } else {  // Search for bare names
            executablePath = findExecutablePath(command);
        }

        if (executablePath != NULL) {
            // Build argument list
            int argc = getListSize(head);
            char** argv = (char**)malloc((argc + 1) * sizeof(char*));
            Node* current = head;
            for (int i = 0; i < argc; i++) {
                argv[i] = current->word;
                current = current->next;
            }
            argv[argc] = NULL;  // Null-terminate the argument list

            // Fork and execute
            pid_t pid = fork();
            if (pid == 0) {  // Child process
                execv(executablePath, argv);
                perror("execv");  // If execv fails
                exit(1);
            } else if (pid > 0) {  // Parent process
                wait(NULL);  // Wait for child process to complete
            } else {
                perror("fork");
            }

            free(argv);
        } else {
            fprintf(stderr, "%s: command not found\n", command);
        }
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

    int interactive = isatty(fd);
    if (interactive) {
        printf("Welcome to my shell!\n");
    }

    char buffer[BUFFER_SIZE];
    char* line = (char*)malloc(BUFFER_SIZE * sizeof(char));
    int lineLength = 0;
    int lineCapacity = BUFFER_SIZE;

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
            if (c == '\n' || c == '\0') { // End of line
                line[lineLength] = '\0';

                Node* head = createTokenList(line);
                if (head) {
                    manageCommands(head);
                    freeTokenList(head);
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
