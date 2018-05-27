#include "Factory.h"
#include <vector>


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

class SimpleBuyerArgument {
public:
    SimpleBuyerArgument(Factory *factory, unsigned int id) : factory(factory),
                                                             id(id) {}

    ~SimpleBuyerArgument() = default;

    Factory *factory;
    unsigned int id;
};

class CompanyArgument {
public:
    CompanyArgument(Factory *factory, int num_products, int min_value,
                    unsigned int id) : factory(factory),
                                       num_products(num_products),
                                       id(id), min_value(min_value) {}

    ~CompanyArgument() = default;

    Factory *factory;
    unsigned int id;
    int num_products;
    int min_value;
};

Factory::Factory() : open_to_returns(true), open_to_visitors(true),
                     products_being_edited(false), num_available_products(0) {
    pthread_mutexattr_init(&products_lock_attributes);
    pthread_mutexattr_settype(&products_lock_attributes,
                              PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&products_lock, &products_lock_attributes);
    pthread_cond_init(&products_cond, nullptr);
}

Factory::~Factory() {
    pthread_cond_destroy(&products_cond);
}

void *productionFunc(void *arg) {
    auto factory = static_cast<ProductionArgument *>(arg)->factory;
    auto num_products = static_cast<ProductionArgument *>(arg)->num_products;
    auto products = static_cast<ProductionArgument *>(arg)->products;
    auto id = static_cast<ProductionArgument *>(arg)->id;
//    factory->updateProductionThreads(id, pthread_self());
    factory->Factory::produce(num_products, products);
    factory->removeProductionThreadFromList(id);
    delete static_cast<ProductionArgument *>(arg);
    return nullptr;
}

void Factory::startProduction(int num_products, Product *products,
                              unsigned int id) {
    production_threads[id] = new pthread_t;
    auto arg = new ProductionArgument(this, num_products, products, id);
    pthread_create(production_threads[id], nullptr, productionFunc, arg);
}

void Factory::produce(int num_products, Product *products) {
    pthread_mutex_lock(&products_lock);
    while (products_being_edited) {
        pthread_cond_wait(&products_cond, &products_lock);
    }
    for (int i = 0; i < num_products; ++i) {
        available_products.push_back(products[i]);
    }
    num_available_products += num_products;
    products_being_edited = false;
    pthread_cond_signal(&products_cond);
    pthread_mutex_unlock(&products_lock);
}

void Factory::finishProduction(unsigned int id) {
    pthread_mutex_lock(&products_lock);
    while (products_being_edited) {
        pthread_cond_wait(&products_cond, &products_lock);
    }
    pthread_join(*production_threads[id], nullptr);
    removeProductionThreadFromList(id);
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&products_lock);
}

void *simpleBuyerFunc(void *arg) {
    auto factory = static_cast<ProductionArgument *>(arg)->factory;
    auto id = static_cast<ProductionArgument *>(arg)->id;
    auto bought_value = new int(factory->tryBuyOne()); // @TODO free this memory
    factory->removeSimpleBuyerThreadByID(id);
    delete static_cast<SimpleBuyerArgument *>(arg);
    return bought_value;
}

void Factory::startSimpleBuyer(unsigned int id) {
    simple_buyer_threads[id] = new pthread_t;
    auto arg = new SimpleBuyerArgument(this, id);
    pthread_create(simple_buyer_threads[id], nullptr, simpleBuyerFunc, arg);
}

int Factory::tryBuyOne() {
    pthread_mutex_lock(&products_lock);
    if (products_being_edited || num_available_products == 0) {
        pthread_mutex_unlock(&products_lock);
        return -1;
    }
    products_being_edited = true;
    int value = available_products.back().getId();
    available_products.pop_back();
    num_available_products--;
    products_being_edited = false;
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&products_lock);
    return value;
}

int Factory::finishSimpleBuyer(unsigned int id) {
    int *bought_value;
    pthread_mutex_lock(&products_lock);
    while (products_being_edited) {
        pthread_cond_wait(&products_cond, &products_lock);
    }
    pthread_join(*simple_buyer_threads[id], (void **) &bought_value);
    removeProductionThreadFromList(id);
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&products_lock);
    return *bought_value;
}

void *companyFunc(void *arg) {
    auto factory = static_cast<CompanyArgument *>(arg)->factory;
    auto num_products = static_cast<CompanyArgument *>(arg)->num_products;
    auto min_value = static_cast<CompanyArgument *>(arg)->min_value;
    auto id = static_cast<CompanyArgument *>(arg)->id;
//    factory->updateProductionThreads(id, pthread_self());
    factory->buyProducts(num_products);
    factory->removeProductionThreadFromList(id);
    delete static_cast<ProductionArgument *>(arg);
    return nullptr;
}

void Factory::startCompanyBuyer(int num_products, int min_value,
                                unsigned int id) {
    company_threads[id] = new pthread_t;
    auto arg = new CompanyArgument(this, num_products, min_value, id);
    pthread_create(company_threads[id], nullptr, companyFunc, arg);
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

//void Factory::updateProductionThreads(unsigned int id, int thread_id){
//    production_threads[id].thread_id = thread_id;
//}
//void updateThiefThreads(){}
//void updateCompanyThreads(){}

void Factory::removeProductionThreadFromList(unsigned int id) {
    delete production_threads[id];
    production_threads.erase(id);
}

void Factory::removeSimpleBuyerThreadByID(unsigned int id) {
    delete simple_buyer_threads[id];
    simple_buyer_threads.erase(id);
}

void Factory::removeThiefThreadByID(unsigned int id) {
    delete thief_threads[id];
    thief_threads.erase(id);
}