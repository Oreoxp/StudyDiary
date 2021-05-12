#include <stdio.h>
#include <vector>  

using namespace std;

vector<int> countingSortEx(vector<int> &nums, int maxnum){
    int size = nums.size();
    vector<int> outnums(size, 0);
    vector<int> midnums(maxnum+1, 0);

    for(int i = 0;i<size;i++){
        midnums[nums[i]] += 1;
    }

    for(int i = 1;i<midnums.size();i++){
        midnums[i] += midnums[i-1];
    }

    for(int i = size-1 ;i >= 0;i--){
        outnums[midnums[nums[i]]] = nums[i];
        midnums[nums[i]] -= 1;
    }
    return outnums;
}

vector<int> countingSort(vector<int> &nums, int max){
    int start_time = clock(); 
    auto vec = countingSortEx(nums, max);
    int end_time = clock();
    cout << "countingSort        耗时：" << (end_time - start_time) << " ms" << endl;
    return vec;
}