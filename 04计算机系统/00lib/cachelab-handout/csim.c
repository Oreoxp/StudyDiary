#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>

#include <getopt.h> // get param form command
#include <unistd.h> // get param form command

#define bool int
#define true 1
#define false 0
typedef unsigned long int u_int64_t;


typedef struct 
{
    bool valid;
    u_int64_t tag;
    int count;
}line;

typedef struct 
{
    int s;
    int E;//line count
    line *lines;
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

void initCache(cache *inCache, int s, int E)
{
    inCache->E = E;
    inCache->s = s;
    int numb = 1 << s;
    numb = numb*E;
    inCache->lines = calloc(numb, sizeof(line)); //根据s，E可以访问到数组任意位置，E为列，2^s为行
    printf("initCache : cache = { E = %d , s = %d , line count = %d }\n",inCache->E , inCache->s ,numb);
}

void realseMemory(cache *inCache)
{
    int num = inCache->E * inCache->s;
    printf("realseMemory: %d",num);
    free(inCache->lines);
    free(inCache);
}

void missHit(int sSetIndex, u_int64_t tBit,cache *inCache,result *inResult)
{
    line *leastLine = &inCache->lines[sSetIndex];
    line *pLine = NULL;
    int minCount = 0;
    for (int i = 0; i < inCache->E; i++) {

        printf("check lines : sSetIndex = %d , tBit = %ld \n" ,sSetIndex ,tBit);
        
        pLine = &inCache->lines[sSetIndex+i];

        // true && t = t
        if (pLine->valid && pLine->tag == tBit)
        {
            printf("hit\n");
            ++pLine->count;
            ++inResult->hitCount;
            return;
        }

        // false
        if (pLine->valid == false)
        {
            printf("miss\n");
            pLine->valid = true;
            pLine->tag = tBit;
            ++pLine->count;
            ++inResult->misCount;
            return;
        }
        
        // true
        leastLine = minCount >= pLine->count ? pLine : leastLine;
        minCount = minCount >= pLine->count ? pLine->count : minCount;
        
    }

    printf("replace\n");
    leastLine->tag = tBit;
    ++leastLine->count;
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

    char contiueFlag;
    char identifier;
    u_int64_t address;
    int size;
 
    while (fscanf(pFile, "%c %c %lx,%d", &contiueFlag, &identifier, &address, &size) > 1)
    {
        printf("read file : %c %lx,%d \n" , identifier, address, size);

        if (contiueFlag == 'I')
            continue;
        sSetIndex = (address >> b) & sBitMask;
        tBit = (address >> b) >> s;
        sSetIndex = inCache->E * sSetIndex;
        switch (identifier)
        {

        case 'M':
            printf("operator : M %lx,%d\n",address,size);
            missHit(sSetIndex, tBit, inCache, inResult);
            missHit(sSetIndex, tBit, inCache, inResult);
            break;
        case 'L':
            printf("operator : L %lx,%d\n",address,size);
            missHit(sSetIndex, tBit, inCache, inResult);
            break;
        case 'S':
            printf("operator : S %lx,%d\n",address,size);
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

    if (pFile == NULL) {
        printf("open file :open file faide!\n");
        // return 0;
    } else {
        printf("open file :file open succ!\n");
    }

    cache *myCache = calloc(1, sizeof(cache));
    result myResult;
    initResult(&myResult ,0,0,0);
    initCache(myCache, s, E);
    readAndTest(b, pFile, myCache,&myResult);
    printf("hitCount:%d,misCount:%d,eviCount:%d\n",myResult.hitCount,myResult.misCount,myResult.eviCount);
    realseMemory(myCache);

    if (pFile != NULL)
        fclose(pFile);
    printSummary(myResult.hitCount, myResult.misCount, myResult.eviCount);
    return 0;
}
