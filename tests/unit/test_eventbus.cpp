#include "EventBus.h"
#include "ragger_plugin_api.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

namespace Ragger {

class EventBusTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventBus = std::make_unique<EventBus>();
    }

    void TearDown() override {
        eventBus.reset();
    }

    std::unique_ptr<EventBus> eventBus;
};

TEST_F(EventBusTest, BasicEventEmission) {
    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "test_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    int result = eventBus->emitEvent(&event);
    EXPECT_EQ(result, RAGGER_SUCCESS);
}

TEST_F(EventBusTest, EventSubscription) {
    bool callbackCalled = false;
    
    auto callback = [&callbackCalled](const EventData* event, void* userData) {
        callbackCalled = true;
    };

    int result = eventBus->subscribe(EVENT_CODEBLOCK_INDEXED, callback, nullptr);
    EXPECT_EQ(result, RAGGER_SUCCESS);

    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "test_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    result = eventBus->emitEvent(&event);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    
    // Give some time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_TRUE(callbackCalled);
}

TEST_F(EventBusTest, EventUnsubscription) {
    bool callbackCalled = false;
    
    auto callback = [&callbackCalled](const EventData* event, void* userData) {
        callbackCalled = true;
    };

    // Subscribe
    int result = eventBus->subscribe(EVENT_CODEBLOCK_INDEXED, callback, nullptr);
    EXPECT_EQ(result, RAGGER_SUCCESS);

    // Unsubscribe
    result = eventBus->unsubscribe(EVENT_CODEBLOCK_INDEXED, callback);
    EXPECT_EQ(result, RAGGER_SUCCESS);

    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "test_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    result = eventBus->emitEvent(&event);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    
    // Give some time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_FALSE(callbackCalled);
}

TEST_F(EventBusTest, PriorityOrdering) {
    std::vector<int> callbackOrder;
    
    auto callback1 = [&callbackOrder](const EventData* event, void* userData) {
        callbackOrder.push_back(1);
    };
    
    auto callback2 = [&callbackOrder](const EventData* event, void* userData) {
        callbackOrder.push_back(2);
    };

    // Subscribe with different priorities
    eventBus->subscribe(EVENT_CODEBLOCK_INDEXED, callback1, nullptr, 10);
    eventBus->subscribe(EVENT_CODEBLOCK_INDEXED, callback2, nullptr, 20);

    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "test_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    int result = eventBus->emitEvent(&event);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    
    // Give some time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Higher priority (20) should be called first
    EXPECT_EQ(callbackOrder.size(), 2);
    EXPECT_EQ(callbackOrder[0], 2);
    EXPECT_EQ(callbackOrder[1], 1);
}

TEST_F(EventBusTest, StatsTracking) {
    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "test_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Emit multiple events
    for (int i = 0; i < 5; ++i) {
        int result = eventBus->emitEvent(&event);
        EXPECT_EQ(result, RAGGER_SUCCESS);
    }

    const auto& stats = eventBus->getStats();
    EXPECT_EQ(stats.totalEventsProcessed, 5);
    EXPECT_EQ(stats.eventsByType[EVENT_CODEBLOCK_INDEXED], 5);
}

TEST_F(EventBusTest, InvalidEventHandling) {
    // Test with null event
    int result = eventBus->emitEvent(nullptr);
    EXPECT_EQ(result, RAGGER_ERROR_INVALID_ARGUMENT);

    // Test with null callback
    result = eventBus->subscribe(EVENT_CODEBLOCK_INDEXED, nullptr, nullptr);
    EXPECT_EQ(result, RAGGER_ERROR_INVALID_ARGUMENT);
}

} // namespace Ragger
