#include "EventBus.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <climits>

namespace Ragger {

EventBus::EventBus()
    : minPriority_(INT_MIN), maxPriority_(INT_MAX), running_(false) {
    // Start processing thread
    running_ = true;
    processingThread_ = std::thread(&EventBus::processingLoop, this);
}

EventBus::~EventBus() {
    // Stop processing thread
    running_ = false;
    eventCondition_.notify_all();
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

int EventBus::emitEvent(const EventData* event) {
    if (!event) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (!shouldProcessEvent(event)) {
        stats_.eventsDropped++;
        return RAGGER_SUCCESS; // Not an error, just filtered out
    }

    // Process immediately
    auto startTime = std::chrono::high_resolution_clock::now();
    processEvent(event);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    stats_.totalEventsProcessed++;
    stats_.eventsByType[event->type]++;

    // Update average processing time
    if (stats_.totalEventsProcessed == 1) {
        stats_.averageProcessingTime = duration.count();
    } else {
        stats_.averageProcessingTime =
            (stats_.averageProcessingTime * (stats_.totalEventsProcessed - 1) + duration.count()) /
            stats_.totalEventsProcessed;
    }

    return RAGGER_SUCCESS;
}

int EventBus::emitEventAsync(const EventData* event) {
    if (!event) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (!shouldProcessEvent(event)) {
        stats_.eventsDropped++;
        return RAGGER_SUCCESS;
    }

    // Copy event data
    EventData* eventCopy = new EventData(*event);
    if (event->data && event->dataSize > 0) {
        eventCopy->data = new char[event->dataSize];
        std::memcpy(eventCopy->data, event->data, event->dataSize);
    }

    // Add to queue with default priority
    eventQueue_.push({std::unique_ptr<EventData>(eventCopy), 0});
    eventCondition_.notify_one();

    stats_.totalEventsEmitted++;
    return RAGGER_SUCCESS;
}

int EventBus::subscribe(EventType type, EventCallback callback, void* userData,
                       int priority, const char* filter) {
    if (!callback) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already subscribed
    for (const auto& sub : subscriptions_) {
        if (sub.type == type && sub.callback == callback && sub.userData == userData) {
            return RAGGER_ERROR_INVALID_ARGUMENT; // Already subscribed
        }
    }

    subscriptions_.push_back({
        type,
        callback,
        userData,
        priority,
        filter ? std::string(filter) : "",
        true
    });

    std::cout << "EventBus: Subscribed to event type " << static_cast<int>(type) << " with priority " << priority << std::endl;
    return RAGGER_SUCCESS;
}

int EventBus::unsubscribe(EventType type, EventCallback callback) {
    if (!callback) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::remove_if(subscriptions_.begin(), subscriptions_.end(),
        [type, callback](const Subscription& sub) {
            return sub.type == type && sub.callback == callback;
        });

    if (it != subscriptions_.end()) {
        subscriptions_.erase(it, subscriptions_.end());
        std::cout << "EventBus: Unsubscribed from event type " << static_cast<int>(type) << std::endl;
        return RAGGER_SUCCESS;
    }

    return RAGGER_ERROR_INVALID_ARGUMENT; // Not found
}

int EventBus::subscribeMultiple(const EventType* types, size_t numTypes,
                               EventCallback callback, void* userData,
                               int priority, const char* filter) {
    if (!types || numTypes == 0 || !callback) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    for (size_t i = 0; i < numTypes; ++i) {
        subscriptions_.push_back({
            types[i],
            callback,
            userData,
            priority,
            filter ? std::string(filter) : "",
            true
        });
    }

    std::cout << "EventBus: Subscribed to " << numTypes << " event types with priority " << priority << std::endl;
    return RAGGER_SUCCESS;
}

int EventBus::unsubscribeAll(EventCallback callback) {
    if (!callback) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::remove_if(subscriptions_.begin(), subscriptions_.end(),
        [callback](const Subscription& sub) {
            return sub.callback == callback;
        });

    size_t removed = std::distance(it, subscriptions_.end());
    subscriptions_.erase(it, subscriptions_.end());

    std::cout << "EventBus: Unsubscribed from " << removed << " subscriptions" << std::endl;
    return RAGGER_SUCCESS;
}

void EventBus::setGlobalFilter(const char* pluginName, bool enabled) {
    if (!pluginName) return;

    std::lock_guard<std::mutex> lock(mutex_);
    globalFilters_[pluginName] = enabled;
}

void EventBus::setEventFilter(EventType type, const char* filter) {
    std::lock_guard<std::mutex> lock(mutex_);
    eventFilters_[type] = filter ? std::string(filter) : "";
}

void EventBus::setPriorityRange(int minPriority, int maxPriority) {
    std::lock_guard<std::mutex> lock(mutex_);
    minPriority_ = minPriority;
    maxPriority_ = maxPriority;
}

const EventBus::Stats& EventBus::getStats() const {
    return stats_;
}

void EventBus::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = Stats{};
}

void EventBus::lock() {
    mutex_.lock();
}

void EventBus::unlock() {
    mutex_.unlock();
}

bool EventBus::tryLock() {
    return mutex_.try_lock();
}

void EventBus::processingLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait for events or shutdown signal
        eventCondition_.wait(lock, [this]() {
            return !eventQueue_.empty() || !running_;
        });

        if (!running_) break;

        // Process events in priority order
        while (!eventQueue_.empty()) {
            auto item = std::move(const_cast<EventQueueItem&>(eventQueue_.top()));
            eventQueue_.pop();

            lock.unlock(); // Unlock while processing

            auto startTime = std::chrono::high_resolution_clock::now();
            processEvent(item.event.get());
            auto endTime = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            lock.lock(); // Re-lock for stats update

            stats_.totalEventsProcessed++;
            stats_.eventsByType[item.event->type]++;

            // Update average processing time
            if (stats_.totalEventsProcessed == 1) {
                stats_.averageProcessingTime = duration.count();
            } else {
                stats_.averageProcessingTime =
                    (stats_.averageProcessingTime * (stats_.totalEventsProcessed - 1) + duration.count()) /
                    stats_.totalEventsProcessed;
            }
        }
    }
}

void EventBus::processEvent(const EventData* event) {
    auto matchingSubs = findMatchingSubscriptions(event);

    // Sort by priority (higher priority first)
    std::sort(matchingSubs.begin(), matchingSubs.end(),
        [](const Subscription* a, const Subscription* b) {
            return a->priority > b->priority;
        });

    // Call callbacks
    for (auto* sub : matchingSubs) {
        if (sub->enabled) {
            try {
                sub->callback(event, sub->userData);
            } catch (const std::exception& e) {
                std::cerr << "EventBus: Exception in event callback: " << e.what() << std::endl;
            }
        }
    }
}

bool EventBus::shouldProcessEvent(const EventData* event) const {
    // Check global filters
    if (event->sourcePlugin) {
        auto it = globalFilters_.find(event->sourcePlugin);
        if (it != globalFilters_.end() && !it->second) {
            return false;
        }
    }

    // Check event filters
    auto filterIt = eventFilters_.find(event->type);
    if (filterIt != eventFilters_.end() && !filterIt->second.empty()) {
        // Simple string matching (could be extended to regex)
        if (event->sourcePlugin && filterIt->second != event->sourcePlugin) {
            return false;
        }
    }

    return true;
}

std::vector<EventBus::Subscription*> EventBus::findMatchingSubscriptions(const EventData* event) {
    std::vector<Subscription*> matching;

    for (auto& sub : subscriptions_) {
        if (sub.type == event->type && sub.matches(event)) {
            matching.push_back(&sub);
        }
    }

    return matching;
}

bool EventBus::Subscription::matches(const EventData* event) const {
    if (!enabled) return false;

    // Check priority range
    // Note: Priority filtering is handled at EventBus level

    // Check filter
    if (!filter.empty()) {
        if (!event->sourcePlugin || filter != event->sourcePlugin) {
            return false;
        }
    }

    return true;
}

} // namespace Ragger
