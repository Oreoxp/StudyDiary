#include <stdio.h>
#include <vector>    
//#include<Windows.h>

using namespace std;

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

