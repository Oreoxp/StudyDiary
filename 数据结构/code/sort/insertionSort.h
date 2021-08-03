#include <stdio.h>
#include <vector>    
//#include<Windows.h>

using namespace std;
/*
插入排序思路：
    将第一待排序序列第一个元素看做一个有序序列，把第二个元素到最后一个元素当成是未排序序列。
    从头到尾依次扫描未排序序列，将扫描到的每个元素插入有序序列的适当位置。
    （ 如果待插入的元素与有序序列中的某个元素相等，则将待插入元素插入到相等元素的后面。）
*/
void instertionSortEx(vector<int> &nums){
    
    for(int i = 1;i < nums.size();i++) {
        int j = i - 1;
        int key = nums[i];

        while(j >= 0 && key > nums[j]) {
            nums[j+1] = nums[j];
            j -= 1;
        }
        nums[j+1] = key;
    }
}

void instertionSort(vector<int> &nums){
    int start_time = clock(); 
    instertionSortEx(nums);
    int end_time = clock();
    cout << "instertionSort     耗时：" << (end_time - start_time) << " ms" << endl;
}

