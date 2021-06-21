#include <stdio.h>
#include <vector>

using namespace std;

#define RIGHT(i) ((i<<1)+1)
#define LEFT(i)  (i<<1)


void max_heapify(vector<int> &nums, int i){
    int l = LEFT(i);
    int r = RIGHT(i);
    int largest = i;
    int size = nums.size();
    if(l <= size && nums[l] > nums[i])
        largest = l;
    if(r <= size && nums[r] > nums[largest])
        largest = r;
    
    if(largest != i){
        swap(nums[i], nums[largest]);
        max_heapify(nums, largest);
    }
}

void buildMaxHeap(vector<int> &nums){
    for(int i = nums.size()/2; i >= 0; i--){
        max_heapify(nums, i);
    }
}

void heapSortEx(vector<int> &nums){
    buildMaxHeap(nums);
    for(int i = nums.size(); i > 1; i--){
        swap(nums[i], nums[0]);
        nums.pop_back();
        max_heapify(nums, 0);
    }
}

void heapSort(vector<int> &nums){
    int start_time = clock(); 
    heapSortEx(nums);
    int end_time = clock();
    cout << "heapSort           耗时：" << (end_time - start_time) << " ms" << endl;
}