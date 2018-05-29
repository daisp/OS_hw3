#ifndef FACTORY_H_
#define FACTORY_H_

#include <pthread.h>
#include <list>
#include <map>
#include "Product.h"

class Factory {
private:
    pthread_mutex_t open_to_visitors_lock;
    pthread_mutexattr_t open_to_visitors_lock_attributes; // for init purposes
    pthread_cond_t open_to_visitors_cond;
    bool open_to_visitors;

    pthread_mutex_t open_to_returns_lock;
    pthread_mutexattr_t open_to_returns_lock_attributes; // for init purposes
    pthread_cond_t open_to_returns_cond;

    bool open_to_returns;


    // the threads currently running and their locks
    std::map<unsigned int, pthread_t *> production_threads;
//    pthread_mutex_t production_threads_lock;
//    pthread_mutexattr_t production_threads_lock_attributes; // for initialization purposes

    std::map<unsigned int, pthread_t *> simple_buyer_threads;
//    pthread_mutex_t simple_buyer_threads_lock;
//    pthread_mutexattr_t simple_buyer_threads_lock_attributes; // for initialization purposes

    std::map<unsigned int, pthread_t *> company_buyer_threads;
//    pthread_mutex_t company_buyer_threads_lock;
//    pthread_mutexattr_t company_buyer_threads_lock_attributes; // for initialization purposes

    std::map<unsigned int, pthread_t *> thief_threads;
//    pthread_mutex_t thief_threads_lock;
//    pthread_mutexattr_t thief_threads_lock_attributes; // for initialization purposes

    // the factory's available and stolen products, and their lock
    std::list<Product> available_products;
    std::list<std::pair<Product, int>> stolen_products;
    bool products_being_edited;
    pthread_cond_t products_cond;
    pthread_mutex_t products_lock;
    pthread_mutexattr_t products_lock_attributes; // for initialization purposes

    void removeProductionThreadFromList(unsigned int id);

    void removeSimpleBuyerThreadFromList(unsigned int id);

    void removeCompanyThreadFromList(unsigned int id);

    void removeThiefThreadFromList(unsigned int id);

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
};

#endif // FACTORY_H_
