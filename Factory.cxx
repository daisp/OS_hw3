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
//    pthread_mutex_lock(&products_lock);
//    while (products_being_edited) {
//        pthread_cond_wait(&products_cond, &products_lock);
//    }
    pthread_join(*production_threads[id], nullptr);
    removeProductionThreadFromList(id);
//    pthread_cond_broadcast(&products_cond);
//    pthread_mutex_unlock(&products_lock);
}

void *simpleBuyerFunc(void *arg) {
    auto factory = static_cast<ProductionArgument *>(arg)->factory;
    auto id = static_cast<ProductionArgument *>(arg)->id;
    auto bought_value_ptr = new int(factory->tryBuyOne());
    factory->removeSimpleBuyerThreadFromList(id);
    delete static_cast<SimpleBuyerArgument *>(arg);
    return bought_value_ptr;
}

void Factory::startSimpleBuyer(unsigned int id) {
    simple_buyer_threads[id] = new pthread_t;
    auto arg = new SimpleBuyerArgument(this, id);
    pthread_create(simple_buyer_threads[id], nullptr, simpleBuyerFunc, arg);
}

int Factory::tryBuyOne() {
    pthread_mutex_lock(&products_lock);
    while (products_being_edited || num_available_products == 0) {
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
    int *bought_value_ptr;
    int bought_value;
//    pthread_mutex_lock(&products_lock);
//    while (products_being_edited) {
//        pthread_cond_wait(&products_cond, &products_lock);
//    }
    pthread_join(*simple_buyer_threads[id], (void **) &bought_value_ptr);
    removeProductionThreadFromList(id);
//    pthread_cond_broadcast(&products_cond);
//    pthread_mutex_unlock(&products_lock);
    bought_value = *bought_value_ptr;
    delete bought_value_ptr;
    return bought_value;
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
    factory->removeCompanyThreadFromList(id);
    delete static_cast<CompanyArgument *>(arg);
    return num_returned_ptr;
}

void Factory::startCompanyBuyer(int num_products, int min_value,
                                unsigned int id) {
    company_buyer_threads[id] = new pthread_t;
    auto arg = new CompanyArgument(this, num_products, min_value, id);
    pthread_create(company_buyer_threads[id], nullptr, companyBuyerFunc, arg);
}

std::list<Product> Factory::buyProducts(int num_products) {
    auto bought_products = std::list<Product>();
    pthread_mutex_lock(&products_lock);
    while (products_being_edited || num_available_products < num_products) {
        pthread_cond_wait(&products_cond, &products_lock);
    }
    products_being_edited = true;
    for (int i = 0; i < num_products; ++i) {
        bought_products.push_back(available_products.back());
        available_products.pop_back();
    }
    num_available_products -= num_products;
    products_being_edited = false;
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&products_lock);
    return bought_products;
}

void Factory::returnProducts(std::list<Product> products, unsigned int id) {
    pthread_mutex_lock(&products_lock);
    while (products_being_edited) {
        pthread_cond_wait(&products_cond, &products_lock);
    }
    products_being_edited = true;
    for (int i = 0; i < products.size(); ++i) {
        available_products.push_back(products.back());
        products.pop_back();
        /* @TODO check order of insertion and deletion of all product arrays */
    }
    num_available_products += products.size();
    products_being_edited = false;
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&products_lock);
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
    auto num_stolen_ptr = new int(factory->stealProducts(num_products, fake_id));
    factory->removeThiefThreadFromList(fake_id);
    delete static_cast<ThiefArgument *>(arg);
    return num_stolen_ptr;
}

void Factory::startThief(int num_products, unsigned int fake_id) {
    thief_threads[fake_id] = new pthread_t;
    auto arg = new ThiefArgument(this, num_products, fake_id);
    pthread_create(thief_threads[fake_id], nullptr, thiefFunc, arg);
}

int Factory::stealProducts(int num_products, unsigned int fake_id) {
    int num_stolen_products = 0;
    pthread_mutex_lock(&products_lock);
    while (products_being_edited) {
        pthread_cond_wait(&products_cond, &products_lock);
    }
    products_being_edited = true;
    for (int i = 0; i < num_products && num_available_products > 0; ++i) {
        std::pair<Product, int> pair = std::pair(available_products.back(),
                                                 static_cast<int>(fake_id));
        stolen_products.push_back(pair);
        available_products.pop_back();
        num_stolen_products++;
        num_available_products--;
    }
    products_being_edited = false;
    pthread_cond_broadcast(&products_cond);
    pthread_mutex_unlock(&products_lock);
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


void Factory::removeProductionThreadFromList(unsigned int id) {
    delete production_threads[id];
    production_threads.erase(id);
}

void Factory::removeSimpleBuyerThreadFromList(unsigned int id) {
    delete simple_buyer_threads[id];
    simple_buyer_threads.erase(id);
}

void Factory::removeCompanyThreadFromList(unsigned int id) {
    delete company_buyer_threads[id];
    company_buyer_threads.erase(id);
}

void Factory::removeThiefThreadFromList(unsigned int id) {
    delete thief_threads[id];
    thief_threads.erase(id);
}