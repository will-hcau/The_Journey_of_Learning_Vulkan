#ifndef THE_JOURNEY_OF_LEARNING_VULKAN_INCLUDE_LOG_H_
#define THE_JOURNEY_OF_LEARNING_VULKAN_INCLUDE_LOG_H_

#include <iostream>

#define LOG_INFO(message) \
    do \
    { \
        std::cout << "[INFO] " << message << " (" << __func__ << ':' << __LINE__ << ")\n"; \
    } while (false)

#define LOG_DEBUG(message) \
    do \
    { \
        std::cout << "[DEBUG] " << message << " (" << __func__ << ':' << __LINE__ << ")\n"; \
    } while (false)

#define LOG_ERROR(message) \
    do \
    { \
        std::cerr << "[ERROR] " << message << " (" << __func__ << ':' << __LINE__ << ")\n"; \
    } while (false)

#endif  // THE_JOURNEY_OF_LEARNING_VULKAN_INCLUDE_LOG_H_
