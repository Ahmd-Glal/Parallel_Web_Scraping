#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define ALPHABET_SIZE 300

typedef struct node
{
    struct node* branches[ALPHABET_SIZE];
    bool isEnd;
} node;

typedef struct Trie
{
    node* root;
    int count;
} Trie;

node* createNode()
{
    node* newNode = (node*)malloc(sizeof(node));
    if (newNode != NULL)
    {
    newNode->isEnd = false;
    for (int i = 0; i < ALPHABET_SIZE; ++i)
    {
        newNode->branches[i] = NULL;
    }
    }
    return newNode;
}

void initializeTrie(Trie* myTrie)
{
    myTrie->root = createNode();
    myTrie->count = 0;
}

void insertURL(Trie* myTrie, char* url)
{
    int sz = strlen(url);
    node* currentPOS = myTrie->root;
    for (int i = 0; i < sz; i++)
    {
        if (currentPOS->branches[url[i]] == NULL)
        {
            currentPOS->branches[url[i]] = createNode();
        }
        currentPOS = currentPOS->branches[url[i]];
    }
    currentPOS->isEnd = true;
    myTrie->count++;
}

void freeTrie(node* currentNode)
{
    if (currentNode == NULL)
    {
        return;
    }
    for (int i = 0; i < ALPHABET_SIZE; ++i)
    {
        freeTrie(currentNode->branches[i]);
    }
    free(currentNode);
}

void destroyTrie(Trie* myTrie)
{
    freeTrie(myTrie->root);
}

bool searchURL(Trie* myTrie, char* url)
{
    int sz = strlen(url);
    node* currentPOS = myTrie->root;
    for (int i = 0; i < sz; i++)
    {
        if (currentPOS->branches[url[i]] == NULL)
        {
            return false;
        }
        currentPOS = currentPOS->branches[url[i]];
    }
    return currentPOS->isEnd;
}
