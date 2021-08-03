#include <stdio.h>
#include <vector>    
//#include<Windows.h>

using namespace std;
/*
归并排序思路：
    将数组分为两部分，然后将两个数组合二为一。缩小递归上述操作。
*/

void merge(vector<int>& nums,int l,int mid, int r) {
    vector<int> L(nums.begin() + l, nums.begin() + mid);
    vector<int> R(nums.begin() + mid + 1, nums.begin() + r);
    int size = L.size() + R.size();
    int li = 0;
    int ri = 0;

    for(int i = l; i < r; i++) {
        if(li < L.size() && ri < R.size()) {
            if(L[li] >= R[ri]){
                nums[i] = L[li];
                li += 1;
            } else {
                nums[i] = R[ri];
                ri += 1;
            }
        }else if(li < L.size()){
            nums[i] = L[li];
            li += 1;
        }else if(ri < R.size()){
            nums[i] = R[ri];
            ri += 1;
        }else {
            return;
        }
    }
}

void mergeSortEx(vector<int>& nums,int l,int r){
    if(l < r){
        int mid = ( l + r ) / 2;
        mergeSortEx(nums, l, mid);
        mergeSortEx(nums, mid+1, r);
        merge(nums, l, mid, r);
    }
}




void mergeSort(vector<int> &nums){
    int start_time = clock(); 
    mergeSortEx(nums, 0, nums.size());
    int end_time = clock(); 
    cout << "mergeSort          耗时：" << (end_time - start_time) << " ms" << endl;

}