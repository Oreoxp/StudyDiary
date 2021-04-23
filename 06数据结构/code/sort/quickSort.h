#include <stdio.h>
#include <vector>  

using namespace std;

int Pos(vector<int> &nums, int l, int r){
    int x = nums[r];
    int i = l-1;
    for(int j = l;j < r;j++){
        if(x <= nums[j]){
            i += 1;
            swap(nums[i],nums[j]);
        }
    }
    swap(nums[i+1],nums[r]);
    return i+1;
}

int RamdomPos(vector<int> &nums, int l, int r){
    int randnum = (rand() % (r-l+1))+ l;
    return Pos(nums, l, r);
}

void quickSortEx(vector<int> &nums, int l, int r){
    if(l < r){
        int random_pos = RamdomPos(nums, l, r);
        quickSortEx(nums, l, random_pos - 1);
        quickSortEx(nums, random_pos+1, r);
    }
}

void quickSort(vector<int> &nums){
    int start_time = clock(); 
    quickSortEx(nums, 0, nums.size() - 1 );
    int end_time = clock();
    cout << "quickSort          耗时：" << (end_time - start_time) << " ms" << endl;
}