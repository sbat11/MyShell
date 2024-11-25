#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

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

    int inWord = 0;
    int wordLength = 0;
    int wordCapacity = BUFFER_SIZE;
    char* word = (char*)malloc(wordCapacity * sizeof(char));
    if (!word) {
        perror("Failed to allocate memory for word");
        return NULL;
    }

    for (int i = 0; i <= strlen(line); i++) {
        char c = line[i];

        if ((c == ' ' || c == '\0') && inWord) { // End of a word
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

            inWord = 0;
            wordLength = 0;
        } else if (c != ' ' && c != '\0') { // Part of a word
            if (wordLength >= wordCapacity - 1) {
                wordCapacity *= 2;
                word = realloc(word, wordCapacity * sizeof(char));
                if (!word) {
                    perror("Failed to reallocate memory for word");
                    return NULL;
                }
            }
            word[wordLength++] = c;
            inWord = 1;
        }
    }

    free(word); // Free temporary word buffer
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
    } else {
        printf("Command not recognized: %s\n", command);
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
