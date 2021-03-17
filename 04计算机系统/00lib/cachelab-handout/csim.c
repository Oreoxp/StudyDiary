#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>

#include <getopt.h> // get param form command
#include <unistd.h> // get param form command

#define bool int
#define true 1
#define false 0
typedef unsigned long int u_int64_t;


typedef struct {
    bool valid;
    u_int64_t tag;
    int count;
}cache_line;

typedef struct {
    cache_line *lines;
}cache_set;

typedef struct {
    int s;
    int E;//line count
    cache_set *sets;
}cache;

typedef struct {
    int hitCount;
    int misCount;
    int eviCount;
    /* data */
}result;

void initResult(result* myresult, int hit_count , int mis_count , int evi_count) {
    myresult->hitCount = hit_count;
    myresult->misCount = mis_count;
    myresult->eviCount = evi_count;
    printf("initResult : result = { %d , %d , %d }\n", myresult->hitCount,myresult->misCount,myresult->eviCount);
}

void initCache(cache *inCache, int s, int E) {
    printf("initCache\n");
    inCache->s = s;
    inCache->E = E;
}

void initSet(cache *inCache, int E) {
    printf("initSet\n");
    int S = 1 << inCache->s;
    if ((inCache->sets = calloc(S, sizeof(cache_set))) == NULL) {
        printf("initSet : set init error");
    }
    for (size_t i = 0; i < S; i++) {
        if ((inCache->sets[i].lines = calloc(E, sizeof(cache_line))) == NULL) {
            printf("initSet : line init error");
        }
    }
}

void printSet(cache *inCache){
    int S = 2;//1 << inCache->s;

    printf("*********printf all Set**************\n");
    for (size_t i = 0; i < S; i++) {
        printf("set %ld :\n[\n",i);
        cache_set c_set = inCache->sets[i];
        for (size_t j = 0; j < inCache->E; j++) {
            printf("line[%ld]  : valid = %d, tag = %lu \n",j,c_set.lines[j].valid,c_set.lines[j].tag);
        }
        printf("]\n");
    }
    
    printf("*********printf over*****************\n");
}

void realseMemory(cache *inCache) {
    //int num = inCache->E * inCache->s;
    //printf("realseMemory: %d",num);
    free(inCache->sets->lines);
    free(inCache->sets);
    free(inCache);
}

void missHit(int sSetIndex, u_int64_t tBit,cache *inCache,result *inResult) {
    cache_set *find_set = &inCache->sets[sSetIndex];
    cache_line *find_set_last_line = &find_set->lines[0];
    cache_line *pLine = NULL;

    int minCount = 0;
    //printSet(inCache);
    for (int i = 0; i < inCache->E; i++) {

        //printf("check lines : sSetIndex = %d , tBit = %ld \n" ,sSetIndex ,tBit);
        
        pLine = &find_set->lines[i];

        // true && t = t
        if (pLine->valid == true && pLine->tag == tBit)
        {
            printf("hit\n}\n\n\n");
            ++pLine->count;
            ++inResult->hitCount;
            return;
        }

        // false
        if (pLine->valid == false)
        {
            printf("miss\n}\n\n\n");
            pLine->valid = true;
            pLine->tag = tBit;
            ++pLine->count;
            ++inResult->misCount;
            return;
        }
        
        // true
        find_set_last_line = minCount >= pLine->count ? pLine : find_set_last_line;
        minCount = minCount >= pLine->count ? pLine->count : minCount;
        
    }

    printf("replace\n}\n\n\n");
    find_set_last_line->tag = tBit;
    ++find_set_last_line->count;
    //printSet(inCache);
    ++inResult->misCount;
    ++inResult->eviCount;
    return;
}

void readAndTest(int b,FILE *pFile, cache *inCache,result* inResult) {
    int s = inCache->s;
    //int E = inCache->E;
    u_int64_t sBitMask = (1 << s) - 1; // 这里的s是幂"2^s"
    u_int64_t sSetIndex;
    u_int64_t tBit;

    char identifier;
    u_int64_t address;
    int size;
 
    while (fscanf(pFile, " %c %lx,%d[^\n]", &identifier, &address, &size) != -1)
    {
        //printf("{\nread file : %c %lx,%d \n" , identifier, address, size);

        if (identifier == 'I')
            continue;
        sSetIndex = (address >> b) & sBitMask;
        tBit = (address >> b) >> s;
        //sSetIndex = inCache->E * sSetIndex;
        switch (identifier) {
        case 'M':
            printf("operator : M %lx,%d  ",address,size);
            missHit(sSetIndex, tBit, inCache, inResult);
            missHit(sSetIndex, tBit, inCache, inResult);
            break;
        case 'L':
            printf("operator : L %lx,%d  ",address,size);
            missHit(sSetIndex, tBit, inCache, inResult);
            break;
        case 'S':
            printf("operator : S %lx,%d  ",address,size);
            missHit(sSetIndex, tBit, inCache, inResult);
            break;
        default:
            break;
        }
    }
}

int main(int argc, char **argv) {
    int opt, s, E, b;
    FILE *pFile;

    //read con
    while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))) {
        switch (opt) {
        case 'h':
            printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n\
            -h: Optional help flag\n\
            -v: Optional verbose flag that displays trace info\n\
            -s <s>: Number of set index bits(S = 2s is the number of set)\n\
            -E <E>: Associtivity (number of lines per set)\n\
            -b <b>: Number of block bits (B = 2b is the block size)\n\
            -t <tracefile>: Name of the valgrind trace to repla\n");
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 'v':
            // vFlag = 1;
            printf("show step!\n");
            break;
        case 't':
            pFile = fopen((const char *)optarg, "r");
            printf("open file : %s\n" , (const char *)optarg);
            break;
        default:
            break;
        }
    }

    //open file
    if (pFile == NULL) {
        printf("open file :open file faide!\n");
        // return 0;
    } else {
        printf("open file :file open succ!\n");
    }

    cache *myCache = calloc(1, sizeof(cache));
    result myResult;

    //init
    printf("*************************** init start ***************************\n");
    initResult(&myResult ,0,0,0);
    initCache(myCache, s, E);
    initSet(myCache,E);
    printf("*************************** init over ***************************\n");

    //read file and run
    readAndTest(b, pFile, myCache,&myResult);
    printf("hitCount:%d,misCount:%d,eviCount:%d\n",myResult.hitCount,myResult.misCount,myResult.eviCount);

    //realse
    realseMemory(myCache);

    if (pFile != NULL)
        fclose(pFile);
    printSummary(myResult.hitCount, myResult.misCount, myResult.eviCount);
    return 0;
}
