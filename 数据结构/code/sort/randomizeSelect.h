#include <stdio.h>
#include <vector>  

using namespace std;
/*
选择排序思路：
    首先在未排序序列中找到最小（大）元素，存放到排序序列的起始位置。
    再从剩余未排序元素中继续寻找最小（大）元素，然后放到已排序序列的末尾。
    重复第二步，直到所有元素均排序完毕。
*/

int rsPos(vector<int> &nums, int l, int r){
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

int rsRandomPos(vector<int> &nums, int l, int r){
    int randnum = (rand() % (r-l+1))+ l;
    return rsPos(nums, l, r);
}

int randomizedSelectEx(vector<int> &nums, int l, int r, int i){
    if(l == r)
        return nums[l];
    int mid = rsRandomPos(nums, l, r);
    int k = mid - l + 1;
    if(i == k)
        return nums[mid];
    else if(i < k)
        return randomizedSelectEx(nums, l, mid-1, i);
    else 
        return randomizedSelectEx(nums, mid+1, r, i-k);
}

void randomizedSelect(vector<int> &nums, int i){
    int start_time = clock(); 
    int ret  = randomizedSelectEx(nums, 0, nums.size() - 1, i);
    int end_time = clock();
    cout << "randomizedSelect   耗时：" << (end_time - start_time) << " ms ret="<<ret<< endl;
}