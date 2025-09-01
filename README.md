# Morningside Wagewise

A high-performance C++ market data feed handler and local orderbook implementation for Kalshi prediction markets, designed to enable development  of algorithmic trading and market making strategies.

## Overview

We fetch real-time market data from Kalshi's REST API and maintain a local orderbook replica for ultra-low latency trading operations. The system abstracts Kalshi's binary "Yes/No" betting markets into traditional bid/ask order flow, enabling standard options/futures-like pricing models.

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Kalshi API    │────│  MarketDataFeed  │────│ Local Orderbook │
│  (HTTP/WS)      │    │     Handler      │    │    Engine       │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## Quick Start

```bash
mkdir build
cd build

cmake ..
cmake --build .

./morningside-wagerwise
```

**Dependencies**: `libcurl`, `nlohmann/json`, C++20 compiler

## Features

### Market Data Integration
- Fetches orderbook data via REST endpoint by passing market ticker symbols
- Architected to support additional betting exchanges (Polymarket, etc.)

### Local Orderbook Engine
The core orderbook supports sophisticated order management:
- Supports multiple order types: `GoodTillCancel` and `FillAndKill`
- **Price-Time Priority Matching Algorithm**
- Automatically generates a trade when bids cross asks
- Level Info: aggregated bid/ask levels for market analysis

```cpp
// Example Usage: Add order and get resulting trades
auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 123, Side::Buy, 55, 100);
Trades trades = orderbook.AddOrder(order);
```

## Next Steps

We currently use Kalshi's HTTP REST endpoint for initial orderbook snapshots. While Kalshi offers WebSocket feeds for real-time updates, this serves as a working proof of concept for the data pipeline and orderbook integration. We show in the `main.cpp` example that we are able to fetch market data from Kalshi API, populate the Orderbook and manage its state and match orders. We can expand this in multiple ways:

- WebSocket integration for real-time updates
- Order lifecycle management (fills, cancellations from exchange)
- State synchronization with live Kalshi orderbook
- Multi-exchange adapter pattern


