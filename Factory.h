#ifndef FACTORY_H_
#define FACTORY_H_

#include <pthread.h>
#include <list>
#include <map>
#include "Product.h"

//class Thread {
//public:
//    pthread_t* thread_ptr;
//    int thread_id;
//};

class Factory {
private:
    bool open_to_returns;
    bool open_to_visitors;
    std::list<std::pair<Product, int>> stolen_products;

    // the threads currently running
    std::map<unsigned int, pthread_t *> production_threads;
    std::map<unsigned int, pthread_t *> simple_buyer_threads;
    std::map<unsigned int, pthread_t *> company_threads;
    std::map<unsigned int, pthread_t *> thief_threads;

    // the factory's available products
    unsigned int num_available_products;
    std::list<Product> available_products;
    bool products_being_edited;
    pthread_cond_t products_cond;
    pthread_mutex_t products_lock;
    pthread_mutexattr_t products_lock_attributes; // for initialization purposes



public:
    Factory();

    ~Factory();

    void startProduction(int num_products, Product *products, unsigned int id);

    void produce(int num_products, Product *products);

    void finishProduction(unsigned int id);

    void startSimpleBuyer(unsigned int id);

    int tryBuyOne();

    int finishSimpleBuyer(unsigned int id);

    void startCompanyBuyer(int num_products, int min_value, unsigned int id);

    std::list<Product> buyProducts(int num_products);

    void returnProducts(std::list<Product> products, unsigned int id);

    int finishCompanyBuyer(unsigned int id);

    void startThief(int num_products, unsigned int fake_id);

    int stealProducts(int num_products, unsigned int fake_id);

    int finishThief(unsigned int fake_id);

    void closeFactory();

    void openFactory();

    void closeReturningService();

    void openReturningService();

    std::list<std::pair<Product, int>> listStolenProducts();

    std::list<Product> listAvailableProducts();

//    void updateProductionThreads(unsigned int id, int thread_id);
//    void updateThiefThreads();
//    void updateCompanyThreads();

    void removeProductionThreadFromList(unsigned int id);

    void removeSimpleBuyerThreadByID(unsigned int id);

    void removeThiefThreadByID(unsigned int id);

};

#endif // FACTORY_H_
