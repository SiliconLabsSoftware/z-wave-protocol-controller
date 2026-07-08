/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef INIT_BUILDER_HPP
#define INIT_BUILDER_HPP

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include "sl_status.h"
#include "log.h"
#include "threading.hpp"

// Log tag for init_builder
[[maybe_unused]] static constexpr std::string_view INIT_BUILDER_LOG_TAG = "main";

// Note: log.h should be included before this header for logging to work.

/**
 * @brief Base interface for initializable components
 *
 * Components that need initialization should implement this interface
 * or use the function pointer wrappers provided by InitBuilder.
 */
class Initializable
{
    public:
        virtual ~Initializable() = default;

        /**
         * @brief Initialize the component
         * @return SL_STATUS_OK on success, other status on failure
         */
        virtual sl_status_t initialize() = 0;

        /**
         * @brief Shutdown/teardown the component
         * @return 0 on success, non-zero on error
         */
        virtual int shutdown() = 0;

        /**
         * @brief Get the name of the component for logging
         * @return Component name
         */
        virtual std::string name() const = 0;
};

/**
 * @brief Wrapper for C-style fixture functions
 *
 * This allows C fixture functions to be used with the C++ builder pattern.
 */
class FixtureWrapper : public Initializable
{
    public:
        using SetupFunc    = std::function<sl_status_t()>;
        using ShutdownFunc = std::function<int()>;

        FixtureWrapper(const std::string &component_name, SetupFunc setup_func, ShutdownFunc shutdown_func = nullptr) : name_(component_name), setup_func_(setup_func), shutdown_func_(shutdown_func) {}

        sl_status_t initialize() override
        {
            if (!setup_func_) {
                return SL_STATUS_FAIL;
            }
            return setup_func_();
        }

        int shutdown() override
        {
            if (shutdown_func_) {
                return shutdown_func_();
            }
            return 0;
        }

        std::string name() const override
        {
            return name_;
        }

    private:
        std::string name_;
        SetupFunc setup_func_;
        ShutdownFunc shutdown_func_;
};

/**
 * @brief Builder pattern for component initialization
 *
 * Provides a fluent interface for building and executing initialization sequences.
 * Components are initialized in the order they are added, and shutdown in reverse order.
 */
class InitBuilder
{
    public:
        /**
         * @brief Add a component using C-style function pointers
         *
         * @param name Component name for logging
         * @param setup_func Setup function (can be nullptr)
         * @param shutdown_func Shutdown function (optional, can be nullptr)
         * @return Reference to this builder for method chaining
         */
        InitBuilder &add(const std::string &name, std::function<sl_status_t()> setup_func, std::function<int()> shutdown_func = nullptr)
        {
            component_inventory.push_back(std::make_unique<FixtureWrapper>(name, setup_func, shutdown_func));
            return *this;
        }

        /**
         * @brief Add a component using C function pointer (for compatibility)
         *
         * @param name Component name for logging
         * @param setup_func C-style setup function pointer
         * @param shutdown_func C-style shutdown function pointer (optional)
         * @return Reference to this builder for method chaining
         */
        InitBuilder &add(const std::string &name, sl_status_t (*setup_func)(), int (*shutdown_func)() = nullptr)
        {
            std::function<sl_status_t()> setup = setup_func ? std::function<sl_status_t()>(setup_func) : std::function<sl_status_t()>();
            std::function<int()> shutdown      = shutdown_func ? std::function<int()>(shutdown_func) : std::function<int()>();
            return add(name, setup, shutdown);
        }

        /**
         * @brief Add a component implementing Initializable interface
         *
         * @param component Unique pointer to the component
         * @return Reference to this builder for method chaining
         */
        InitBuilder &add(std::unique_ptr<Initializable> component)
        {
            component_inventory.push_back(std::move(component));
            return *this;
        }

        /**
         * @brief Initialize all components in the order they were added
         *
         * Stops on first failure and returns the error status.
         * After all components are initialized, automatically starts threads for
         * any component that inherits from threading::threading. This allows
         * components to register callbacks before threads start processing events.
         *
         * @return SL_STATUS_OK if all components initialized successfully,
         *         otherwise returns the first error status encountered
         */
        sl_status_t initialize_all()
        {
            // First phase: initialize all components
            for (auto &comp: component_inventory) {
                sl_status_t status = comp->initialize();
                if (status == SL_STATUS_OK) {
                    sl_log_info(INIT_BUILDER_LOG_TAG.data(), "Completed: %s\n", comp->name().c_str());
                    continue;
                } else if (status == SL_STATUS_NOT_AVAILABLE) {
                    // Non-critical error, continue
                    sl_log_warning(INIT_BUILDER_LOG_TAG.data(), "Non critical error in setup for: %s. Continuing setup.\n", comp->name().c_str());
                    continue;
                } else if (status == SL_STATUS_ABORT) {
                    // Startup aborted
                    sl_log_debug(INIT_BUILDER_LOG_TAG.data(), "Startup sequence aborted by: %s.\n", comp->name().c_str());
                    return status;
                } else {
                    // Critical error
                    sl_log_critical(INIT_BUILDER_LOG_TAG.data(), "Failed [%d]: %s.\n", status, comp->name().c_str());
                    return status;
                }
            }

            // Second phase: start threads after all components are initialized
            // This allows components to register callbacks before threads start processing events
            // Check if component inherits from threading::threading and start it if so
            for (auto &comp: component_inventory) {
                threading::threading *threading_comp = dynamic_cast<threading::threading *>(comp.get());
                if (threading_comp != nullptr) {
                    threading_comp->start();
                }
            }

            return SL_STATUS_OK;
        }

        /**
         * @brief Shutdown all components in reverse order
         *
         * Shutdowns are executed in reverse order of initialization.
         * Errors during shutdown are logged but don't stop the process.
         *
         * @return Number of components that failed to shutdown
         */
        int shutdown_all()
        {
            int error_count = 0;
            // Shutdown in reverse order
            for (auto it = component_inventory.rbegin(); it != component_inventory.rend(); ++it) {
                int result = (*it)->shutdown();
                sl_log_info(INIT_BUILDER_LOG_TAG.data(), "Shutdown %s\n", (*it)->name().c_str());
                if (result != 0) {
                    error_count++;
                }
            }
            return error_count;
        }

        /**
         * @brief Get the number of registered components
         * @return Number of components
         */
        size_t size() const
        {
            return component_inventory.size();
        }

        /**
         * @brief Clear all registered components
         */
        void clear()
        {
            component_inventory.clear();
        }

    private:
        std::vector<std::unique_ptr<Initializable>> component_inventory;
};

#endif  // INIT_BUILDER_HPP
