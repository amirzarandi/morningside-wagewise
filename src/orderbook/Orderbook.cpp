#include "internal/Orderbook.h"

# include <numeric>

bool Orderbook::CanMatch(Side side, Price price) const
{
    if (side == Side::Buy)
    {
        if (asks_.empty())
            return false;
        
        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    }
    else
    {
        if (bids_.empty())
            return false;
        
        const auto& [bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}

Trades Orderbook::MatchOrders()
{
    Trades trades;
    trades.reserve(orders_.size());
    
    while (!bids_.empty() && !asks_.empty())
    {
        auto& [bidPrice, bids] = *bids_.begin();
        auto& [askPrice, asks] = *asks_.begin();
        
        if (bidPrice < askPrice)
            break;
        
        while (!bids.empty() && !asks.empty())
        {
            auto bid = bids.front();
            auto ask = asks.front();
            Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());
            bid->Fill(quantity);
            ask->Fill(quantity);
            
            if (bid->IsFilled())
            {
                bids.pop_front();
                orders_.erase(bid->GetOrderId());
            }
            if (ask->IsFilled())
            {
                asks.pop_front();
                orders_.erase(ask->GetOrderId());
            }
            
            trades.push_back(Trade{
                TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity },
                TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity }
            });
        }

        if (bids.empty())
            bids_.erase(bidPrice);
        if (asks.empty())
            asks_.erase(askPrice);
    }
    
    if (!bids_.empty())
    {
        auto& [_, bids] = *bids_.begin();
        auto& order = bids.front();
        if (order->GetOrderType() == OrderType::FillAndKill)
            CancelOrder(order->GetOrderId());
    }
    
    if (!asks_.empty())
    {
        auto& [_, asks] = *asks_.begin();
        auto& order = asks.front();
        if (order->GetOrderType() == OrderType::FillAndKill)
            CancelOrder(order->GetOrderId());
    }
    
    return trades;
}

Orderbook::Orderbook() { }

Orderbook::~Orderbook() { }

Trades Orderbook::AddOrder(OrderPointer order)
{
    if (orders_.contains(order->GetOrderId()))
        return { };
    
    if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice()))
        return { };
    
    OrderPointers::iterator iterator;

    if (order->GetSide() == Side::Buy)
    {
        auto& orders = bids_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }
    else
    {
        auto& orders = asks_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    orders_.insert({ order->GetOrderId(), OrderEntry{ order, iterator }});
    return MatchOrders();
}

void Orderbook::CancelOrder(OrderId orderId)
{
    if (!orders_.contains(orderId))
        return;
    
    const auto [order, iterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->GetSide() == Side::Sell)
    {
        auto price = order->GetPrice();
        auto& orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            asks_.erase(price);
    }
    else
    {
        auto price = order->GetPrice();
        auto& orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            bids_.erase(price);
    }
}

Trades Orderbook::MatchOrder(OrderModify order)
{
    if (!orders_.contains(order.GetOrderId()))
        return { };
    
    OrderType orderType = orders_.at(order.GetOrderId()).order_->GetOrderType();
    CancelOrder(order.GetOrderId());
    return AddOrder(order.ToOrderPointer(orderType));
}

std::size_t Orderbook::Size() const
{
    return orders_.size();
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const
{
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
    {
        return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0,
            [](Quantity runningSum, const OrderPointer& order)
            { return runningSum + order->GetRemainingQuantity(); }) };
    };

    for (const auto& [price, orders] : bids_)
        bidInfos.push_back(CreateLevelInfos(price, orders));

    for (const auto& [price, orders] : asks_)
        askInfos.push_back(CreateLevelInfos(price, orders));

    return OrderbookLevelInfos{ bidInfos, askInfos };
}