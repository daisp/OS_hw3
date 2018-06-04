#include "Factory.h"
#include <vector>
#include <iostream>

void Factory::removeProductionThreadFromList(unsigned int id) {
//    pthread_mutex_lock(&production_threads_lock);
    delete production_threads[id];
    production_threads.erase(id);
//    pthread_mutex_unlock(&production_threads_lock);
}

void Factory::removeSimpleBuyerThreadFromList(unsigned int id) {
//    pthread_mutex_lock(&simple_buyer_threads_lock);
    delete simple_buyer_threads[id];
    simple_buyer_threads.erase(id);
//    pthread_mutex_unlock(&simple_buyer_threads_lock);
}

void Factory::removeCompanyThreadFromList(unsigned int id) {
//    pthread_mutex_lock(&company_buyer_threads_lock);
    delete company_buyer_threads[id];
    company_buyer_threads.erase(id);
//    pthread_mutex_unlock(&company_buyer_threads_lock);
}

void Factory::removeThiefThreadFromList(unsigned int id) {
//    pthread_mutex_lock(&thief_threads_lock);
    delete thief_threads[id];
    thief_threads.erase(id);
//    pthread_mutex_unlock(&thief_threads_lock);
}

class ProductionArgument {
public:
    ProductionArgument(Factory *factory, int num_products, Product *products,
                       unsigned int id)
            : factory(factory), num_products(num_products),
              products(products) {}

    ~ProductionArgument() = default;

    Factory *factory;
    int num_products;
    Product *products;
};

class SimpleBuyerArgument {
public:
    SimpleBuyerArgument(Factory *factory, unsigned int id) : factory(factory) {}

    ~SimpleBuyerArgument() = default;

    Factory *factory;
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

class ThiefArgument {
public:
    ThiefArgument(Factory *factory, int num_products, unsigned int fake_id)
            : factory(factory), num_products(num_products), fake_id(fake_id) {}

    ~ThiefArgument() = default;

    Factory *factory;
    unsigned int fake_id;
    int num_products;
};

Factory::Factory() : open_to_returns(true), open_to_visitors(true),
                     products_being_edited(false), thief_count(0),
                     company_buyer_count(0) {
    pthread_cond_init(&open_to_visitors_cond, nullptr);
    pthread_cond_init(&open_to_returns_cond, nullptr);
    //=================================================================
    pthread_mutexattr_init(&products_lock_attributes);
    pthread_mutexattr_settype(&products_lock_attributes,
                              PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&factory_lock, &products_lock_attributes);
    pthread_cond_init(&products_cond, nullptr);
}

Factory::~Factory() {
    pthread_cond_destroy(&products_cond);
    pthread_mutex_destroy(&factory_lock);
    pthread_cond_destroy(&open_to_visitors_cond);
    pthread_cond_destroy(&open_to_returns_cond);
}

void *productionFunc(void *arg) {
    auto factory = static_cast<ProductionArgument *>(arg)->factory;
    auto num_products = static_cast<ProductionArgument *>(arg)->num_products;
    auto products = static_cast<ProductionArgument *>(arg)->products;
    factory->Factory::produce(num_products, products);
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
    pthread_mutex_lock(&factory_lock);
    while (products_being_edited) {
        pthread_cond_wait(&products_cond, &factory_lock);
    }
    products_being_edited = true;
    for (int i = 0; i < num_products; ++i) {
        available_products.push_back(products[i]);
    }
    products_being_edited = false;
    pthread_cond_signal(&products_cond);
    pthread_mutex_unlock(&factory_lock);
}

void Factory::finishProduction(unsigned int id) {
    void **result = nullptr;
    pthread_join(*production_threads[id], result);
    removeProductionThreadFromList(id);
}

void *simpleBuyerFunc(void *arg) {
    auto factory = static_cast<ProductionArgument *>(arg)->factory;
    auto bought_value_ptr = new int(factory->tryBuyOne());
    delete static_cast<SimpleBuyerArgument *>(arg);
    return bought_value_ptr;
}

void Factory::startSimpleBuyer(unsigned int id) {
    simple_buyer_threads[id] = new pthread_t;
    auto arg = new SimpleBuyerArgument(this, id);
    pthread_create(simple_buyer_threads[id], nullptr, simpleBuyerFunc, arg);
}

int Factory::tryBuyOne() {
    pthread_mutex_lock(&factory_lock);
    while (!open_to_visitors || products_being_edited ||
           available_products.empty() || thief_count > 0 ||
           company_buyer_count > 0) {
        pthread_mutex_unlock(&factory_lock);
        return -1;
    }
    products_being_edited = true;
    int id = available_products.front().getId();
    available_products.pop_front();
    products_being_edited = false;
    pthread_cond_signal(&products_cond);
    pthread_mutex_unlock(&factory_lock);
    return id;
}

int Factory::finishSimpleBuyer(unsigned int id) {
    int *bought_id_ptr;
    int bought_id;
    pthread_join(*simple_buyer_threads[id], (void **) &bought_id_ptr);
    removeSimpleBuyerThreadFromList(id);
    bought_id = *bought_id_ptr;
    delete bought_id_ptr;
    return bought_id;
}

/*
 * This function removes the elements that are to be KEPT by the company.
 * In other words, after this function, all the elements in bought_products are
 * products that should to be returned to the factory.
 */
int filterProducts(std::list<Product> *bought_products, int min_value) {
    int num_returned = 0;
    auto i = bought_products->begin();
    while (i != bought_products->end()) {
        if (i->getValue() >= min_value) i = bought_products->erase(i);
        else {
            ++i;
            num_returned++;
        }
    }
    return num_returned;
}

void *companyBuyerFunc(void *arg) {
    auto factory = static_cast<CompanyArgument *>(arg)->factory;
    auto num_products = static_cast<CompanyArgument *>(arg)->num_products;
    auto min_value = static_cast<CompanyArgument *>(arg)->min_value;
    auto id = static_cast<CompanyArgument *>(arg)->id;
    auto bought_products = factory->buyProducts(num_products);
    auto num_returned_ptr = new int(
            filterProducts(&bought_products, min_value));
    factory->returnProducts(bought_products, id);
    delete static_cast<CompanyArgument *>(arg);
    return num_returned_ptr;
}

void Factory::startCompanyBuyer(int num_products, int min_value,
                                unsigned int id) {
    company_buyer_count++;
    company_buyer_threads[id] = new pthread_t;
    auto arg = new CompanyArgument(this, num_products, min_value, id);
    pthread_create(company_buyer_threads[id], nullptr, companyBuyerFunc, arg);
}

std::list<Product> Factory::buyProducts(int num_products) {
    auto bought_products = std::list<Product>();
    pthread_mutex_lock(&factory_lock);
    while (!open_to_visitors || thief_count > 0 || products_being_edited ||
           available_products.size() < num_products) {
        if (!open_to_visitors) {
            pthread_cond_wait(&open_to_visitors_cond, &factory_lock);
        } else {
            pthread_cond_wait(&products_cond, &factory_lock);
        }
        pthread_mutex_lock(&factory_lock);
    }
    products_being_edited = true;
    for (int i = 0; i < num_products; ++i) {
        bought_products.push_back(available_products.front());
        available_products.pop_front();
    }
    products_being_edited = false;
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&factory_lock);
    return bought_products;
}

void Factory::returnProducts(std::list<Product> products, unsigned int id) {
    pthread_mutex_lock(&factory_lock);
    while (!open_to_visitors || !open_to_returns || products_being_edited ||
           thief_count > 0) {
        if (!open_to_visitors) {
            pthread_cond_wait(&open_to_visitors_cond, &factory_lock);
        } else if (!open_to_returns) {
            pthread_cond_wait(&open_to_returns_cond, &factory_lock);
        } else {
            pthread_cond_wait(&products_cond, &factory_lock);
        }
        pthread_mutex_lock(&factory_lock);
    }
    products_being_edited = true;
    auto num_to_return = products.size();
    for (int i = 0; i < num_to_return; ++i) {
        available_products.push_back(products.front());
        products.pop_front();
    }
    products_being_edited = false;
    company_buyer_count--;
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&factory_lock);
}

int Factory::finishCompanyBuyer(unsigned int id) {
    int *num_returned_ptr;
    int num_returned;
    pthread_join(*company_buyer_threads[id], (void **) &num_returned_ptr);
    removeCompanyThreadFromList(id);
    num_returned = *num_returned_ptr;
    delete num_returned_ptr;
    return num_returned;
}

void *thiefFunc(void *arg) {
    auto factory = static_cast<ThiefArgument *>(arg)->factory;
    auto num_products = static_cast<ThiefArgument *>(arg)->num_products;
    auto fake_id = static_cast<ThiefArgument *>(arg)->fake_id;
    auto num_stolen_ptr = new int(
            factory->stealProducts(num_products, fake_id));
    delete static_cast<ThiefArgument *>(arg);
    return num_stolen_ptr;
}

void Factory::startThief(int num_products, unsigned int fake_id) {
    thief_count++;
    thief_threads[fake_id] = new pthread_t;
    auto arg = new ThiefArgument(this, num_products, fake_id);
    pthread_create(thief_threads[fake_id], nullptr, thiefFunc, arg);
}

int Factory::stealProducts(int num_products, unsigned int fake_id) {
    int num_stolen_products = 0;
    pthread_mutex_lock(&factory_lock);
    while (!open_to_visitors || products_being_edited) {
        if (!open_to_visitors) {
            pthread_cond_wait(&open_to_visitors_cond, &factory_lock);
        } else {
            pthread_cond_wait(&products_cond, &factory_lock);
        }
        pthread_mutex_lock(&factory_lock);
    }
    products_being_edited = true;
    for (int i = 0; i < num_products && !available_products.empty(); ++i) {
        std::pair<Product, int> pair = std::pair<Product, int>(
                available_products.front(), static_cast<int>(fake_id));
        stolen_products.push_back(pair);
        available_products.pop_front();
        num_stolen_products++;
    }
    products_being_edited = false;
    thief_count--;
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&factory_lock);
    return num_stolen_products;
}

int Factory::finishThief(unsigned int fake_id) {
    int *num_stolen_ptr;
    int num_stolen;
    pthread_join(*thief_threads[fake_id], (void **) &num_stolen_ptr);
    removeThiefThreadFromList(fake_id);
    num_stolen = *num_stolen_ptr;
    delete num_stolen_ptr;
    return num_stolen;
}

void Factory::closeFactory() {
    pthread_mutex_lock(&factory_lock);
    this->open_to_visitors = false;
    pthread_mutex_unlock(&factory_lock);
}

void Factory::openFactory() {
    pthread_mutex_lock(&factory_lock);
    this->open_to_visitors = true;
    pthread_cond_broadcast(&open_to_visitors_cond);
    pthread_mutex_unlock(&factory_lock);
}

void Factory::closeReturningService() {
    pthread_mutex_lock(&factory_lock);
    this->open_to_returns = false;
    pthread_mutex_unlock(&factory_lock);
}

void Factory::openReturningService() {
    pthread_mutex_lock(&factory_lock);
    this->open_to_returns = true;
    pthread_cond_broadcast(&open_to_returns_cond);
    pthread_mutex_unlock(&factory_lock);

}

std::list<std::pair<Product, int>> Factory::listStolenProducts() {
    return stolen_products;
}

std::list<Product> Factory::listAvailableProducts() {
    return available_products;
}