#include<stdio.h>
#include<string>
#include<map>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <functional>

#include <iostream>

using namespace std;

typedef const std::lock_guard<std::mutex>  LockGuard;

class Stock {
public:
    Stock(string str){
        cout << "Stock Create:" << str << endl;
        key_ = str;
    }

    string key(){
        return key_;
    }
private:
    string key_;
};


class StockFactory : public std::enable_shared_from_this<StockFactory> {
public:
    shared_ptr<Stock> get(const string& key) {
        shared_ptr<Stock> pStock;
	    LockGuard lock(mutex_) ;
	    weak_ptr<Stock>& wkStock = stocks_[key];    // 如果 key 不存在, 会默认构造一个

	    pStock = wkStock.lock();                    //尝试把“棉线”提升为“铁丝”

        cout << "get"<<endl;
	    if (!pStock) {
		    pStock.reset(new Stock(key) ,
						std::bind(&StockFactory::weakDeleteCallback,
												shared_from_this(),
												std::placeholders::_1));
		    wkStock = pStock;                       // 这里更新了 stocks_[key], 注意 wkStock 是个引用
        }
	    return pStock;
    }

private:
    static void weakDeleteCallback(const weak_ptr<StockFactory>& wkFactory, Stock* stock){
        cout << "StockFactory callback"<< endl;
        shared_ptr<StockFactory> sptr = wkFactory.lock();

        if(sptr){
            sptr->removeStock(stock);
        }
        
        delete stock;
    }

    void removeStock(Stock* stock) {
        if(stock) {
            LockGuard lock(mutex_);
            cout << "StockFactory callback  clear:" << stock->key()<< endl;
            stocks_.erase( stock->key() );
        }
    }
private:
    std::mutex mutex_;
    std::map<string, weak_ptr<Stock> > stocks_;
};

int main(){
    shared_ptr<StockFactory> factory(new StockFactory) ;
    {
        shared_ptr<Stock> stock = factory->get( "NYSE:IBM") ;
        shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
        assert(stock == stock2);
        // stock destructs here
    }
    // factory destructs here

    return 0;
}