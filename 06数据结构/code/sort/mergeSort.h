#include <stdio.h>
#include <vector>    
#include<Windows.h>

using namespace std;


void merge(vector<int>& nums,int p,int q, int r){
    vector<int> L(nums.begin()+p,nums.begin()+q);
    vector<int> R(nums.begin()+q,nums.begin()+r);
    int size = L.size()+R.size();
    int li = 0;
    int ri = 0;

    for(int i = p;i<r;i++){
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
            cout << "zhong :[";
            for(auto item:nums)
                cout<<","<<item;
            cout <<"]"<<endl;
            return;
        }
    }
    cout << "zhong :[";
    for(auto item:nums)
        cout<<","<<item;
    cout <<"]"<< p <<","<< q <<","<< r << endl;
}

void mergeSortEx(vector<int>& nums,int p,int r){
    if(p < r){
        int q = ( p + r)/2;
        mergeSortEx(nums,p,q);
        mergeSortEx(nums,q+1,r);
        merge(nums, p, q, r);
    }
}




void mergeSort(vector<int> &nums){
    DWORD start_time = GetTickCount(); 
    mergeSortEx(nums, 0, nums.size());
    DWORD end_time = GetTickCount();
    cout << "mergeSort  耗时：" << (end_time - start_time) << "ms" << endl;
}