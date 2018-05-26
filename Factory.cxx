#include "Factory.h"
#include <pthread.h>
#include <vector>

class ProductionArgument {
public:
    ProductionArgument(Factory *factory, int num_products, Product *products)
            : factory(factory), num_products(num_products),
              products(products) {}

    ~ProductionArgument() = default;

    Factory *factory;
    int num_products;
    Product *products;
};

Factory::Factory() : open_to_returns(true), open_to_visitors(true),
                     stolen_products(new std::list<std::pair<Product, int>>),
                     available_products(new std::list<Product>) {
}

Factory::~Factory() {
    delete this->stolen_products;
    delete this->available_products;
}

void *ProductionFunc(void *arg) {
    auto factory = static_cast<ProductionArgument*>(arg)->factory;
    auto num_products = static_cast<ProductionArgument*>(arg)->num_products;
    auto products = static_cast<ProductionArgument*>(arg)->products;
    factory->Factory::produce(num_products, products);
    delete static_cast<ProductionArgument*>(arg);
}

void Factory::startProduction(int num_products, Product *products,
                              unsigned int id) {
    auto production_thread = new pthread_t;
    auto arg = new ProductionArgument(this, num_products, products);
    pthread_create(production_thread, nullptr, ProductionFunc, arg);
}

void Factory::produce(int num_products, Product *products) {
}

void Factory::finishProduction(unsigned int id) {
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
    return std::list<std::pair<Product, int>>(*stolen_products);
}

std::list<Product> Factory::listAvailableProducts() {
    return std::list<Product>(*available_products);
}