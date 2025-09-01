#include "internal/Orderbook.h"
#include "internal/MarketDataFeedHandler.h"

#include <iostream>
#include <string>

void displayOrderbookInfo(const Orderbook& orderbook) {
    std::cout << "\n--- Orderbook Summary ---" << std::endl;
    std::cout << "Total orders: " << orderbook.Size() << std::endl;
    
    OrderbookLevelInfos levelInfos = orderbook.GetOrderInfos();
    
    const auto& bids = levelInfos.GetBids();
    const auto& asks = levelInfos.GetAsks();
    
    std::cout << "Bid levels: " << bids.size() << std::endl;
    std::cout << "Ask levels: " << asks.size() << std::endl;
    
    std::cout << "\n--- Top Bids ---" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), bids.size()); ++i) {
        std::cout << "Bid " << (i+1) << ": " << bids[i].price_ << "¢ @ " << bids[i].quantity_ << std::endl;
    }
    
    std::cout << "\n--- Top Asks ---" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), asks.size()); ++i) {
        std::cout << "Ask " << (i+1) << ": " << asks[i].price_ << "¢ @ " << asks[i].quantity_ << std::endl;
    }
}

int main() {
    try {
        MarketDataFeedHandler feedHandler;
        
        if (!feedHandler.initialize()) {
            std::cerr << "Failed to initialize handler: " << feedHandler.getLastError() << std::endl;
            return 1;
        }
        
        std::string ticker = "KXPRESPERSON-28-GNEWS";
        std::cout << "Enter ticker (or press Enter for default '" << ticker << "'): ";
        std::string userTicker;
        std::getline(std::cin, userTicker);
        
        if (!userTicker.empty()) {
            ticker = userTicker;
        }
        
        Orderbook orderbook;
        
        if (!feedHandler.populateOrderbook(orderbook, ticker)) {
            std::cerr << "Failed to populate orderbook: " << feedHandler.getLastError() << std::endl;
            return 1;
        }
        
        displayOrderbookInfo(orderbook);
        
        std::cout << "\n--- Example Orderbook Operations ---" << std::endl;
        
        if (orderbook.Size() > 0) {
            std::cout << "Canceling order with ID 1" << std::endl;
            try {
                orderbook.CancelOrder(1);
                std::cout << "Orders remaining: " << orderbook.Size() << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Failed to cancel order: " << e.what() << std::endl;
            }
        }
        
        std::cout << "\n--- Example Direct Level Info Access ---" << std::endl;
        OrderbookLevelInfos levelInfos({}, {});
        
        if (feedHandler.getOrderbookLevelInfos(ticker, levelInfos)) {
            const auto& bids = levelInfos.GetBids();
            const auto& asks = levelInfos.GetAsks();
            
            std::cout << "Retrieved level info directly - Bids: " << bids.size() 
                    << ", Asks: " << asks.size() << std::endl;
            
            if (!bids.empty() && !asks.empty()) {
                Price spread = asks[0].price_ - bids[0].price_;
                std::cout << "Best bid-ask spread: " << spread << "¢" << std::endl;
            }
        } else {
            std::cout << "Failed to get level infos: " << feedHandler.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}