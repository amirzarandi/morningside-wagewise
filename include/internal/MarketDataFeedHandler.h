#pragma once

#include <string>
#include <memory>
#include <functional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "Orderbook.h"
#include <LevelInfo.h>
#include <OrderbookLevelInfos.h>
#include <Order.h>
#include <Usings.h>

using json = nlohmann::json;

struct APIResponse {
    std::string data;
    long responseCode;
    
    APIResponse() : responseCode(0) {}
};

class MarketDataFeedHandler {
public:
    MarketDataFeedHandler();
    ~MarketDataFeedHandler();

    MarketDataFeedHandler(const MarketDataFeedHandler&) = delete;
    MarketDataFeedHandler& operator=(const MarketDataFeedHandler&) = delete;

    bool initialize();
    
    void cleanup();

    json fetchOrderbookData(const std::string& ticker);
    
    bool populateOrderbook(Orderbook& orderbook, const std::string& ticker);
    
    bool getOrderbookLevelInfos(const std::string& ticker, OrderbookLevelInfos& levelInfos);

    void setApiEndpoint(const std::string& endpoint);
    void setTimeout(long timeoutSeconds);
    void setUserAgent(const std::string& userAgent);
    
    std::string getLastError() const;
    bool hasError() const;

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, APIResponse* response);
    
    bool performHttpRequest(const std::string& url, APIResponse& response);
    
    std::string buildOrderbookUrl(const std::string& ticker) const;
    
    bool parseAndAddOrders(const json& orderbookData, Orderbook& orderbook, OrderId& currentOrderId);
    
    bool parseIntoLevelInfos(const json& orderbookData, LevelInfos& bids, LevelInfos& asks);

    CURL* curl_;
    std::string baseUrl_;
    long timeout_;
    std::string userAgent_;
    std::string lastError_;
    bool initialized_;
};