#include "internal/MarketDataFeedHandler.h"
#include <iostream>
#include <stdexcept>

MarketDataFeedHandler::MarketDataFeedHandler()
    : curl_(nullptr)
    , baseUrl_("https://api.elections.kalshi.com/trade-api/v2/markets/")
    , timeout_(30)
    , userAgent_("Kalshi-Orderbook-Client/1.0")
    , initialized_(false) {
}

MarketDataFeedHandler::~MarketDataFeedHandler() {
    cleanup();
}

bool MarketDataFeedHandler::initialize() {
    if (initialized_) {
        return true;
    }

    CURLcode globalResult = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (globalResult != CURLE_OK) {
        lastError_ = "Failed to initialize curl globally: " + std::string(curl_easy_strerror(globalResult));
        return false;
    }

    curl_ = curl_easy_init();
    if (!curl_) {
        lastError_ = "Failed to initialize curl handle";
        curl_global_cleanup();
        return false;
    }

    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout_);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, userAgent_.c_str());
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);

    initialized_ = true;
    lastError_.clear();
    return true;
}

void MarketDataFeedHandler::cleanup() {
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
    
    if (initialized_) {
        curl_global_cleanup();
        initialized_ = false;
    }
}

json MarketDataFeedHandler::fetchOrderbookData(const std::string& ticker) {
    if (!initialized_ && !initialize()) {
        throw std::runtime_error("Failed to initialize MarketDataFeedHandler: " + lastError_);
    }

    std::string url = buildOrderbookUrl(ticker);
    APIResponse response;
    
    if (!performHttpRequest(url, response)) {
        throw std::runtime_error("HTTP request failed: " + lastError_);
    }

    if (response.responseCode != 200) {
        throw std::runtime_error("HTTP request failed with code: " + std::to_string(response.responseCode));
    }

    try {
        return json::parse(response.data);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON response: " + std::string(e.what()));
    }
}

bool MarketDataFeedHandler::populateOrderbook(Orderbook& orderbook, const std::string& ticker) {
    try {
        json marketData = fetchOrderbookData(ticker);
        
        if (!marketData.contains("orderbook")) {
            lastError_ = "Invalid response: missing orderbook data";
            return false;
        }

        json orderbookData = marketData["orderbook"];
        OrderId currentOrderId = 1;
        
        return parseAndAddOrders(orderbookData, orderbook, currentOrderId);
        
    } catch (const std::exception& e) {
        lastError_ = std::string(e.what());
        return false;
    }
}

bool MarketDataFeedHandler::getOrderbookLevelInfos(const std::string& ticker, OrderbookLevelInfos& levelInfos) {
    try {
        json marketData = fetchOrderbookData(ticker);
        
        if (!marketData.contains("orderbook")) {
            lastError_ = "Invalid response: missing orderbook data";
            return false;
        }

        json orderbookData = marketData["orderbook"];
        LevelInfos bids, asks;
        
        if (parseIntoLevelInfos(orderbookData, bids, asks)) {
            levelInfos = OrderbookLevelInfos(bids, asks);
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        lastError_ = std::string(e.what());
        return false;
    }
}

void MarketDataFeedHandler::setApiEndpoint(const std::string& endpoint) {
    baseUrl_ = endpoint;
}

void MarketDataFeedHandler::setTimeout(long timeoutSeconds) {
    timeout_ = timeoutSeconds;
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout_);
    }
}

void MarketDataFeedHandler::setUserAgent(const std::string& userAgent) {
    userAgent_ = userAgent;
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_USERAGENT, userAgent_.c_str());
    }
}

std::string MarketDataFeedHandler::getLastError() const {
    return lastError_;
}

bool MarketDataFeedHandler::hasError() const {
    return !lastError_.empty();
}

size_t MarketDataFeedHandler::writeCallback(void* contents, size_t size, size_t nmemb, APIResponse* response) {
    size_t totalSize = size * nmemb;
    if (response) {
        response->data.append(static_cast<char*>(contents), totalSize);
    }
    return totalSize;
}

bool MarketDataFeedHandler::performHttpRequest(const std::string& url, APIResponse& response) {
    if (!curl_) {
        lastError_ = "Curl handle not initialized";
        return false;
    }

    response.data.clear();
    response.responseCode = 0;

    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl_);
    
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response.responseCode);

    if (result != CURLE_OK) {
        lastError_ = "Curl request failed: " + std::string(curl_easy_strerror(result));
        return false;
    }

    return true;
}

std::string MarketDataFeedHandler::buildOrderbookUrl(const std::string& ticker) const {
    return baseUrl_ + ticker + "/orderbook";
}

bool MarketDataFeedHandler::parseAndAddOrders(const json& orderbookData, Orderbook& orderbook, OrderId& currentOrderId) {
    try {
        if (orderbookData.contains("yes")) {
            for (const auto& level : orderbookData["yes"]) {
                if (level.size() >= 2) {
                    Price price = level[0];
                    Quantity quantity = level[1];
                    
                    auto order = std::make_shared<Order>(
                        OrderType::GoodTillCancel,
                        currentOrderId++,
                        Side::Buy,
                        price,
                        quantity
                    );
                    orderbook.AddOrder(order);
                }
            }
        }
        
        if (orderbookData.contains("no")) {
            for (const auto& level : orderbookData["no"]) {
                if (level.size() >= 2) {
                    Price noPrice = level[0];
                    Quantity quantity = level[1];
                    
                    Price yesPrice = 100 - noPrice;
                    
                    auto order = std::make_shared<Order>(
                        OrderType::GoodTillCancel,
                        currentOrderId++,
                        Side::Sell,
                        yesPrice,
                        quantity
                    );
                    orderbook.AddOrder(order);
                }
            }
        }
        
        lastError_.clear();
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = "Error parsing orderbook data: " + std::string(e.what());
        return false;
    }
}

bool MarketDataFeedHandler::parseIntoLevelInfos(const json& orderbookData, LevelInfos& bids, LevelInfos& asks) {
    try {
        bids.clear();
        asks.clear();
        
        if (orderbookData.contains("yes")) {
            for (const auto& level : orderbookData["yes"]) {
                if (level.size() >= 2) {
                    Price price = level[0];
                    Quantity quantity = level[1];
                    bids.push_back({price, quantity});
                }
            }
        }
        
        if (orderbookData.contains("no")) {
            for (const auto& level : orderbookData["no"]) {
                if (level.size() >= 2) {
                    Price noPrice = level[0];
                    Quantity quantity = level[1];
                    Price yesPrice = 100 - noPrice;
                    asks.push_back({yesPrice, quantity});
                }
            }
        }
        
        lastError_.clear();
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = "Error parsing level infos: " + std::string(e.what());
        return false;
    }
}