#define HAVE_STRUCT_TIMESPEC
#define CURL_STATICLIB
#define _CRT_SECURE_NO_WARNINGS

#include <curl/curl.h>

#include <pthread.h>
#include<semaphore.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "queue.h"
#include "trie.h"


#define MAX_URL_LENGTH 400
/*
https://www.lettercount.com/
https://github.com/yusuzech/r-web-scraping-cheat-sheet
*/

char RootURL[] = "https://github.com/yusuzech/r-web-scraping-cheat-sheet";

int* allowedNumberOfWork;
int maxAllowed = 40;

int depthLimit = 4;
const int threadCount = 10;

queue workingQueue;
queue answerQueue;
Trie myTrie;

struct MemoryStruct {
    char* memory;
    size_t size;
};

void* parallelScraping(void* rank);
bool extractURL(const char* line, char* answer, size_t max_length);
size_t write_data(void* ptr, size_t size, size_t count, void* userdata);


pthread_mutex_t secuirty;
pthread_mutex_t secureAwaited;

sem_t urlSemaphore;

int main() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Curl initialization failed due to resource limitations\n");
        return EXIT_FAILURE;
    }

    curl_easy_setopt(curl, CURLOPT_URL, RootURL);

    struct MemoryStruct chunk;
    chunk.memory = NULL;
    chunk.size = 0;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(result));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return EXIT_FAILURE;
    }

    char* html_content = chunk.memory;
    if (html_content == NULL) {
        fprintf(stderr, "Download failed: Empty content\n");
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return EXIT_FAILURE;
    }

    createQueue(&workingQueue);
    createQueue(&answerQueue);

    initializeTrie(&myTrie);

    char* line = strtok(html_content, "\n");
    while (line != NULL) {
        char answer[MAX_URL_LENGTH];
        if (extractURL(line, answer, MAX_URL_LENGTH)) {
            if (!searchURL(&myTrie, answer)) {
                append(&workingQueue, answer, 0);
                insertURL(&myTrie, answer);
            }
        }
        line = strtok(NULL, "\n");
    }
    
    if (queueEmpty(&workingQueue)) {
        printf("sorry no links provided\n");
        return EXIT_SUCCESS;
    }

    pthread_t* thread_handles;
    thread_handles = (pthread_t*)malloc(threadCount * sizeof(pthread_t));
    
    allowedNumberOfWork = (int*)malloc(threadCount * sizeof(int));
    for (int i = 0; i < threadCount; i++)allowedNumberOfWork[i] = maxAllowed;

    sem_init(&urlSemaphore, 0, 0);
    pthread_mutex_init(&secuirty, NULL);

    for (int thread = 0; thread < threadCount; thread++)
       pthread_create(&thread_handles[thread], NULL, parallelScraping, (void*)thread);


    for (int thread = 0; thread < threadCount; thread++) 
        pthread_join(thread_handles[thread], NULL);

    traverse(&answerQueue);

    pthread_mutex_destroy(&secuirty);
    sem_destroy(&urlSemaphore);
    destroyTrie(&myTrie);
    clearQueue(&workingQueue);
    clearQueue(&answerQueue);
    free(chunk.memory);
    curl_easy_cleanup(curl);

    return EXIT_SUCCESS;
}

bool extractURL(const char* line, char* answer, size_t max_length) {
    const char* start = strstr(line, "http://");
    if (!start) {
        start = strstr(line, "https://");
    }

    if (!start) {
        return false;
    }

    const char* end = start;
    while (*end && *end != ' ' && *end != '"' && *end != '\'' && *end != '<' && *end != '>') {
        end++;
    }

    size_t length = end - start;
    if (length > 0 && length <= max_length) {
        strncpy(answer, start, length);
        answer[length] = '\0';
        return true;
    }

    return false;
}
size_t write_data(void* ptr, size_t size, size_t count, void* userdata) {
    struct MemoryStruct* mem = (struct MemoryStruct*)userdata;

    if (!mem || !ptr || size == 0 || count == 0) {
        fprintf(stderr, "Invalid input or memory allocation failed\n");
        return 0;
    }

    size_t totalSize = size * count;
    char* newMemory = (char*)realloc(mem->memory, mem->size + totalSize + 1);

    if (!newMemory) {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }

    memcpy(&(newMemory[mem->size]), ptr, totalSize);
    mem->memory = newMemory;
    mem->size += totalSize;
    mem->memory[mem->size] = '\0';

    return totalSize;
}

int numberOfAwaitedThreads = 0, done = 0;
void* parallelScraping(void* rank) {
    int myRank = (int)rank;
    int amountOfWork = allowedNumberOfWork[myRank];
    for (int i = 0; i < amountOfWork; i++) {
        char workingURL[301];
        int depth;

        pthread_mutex_lock(&secuirty);
        if (queueEmpty(&workingQueue)) {

            numberOfAwaitedThreads++;
            if (numberOfAwaitedThreads == threadCount) {
                done = 1;
            }
            else {
                pthread_mutex_unlock(&secuirty);
                sem_wait(&urlSemaphore);
                pthread_mutex_lock(&secuirty);
                numberOfAwaitedThreads--;
            }
            if (done) {
                sem_post(&urlSemaphore);
                pthread_mutex_unlock(&secuirty);
                return NULL;
            }
        }
        serve(&workingQueue, workingURL, &depth);
        append(&answerQueue, workingURL, depth);

        pthread_mutex_unlock(&secuirty);

        if (depth == depthLimit) {
            continue;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            fprintf(stderr, "Curl initialization failed due to resource limitations\n");
            return NULL;
        }

        curl_easy_setopt(curl, CURLOPT_URL, RootURL);

        struct MemoryStruct chunk;
        chunk.memory = NULL;
        chunk.size = 0;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

        CURLcode result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(result));
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return NULL;
        }

        char* html_content = chunk.memory;
        if (html_content == NULL) {
            fprintf(stderr, "Download failed: Empty content\n");
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return NULL;
        }

        char* line = strtok(html_content, "\n");
        while (line != NULL) {
            char answer[MAX_URL_LENGTH];
            if (extractURL(line, answer, MAX_URL_LENGTH)) {
                int num = numberOfAwaitedThreads;
                pthread_mutex_lock(&secuirty);
                if (!searchURL(&myTrie, answer)) {
                    append(&workingQueue, answer, depth + 1);
                    if (num) { sem_post(&urlSemaphore);}
                    insertURL(&myTrie, answer);
                }
                pthread_mutex_unlock(&secuirty);
            }
            line = strtok(NULL, "\n");
        }

        free(chunk.memory);
        curl_easy_cleanup(curl);
    }
}
