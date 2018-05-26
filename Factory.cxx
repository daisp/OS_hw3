#include "Factory.h"
#include <vector>
#include <map>


class ProductionArgument {
public:
    ProductionArgument(Factory *factory, int num_products, Product *products,
                       unsigned int id)
            : factory(factory), num_products(num_products),
              products(products), id(id) {}

    ~ProductionArgument() = default;

    Factory *factory;
    int num_products;
    Product *products;
    unsigned int id;
};

Factory::Factory() : open_to_returns(true), open_to_visitors(true),
                     products_being_edited(false) {
    pthread_mutexattr_init(&products_lock_attributes);
    pthread_mutexattr_settype(&products_lock_attributes,
                              PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&products_lock, &products_lock_attributes);
    pthread_cond_init(&products_cond, nullptr);
}

void *ProductionFunc(void *arg) {
    auto factory = static_cast<ProductionArgument *>(arg)->factory;
    auto num_products = static_cast<ProductionArgument *>(arg)->num_products;
    auto products = static_cast<ProductionArgument *>(arg)->products;
    auto id = static_cast<ProductionArgument *>(arg)->id;
    factory->Factory::produce(num_products, products);
    delete factory->removeProductionThreadByID(id);
    delete static_cast<ProductionArgument *>(arg);
}

void Factory::startProduction(int num_products, Product *products,
                              unsigned int id) {
    production_threads[id] = new pthread_t;
    auto arg = new ProductionArgument(this, num_products, products, id);
    pthread_create(production_threads[id], nullptr, ProductionFunc, arg);
}

void Factory::produce(int num_products, Product *products) {
    pthread_mutex_lock(&products_lock);
    while (products_being_edited) {
        pthread_cond_wait(&products_cond, &products_lock);
    }
    for (int i = 0; i < num_products; ++i) {
        available_products.push_back(products[i]);
    }
    products_being_edited = false;
    pthread_cond_signal(&products_cond);
    pthread_mutex_unlock(&products_lock);
}

void Factory::finishProduction(unsigned int id) {
    pthread_mutex_lock(&products_lock);
    while(products_being_edited){
        pthread_cond_wait(&products_cond, &products_lock);
    }
    pthread_cancel(*(production_threads[id]));
    removeProductionThreadByID(id);
    pthread_cond_signal(&products_cond);
    pthread_mutex_unlock(&products_lock);
}

void Factory::startSimpleBuyer(unsigned int id) {
}

int Factory::tryBuyOne() {
    return -1;
}

int Factory::finishSimpleBuyer(unsigned int id) {
    return -1;
}

void
Factory::startCompanyBuyer(int num_products, int min_value, unsigned int id) {
}

std::list<Product> Factory::buyProducts(int num_products) {
    return std::list<Product>();
}

void Factory::returnProducts(std::list<Product> products, unsigned int id) {
}

int Factory::finishCompanyBuyer(unsigned int id) {
    return 0;
}

void Factory::startThief(int num_products, unsigned int fake_id) {
}

int Factory::stealProducts(int num_products, unsigned int fake_id) {
    return 0;
}

int Factory::finishThief(unsigned int fake_id) {
    return 0;
}

void Factory::closeFactory() {
    this->open_to_visitors = false;
}

void Factory::openFactory() {
    this->open_to_visitors = true;
}

void Factory::closeReturningService() {
    this->open_to_returns = false;
}

void Factory::openReturningService() {
    this->open_to_returns = true;
}

std::list<std::pair<Product, int>> Factory::listStolenProducts() {
    return std::list<std::pair<Product, int>>(stolen_products);
}

std::list<Product> Factory::listAvailableProducts() {
    return std::list<Product>(available_products);
}

void Factory::removeProductionThreadByID(unsigned int id) {
    delete production_threads[id];
    production_threads.erase(id);
}

void Factory::removeThiefThreadByID(unsigned int id) {
    delete thief_threads[id];
    thief_threads.erase(id);
}