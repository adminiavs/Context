#pragma once

#include "ragger_plugin_api.h"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace Ragger {

class EventBus {
public:
    EventBus();
    ~EventBus();

    // Event emission
    int emitEvent(const EventData* event);
    int emitEventAsync(const EventData* event);

    // Event subscription
    int subscribe(EventType type, EventCallback callback, void* userData,
                 int priority = 0, const char* filter = nullptr);
    int unsubscribe(EventType type, EventCallback callback);

    // Bulk operations
    int subscribeMultiple(const EventType* types, size_t numTypes,
                         EventCallback callback, void* userData,
                         int priority = 0, const char* filter = nullptr);
    int unsubscribeAll(EventCallback callback);

    // Event filtering and prioritization
    void setGlobalFilter(const char* pluginName, bool enabled);
    void setEventFilter(EventType type, const char* filter);
    void setPriorityRange(int minPriority, int maxPriority);

    // Statistics and monitoring
    struct Stats {
        std::atomic<uint64_t> totalEventsEmitted{0};
        std::atomic<uint64_t> totalEventsProcessed{0};
        std::atomic<uint64_t> eventsDropped{0};
        std::atomic<uint64_t> averageProcessingTime{0}; // microseconds
        std::unordered_map<EventType, std::atomic<uint64_t>> eventsByType;
    };

    const Stats& getStats() const;
    void resetStats();

    // Thread safety
    void lock();
    void unlock();
    bool tryLock();

private:
    struct Subscription {
        EventType type;
        EventCallback callback;
        void* userData;
        int priority;
        std::string filter;
        bool enabled;

        bool matches(const EventData* event) const;
    };

    struct EventQueueItem {
        std::unique_ptr<EventData> event;
        int priority;

        bool operator<(const EventQueueItem& other) const {
            return priority < other.priority; // Higher priority first
        }
    };

    std::vector<Subscription> subscriptions_;
    std::priority_queue<EventQueueItem> eventQueue_;
    mutable std::mutex mutex_;
    Stats stats_;

    std::unordered_map<std::string, bool> globalFilters_;
    std::unordered_map<EventType, std::string> eventFilters_;
    int minPriority_;
    int maxPriority_;

    // Processing thread
    std::atomic<bool> running_;
    std::thread processingThread_;
    std::condition_variable eventCondition_;

    void processingLoop();
    void processEvent(const EventData* event);
    bool shouldProcessEvent(const EventData* event) const;
    std::vector<Subscription*> findMatchingSubscriptions(const EventData* event);

    // Prevent copying
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
};

} // namespace Ragger
