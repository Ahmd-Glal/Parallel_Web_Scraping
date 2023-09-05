#define HAVE_STRUCT_TIMESPEC
#define CURL_STATICLIB
#define _CRT_SECURE_NO_WARNINGS

#include <curl/curl.h>

#include <pthread.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "queue.h"
#include "trie.h"


int depthLimit;
char* RootURL;
int threadCount=10;

queue workingQueue;
queue answerQueue;

Trie myTrie;

pthread_mutex_t secuirty;


void* parallelScraping(void* rank);
void Get_args(int argc, char* argv[]);
void Usage(char* prog_name);
bool extractURL(char* url, char* answer);

struct MemoryStruct {
    char* memory;
    size_t size;
};

size_t write_data(void* ptr, size_t size, size_t count, void* userdata) {
    size_t totalSize = size * count;
    struct MemoryStruct* mem = (struct MemoryStruct*)userdata;

    mem->memory = (char*)realloc(mem->memory, mem->size + totalSize + 1);
    if (mem->memory == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), ptr, totalSize);
    mem->size += totalSize;
    mem->memory[mem->size] = 0;

    return totalSize;
}

int main(int argc, char* argv[]) {


    Get_args(argc, argv);
    CURL* curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "Curl initialization failed\n");
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
        fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(result));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return EXIT_FAILURE;
    }

    char* html_content = chunk.memory;

    char* line = strtok(html_content, "\n");
    


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

    pthread_mutex_init(&secuirty, NULL);

    for (int thread = 0; thread < threadCount; thread++)
        pthread_create(&thread_handles[thread], NULL, parallelScraping, (void*)thread);

    for (int thread = 0; thread < threadCount; thread++)
        pthread_join(thread_handles[thread], NULL);

    pthread_mutex_destroy(&secuirty);

    free(thread_handles);
    free(chunk.memory);
    curl_easy_cleanup(curl);

    traverse(&answerQueue);

    return EXIT_SUCCESS;
}



void* parallelScraping(void* rank) {
    while (true) {
        char workingURL[301];
        int depth;
        int keepGoing = 1;
        pthread_mutex_lock(&secuirty);
        ////////////////////////////////////
        if (queueEmpty(&workingQueue))
            return NULL;
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
            fprintf(stderr, "Curl initialization failed\n");
            return NULL;
        }

        curl_easy_setopt(curl, CURLOPT_URL, workingURL);

        struct MemoryStruct chunk;
        chunk.memory = NULL;
        chunk.size = 0;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

        CURLcode result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(result));
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return NULL;
        }

        char* html_content = chunk.memory;

        char* line = strtok(html_content, "\n");

        while (line != NULL) {
            char answer[300];
            if (extractURL(line, answer) == true) {
                pthread_mutex_lock(&secuirty);
                if (!searchURL(&myTrie, answer))
                {
                    append(&workingQueue, answer, depth + 1);
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

    strncpy_s(answer, 300, start, length);
    answer[length] = '\0';

    return true;
}

void Get_args(int argc, char* argv[]) {
	if (argc != 3) Usage(argv[0]);

	RootURL = argv[1];
	depthLimit = atoi(argv[2]);

	if (depthLimit <= 0 || depthLimit > 10) Usage(argv[0]);
}



void Usage(char* prog_name) {
	fprintf(stderr, "usage: %s <please enter a vaild url and depth [1,10]>\n", prog_name);
	exit(0);
}
