#include <stdio.h>
#include <vector>  

using namespace std;
/*
快速排序思路：
    从数列随意中挑出一个元素，然后遍历数组，将小于该元素的值放元素左边，大于该元素的值放元素右边。
*/

int Pos(vector<int> &nums, int l, int r){
    int key = nums[r];
    int i = l-1;
    for(int j = l;j < r;j++){
        if(key <= nums[j]){
            i += 1;
            swap(nums[i],nums[j]);
        }
    }
    swap(nums[i+1],nums[r]);
    return i+1;
}

int RamdomPos(vector<int> &nums, int l, int r){
    int randnum = (rand() % (r-l+1))+ l;
    swap(nums[randnum], nums[r]);
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