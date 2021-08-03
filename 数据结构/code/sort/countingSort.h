#include <stdio.h>
#include <vector>  

using namespace std;
/*
计数排序思路：
    找出待排序的数组中最大和最小的元素
    统计数组中每个值为i的元素出现的次数，存入数组C的第i项
    对所有的计数累加（从C中的第一个元素开始，每一项和前一项相加）
    反向填充目标数组：将每个元素i放在新数组的第C(i)项，每放一个元素就将C(i)减去1
*/
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