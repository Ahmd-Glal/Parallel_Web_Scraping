/*
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


char RootURL[] = "https://www.lettercount.com/";
int depthLimit = 4;
const int threadCount = 10;
const int maxURLS = 1;

queue workingQueue;
queue answerQueue;

Trie myTrie;

pthread_mutex_t secuirty;
pthread_mutex_t secureAwaited;


void* parallelScraping(void* rank);
void Get_args(int argc, char* argv[]);
void Usage(char* prog_name);
bool extractURL(char* url, char* answer);

struct MemoryStruct {
    char* memory;
    size_t size;
};

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
    mem->memory[mem->size] = '\0'; // Use single quotes for character literals

    return totalSize;
}
sem_t urlSemaphore;*/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/*int* allowedURLs;

int main(int argc, char* argv[]) {


    //Get_args(argc, argv);

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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "Download failed: due to cite restrictions\n");
        curl_easy_cleanup(curl);
        if (chunk.memory != NULL)
            free(chunk.memory); // Free memory only if it was allocated
        return EXIT_FAILURE;
    }

    char* html_content = chunk.memory;
    char* line = strtok(html_content, "\n");
    allowedURLs = (int*)malloc(threadCount * sizeof(int));


    createQueue(&workingQueue);
    initializeTrie(&myTrie);

    while (line != NULL) {
        char answer[300];
        if (extractURL(line, answer) == true) {
            if (!searchURL(&myTrie, answer))
            {
                append(&workingQueue, answer, 0);
                insertURL(&myTrie, answer);
            }
        }
        line = strtok(NULL, "\n");
    }

    pthread_t* thread_handles;
    thread_handles = (pthread_t*)malloc(threadCount * sizeof(pthread_t));
    traverse(&workingQueue);

    sem_init(&urlSemaphore, 0, 0);

    pthread_mutex_init(&secuirty, NULL);
    pthread_mutex_init(&secureAwaited, NULL);

    //for (int thread = 0; thread < threadCount; thread++)
      //  pthread_create(&thread_handles[thread], NULL, parallelScraping, (void*)thread);


    //for (int thread = 0; thread < threadCount; thread++) 
        //pthread_join(thread_handles[thread], NULL);

    //traverse(&answerQueue);

    sem_destroy(&urlSemaphore);

    pthread_mutex_destroy(&secuirty);
    pthread_mutex_destroy(&secureAwaited);

    free(allowedURLs);
    free(thread_handles);
    // if(chunk.memory!=NULL)free(chunk.memory);

    curl_easy_cleanup(curl);

    return EXIT_SUCCESS;
}*/
/*
int numberOfAwaitedThreads = 0;
int done = 0;

void* parallelScraping(void* rank) {
    int x = 1;//allowedURLs[(int)rank]--
    while (x--) {

        char workingURL[301];
        int depth;
        int keepGoing = 1;

        pthread_mutex_lock(&secuirty);
        ////////////////////////////////////
        if (queueEmpty(&workingQueue)) {
            pthread_mutex_unlock(&secuirty);

            pthread_mutex_lock(&secureAwaited);
            numberOfAwaitedThreads++;
            if (numberOfAwaitedThreads == threadCount) {
                done = 1;
                for (int thread = 0; thread < threadCount; thread++) {
                    sem_post(&urlSemaphore);
                }
            }
            pthread_mutex_unlock(&secureAwaited);

            if (done)return NULL;
            sem_wait(&urlSemaphore);
            pthread_mutex_lock(&secuirty);
        }
        serve(&workingQueue, workingURL, &depth);
        append(&answerQueue, workingURL, depth);
        if (depth == depthLimit) {
            keepGoing = 0;
        }
        //////////////////////////////
        pthread_mutex_unlock(&secuirty);


        if (!keepGoing)continue;

        CURL* curl = curl_easy_init();

        if (!curl) {
            fprintf(stderr, "Curl initialization failed due to resource limitations\n");
            curl_easy_cleanup(curl);
            continue;
        }

        curl_easy_setopt(curl, CURLOPT_URL, workingURL);

        struct MemoryStruct chunk;
        chunk.memory = NULL;
        chunk.size = 0;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

        CURLcode result = curl_easy_perform(curl);

        if (result != CURLE_OK) {
            fprintf(stderr, "Download failed: due to cite restrictions\n");
            curl_easy_cleanup(curl);
            free(chunk.memory);
            continue;
        }

        char* html_content = chunk.memory;
        if (html_content == NULL) {
            fprintf(stderr, "Download failed: due to cite restrictions\n");
            curl_easy_cleanup(curl);
            free(chunk.memory);
            continue;
        }
        char* line = strtok(html_content, "\n");

        while (line != NULL) {
            char answer[400];
            if (extractURL(line, answer) == true) {
                pthread_mutex_lock(&secuirty);
                if (!searchURL(&myTrie, answer))
                {
                    append(&workingQueue, answer, depth + 1);
                    sem_post(&urlSemaphore);
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

bool extractURL(char* url, char* answer) {
    char* start, * end;

    start = strstr(url, "http://");
    if (start == NULL) {
        start = strstr(url, "https://");
    }

    if (start == NULL) {
        return false;
    }

    end = start;
    while (*end != '\0' && *end != ' ' && *end != '"' && *end != '\'' && *end != '<' && *end != '>') {
        end++;
    }

    int length = end - start;
    if (length <= 0 || length > 390) {
        return false; // Handle the case where the length is invalid
    }
    strncpy_s(answer, 390, start, length);
    answer[length] = '\0';

    return true;
}*/
/*
void Get_args(int argc, char* argv[]) {
    if (argc != 3) Usage(argv[0]);

    RootURL = argv[1];
    depthLimit = atoi(argv[2]);

    if (depthLimit < 0 || depthLimit > 10) Usage(argv[0]);
}



void Usage(char* prog_name) {
    fprintf(stderr, "usage: %s <please enter a vaild url and depth [1,10]>\n", prog_name);
    exit(0);
}*/


/*
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

                pthread_mutex_lock(&secuirty);
                if (!searchURL(&myTrie, answer)) {
                    int id = (int)rank + 1;
                    append(&answerQueue, answer, id);
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
*/

