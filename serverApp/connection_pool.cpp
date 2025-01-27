#include <iostream>
#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

class ConnectionPool{
private:
    std::string connString;
    int poolSize;
    std::condition_variable condVar;
    std::mutex mutex;
    std::queue<std::unique_ptr<pqxx::connection>> pool;

public:
    ConnectionPool(const std::string &connString, int poolSize) : connString(connString), poolSize(poolSize){
        for(int i=0;i<poolSize;i++){
            pool.push(std::make_unique<pqxx::connection>(connString));
        }
    }

    std::unique_ptr<pqxx::connection> getConnection(){
        std::unique_lock<std::mutex> lock(mutex);
        condVar.wait(lock, [this]() {return !pool.empty();});
        
        auto conn = std::move(pool.front());
        pool.pop();
        return conn;
    }
    
    void releaseConnection(std::unique_ptr<pqxx::connection> conn){
        std::lock_guard<std::mutex> lock(mutex);
        pool.push(std::move(conn));
        condVar.notify_one();
    }
};