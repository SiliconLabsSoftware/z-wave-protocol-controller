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

// Utilities for parsing JSON-encoded MQTT payloads received by command class handlers.

#ifndef ZWAVE_COMMAND_CLASS_MQTT_UTILS_HPP
#define ZWAVE_COMMAND_CLASS_MQTT_UTILS_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "sl_status.h"

#include "log.h"
#include "nlohmann/json.hpp"

namespace zwave_command_class
{
    namespace detail
    {
        template<typename T> struct is_std_vector : std::false_type {};

        template<typename T, typename A> struct is_std_vector<std::vector<T, A>> : std::true_type {};

        template<typename T> constexpr bool is_std_vector_v = is_std_vector<T>::value;

        template<typename T> constexpr bool is_numeric_v = std::is_arithmetic_v<T> || std::is_enum_v<T>;
    }  // namespace detail

    class command_class_mqtt_utils
    {
        public:
            // Convert a JSON value to a numeric type. Accepts JSON numbers, decimal strings
            // ("11", "00011"), and hex strings with 0x prefix ("0x0B"). Bare hex / leading-zero
            // literals are not legal JSON and are rejected upstream by the JSON parser.
            // Throws std::runtime_error on type or parse failure; callers add field context.
            template<typename T> static T parse_numeric(const nlohmann::json &v)
            {
                if (v.is_number_integer() || v.is_number_unsigned()) {
                    return static_cast<T>(v.template get<long long>());
                }
                if (v.is_string()) {
                    const std::string &s = v.template get_ref<const std::string &>();
                    try {
                        int base          = 10;
                        std::size_t start = 0;
                        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
                            base  = 16;
                            start = 2;
                        }
                        return static_cast<T>(std::stoll(s.substr(start), nullptr, base));
                    } catch (const std::exception &e) {
                        throw std::runtime_error(std::string("not a valid numeric string: ") + e.what());
                    }
                }
                throw std::runtime_error("must be a number or numeric string");
            }

            // Compile-time dispatch: numeric/enum scalars go through parse_numeric; std::vector
            // of numeric/enum iterates and parses each element. Anything else fails to compile
            // (see the static_asserts) — for arrays of structured objects use
            // mqtt_payload_parser::parse_array.
            template<typename T> static void extract_typed_value(const nlohmann::json &node, T &out_value)
            {
                if constexpr (detail::is_std_vector_v<T>) {
                    static_assert(detail::is_numeric_v<typename T::value_type>,
                                  "extract_typed_value supports std::vector of numeric/enum only; "
                                  "for vectors of structs use mqtt_payload_parser::parse_array");
                    if (!node.is_array()) {
                        throw std::runtime_error("must be an array");
                    }
                    out_value.clear();
                    out_value.reserve(node.size());
                    for (const auto &elem: node) {
                        out_value.push_back(parse_numeric<typename T::value_type>(elem));
                    }
                } else {
                    static_assert(detail::is_numeric_v<T>,
                                  "extract_typed_value supports numeric/enum or std::vector thereof only; "
                                  "for structured fields use mqtt_payload_parser::parse_nested");
                    out_value = parse_numeric<T>(node);
                }
            }
    };

    // View into one nested object inside an mqtt_payload_parser's cached payload, returned by
    // mqtt_payload_parser::parse_nested. Shares status with the originating parser — failures
    // inside the scope poison the parent. Error logs include the full path
    // (e.g. "properties1.fan_mode").
    class mqtt_payload_scope
    {
        public:
            mqtt_payload_scope(sl_status_t &parent_status, const char *log_tag, const nlohmann::json *node, std::string label_prefix) noexcept : m_status(&parent_status), m_log_tag(log_tag), m_node(node), m_label_prefix(std::move(label_prefix)) {}

            template<typename T> mqtt_payload_scope &parse(const char *key, T &out_value)
            {
                if (*m_status != SL_STATUS_OK) {
                    return *this;
                }
                const std::string label = m_label_prefix + key;
                try {
                    if (m_node == nullptr) {
                        throw std::runtime_error(std::string("required field '") + label + "' is missing");
                    }
                    auto it = m_node->find(key);
                    if (it == m_node->end()) {
                        throw std::runtime_error(std::string("required field '") + label + "' is missing");
                    }
                    command_class_mqtt_utils::extract_typed_value(*it, out_value);
                } catch (const std::exception &e) {
                    sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': %s", label.c_str(), e.what());
                    *m_status = SL_STATUS_FAIL;
                }
                return *this;
            }

            template<typename T> mqtt_payload_scope &parse_optional(const char *key, T &out_value)
            {
                if (*m_status != SL_STATUS_OK || m_node == nullptr) {
                    return *this;
                }
                const std::string label = m_label_prefix + key;
                try {
                    auto it = m_node->find(key);
                    if (it == m_node->end()) {
                        return *this;
                    }
                    command_class_mqtt_utils::extract_typed_value(*it, out_value);
                } catch (const std::exception &e) {
                    sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': %s", label.c_str(), e.what());
                    *m_status = SL_STATUS_FAIL;
                }
                return *this;
            }

            mqtt_payload_scope parse_nested(const char *key)
            {
                const std::string label     = m_label_prefix + key;
                const nlohmann::json *child = nullptr;
                if (*m_status == SL_STATUS_OK) {
                    if (m_node == nullptr) {
                        sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': required object is missing", label.c_str());
                        *m_status = SL_STATUS_FAIL;
                    } else {
                        auto it = m_node->find(key);
                        if (it == m_node->end() || !it->is_object()) {
                            sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': required object is missing", label.c_str());
                            *m_status = SL_STATUS_FAIL;
                        } else {
                            child = &(*it);
                        }
                    }
                }
                return mqtt_payload_scope(*m_status, m_log_tag, child, label + ".");
            }

        private:
            sl_status_t *m_status;
            const char *m_log_tag;
            const nlohmann::json *m_node;  // nullptr when the scope's parent path was missing
            std::string m_label_prefix;    // empty for top-level, "properties1." etc. for nested
    };

    // Aggregate yielded by mqtt_payload_array_view's iterator. Use with structured bindings:
    //   for (auto &&[elem, item] : parser.parse_array("vg", vg)) { ... }
    // where elem is a mqtt_payload_scope rooted at the current JSON element and item is a
    // reference to the just-emplaced T at the back of the output vector. The forwarding-
    // reference (&&) form is required because the iterator yields by value.
    template<typename T> struct mqtt_payload_array_element {
            mqtt_payload_scope scope;
            T &item;
    };

    // Iterable view over a JSON array of structured elements, returned by
    // mqtt_payload_parser::parse_array. Drives a range-based for loop where each iteration
    // yields a {scope, item&} pair (see mqtt_payload_array_element). At the start of each
    // iteration a default-constructed T is appended to out; the body fills item via the
    // yielded scope. If any per-element parse fails, the parser's status is poisoned, the
    // partially-filled element is popped back off, and iteration stops early. Per-element
    // error labels are prefixed with "<key>[i]." (e.g. "vg[0].properties1.end_point").
    template<typename T> class mqtt_payload_array_view
    {
        public:
            mqtt_payload_array_view(sl_status_t &parent_status, const char *log_tag, const nlohmann::json *array_node, const char *key, std::vector<T> &out) noexcept : m_status(&parent_status), m_log_tag(log_tag), m_array(array_node), m_key(key), m_out(&out)
            {
                if (*m_status == SL_STATUS_OK && m_array != nullptr) {
                    m_out->clear();
                    m_out->reserve(m_array->size());
                }
            }

            class iterator
            {
                public:
                    iterator(mqtt_payload_array_view *view, std::size_t index) noexcept : m_view(view), m_index(index) {}

                    mqtt_payload_array_element<T> operator*() const
                    {
                        m_view->m_out->emplace_back();
                        return mqtt_payload_array_element<T> {
                          mqtt_payload_scope {
                            *m_view->m_status,
                            m_view->m_log_tag,
                            &(*m_view->m_array)[m_index],
                            std::string(m_view->m_key) + "[" + std::to_string(m_index) + "].",
                          },
                          m_view->m_out->back(),
                        };
                    }

                    iterator &operator++() noexcept
                    {
                        if (*m_view->m_status != SL_STATUS_OK && !m_view->m_out->empty()) {
                            m_view->m_out->pop_back();
                        }
                        ++m_index;
                        return *this;
                    }

                    bool operator!=(const iterator &other) const noexcept
                    {
                        if (*m_view->m_status != SL_STATUS_OK) {
                            return false;
                        }
                        return m_index != other.m_index;
                    }

                private:
                    mqtt_payload_array_view *m_view;
                    std::size_t m_index;
            };

            iterator begin() noexcept
            {
                return iterator(this, 0);
            }
            iterator end() noexcept
            {
                return iterator(this, (m_array != nullptr) ? m_array->size() : 0);
            }

        private:
            sl_status_t *m_status;
            const char *m_log_tag;
            const nlohmann::json *m_array;  // nullptr when the field was missing / not an array / parser poisoned
            const char *m_key;
            std::vector<T> *m_out;
    };

    // Stateful, chainable MQTT payload parser. Parses the JSON payload once on construction
    // and caches it. After the first failure the parser is "poisoned" — subsequent calls are
    // no-ops, so the caller only needs to check status() once at the end. All errors are
    // logged via sl_log_error against the constructor's log_tag.
    //
    // Four call shapes:
    //   parse(key, out)          — required leaf
    //   parse_optional(key, out) — optional leaf (missing key → no-op)
    //   parse_nested(key)        — descend into a required nested object, returns a scope
    //   parse_array(key, vec)    — iterate a required array of structured objects; use as the
    //                              range expression of a range-based for loop with a
    //                              structured binding [scope, item] (item is a reference into
    //                              vec.back() — the newly emplaced element)
    //
    // Example:
    //   mqtt_payload_parser parser{payload, LOG_TAG.data()};
    //   parser.parse("operation_type", operation_type)
    //         .parse_optional("hold_and_release_time", hold_and_release_time);
    //   parser.parse_nested("properties1").parse("fan_mode", fan_mode);
    //   for (auto &&[elem, item] : parser.parse_array("vg1", vg1)) {
    //       elem.parse("indicator_id", item.indicator_id);
    //   }
    //   if (parser.status() != SL_STATUS_OK) {
    //       return parser.status();
    //   }
    class mqtt_payload_parser
    {
        public:
            mqtt_payload_parser(const std::string &payload, const char *log_tag);

            template<typename T> mqtt_payload_parser &parse(const char *key, T &out_value)
            {
                if (m_status != SL_STATUS_OK) {
                    return *this;
                }
                try {
                    auto it = m_payload.find(key);
                    if (it == m_payload.end()) {
                        throw std::runtime_error(std::string("required field '") + key + "' is missing");
                    }
                    command_class_mqtt_utils::extract_typed_value(*it, out_value);
                } catch (const std::exception &e) {
                    sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': %s", key, e.what());
                    m_status = SL_STATUS_FAIL;
                }
                return *this;
            }

            template<typename T> mqtt_payload_parser &parse_optional(const char *key, T &out_value)
            {
                if (m_status != SL_STATUS_OK) {
                    return *this;
                }
                try {
                    auto it = m_payload.find(key);
                    if (it == m_payload.end()) {
                        return *this;
                    }
                    command_class_mqtt_utils::extract_typed_value(*it, out_value);
                } catch (const std::exception &e) {
                    sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': %s", key, e.what());
                    m_status = SL_STATUS_FAIL;
                }
                return *this;
            }

            mqtt_payload_scope parse_nested(const char *key)
            {
                const nlohmann::json *child = nullptr;
                if (m_status == SL_STATUS_OK) {
                    auto it = m_payload.find(key);
                    if (it == m_payload.end() || !it->is_object()) {
                        sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': required object is missing", key);
                        m_status = SL_STATUS_FAIL;
                    } else {
                        child = &(*it);
                    }
                }
                return mqtt_payload_scope(m_status, m_log_tag, child, std::string(key) + ".");
            }

            // Iterate a required JSON array of structured elements. Returns a view to be used
            // as the range expression of a range-based for loop with a structured binding
            // [scope, item]; each iteration appends a default-constructed T to out and binds
            // item to it. The caller fills item via the scope; if the iteration fails, the
            // appended element is popped back off. See mqtt_payload_array_view for failure
            // semantics.
            template<typename T> mqtt_payload_array_view<T> parse_array(const char *key, std::vector<T> &out)
            {
                const nlohmann::json *array_node = nullptr;
                if (m_status == SL_STATUS_OK) {
                    auto it = m_payload.find(key);
                    if (it == m_payload.end()) {
                        sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': required field is missing", key);
                        m_status = SL_STATUS_FAIL;
                    } else if (!it->is_array()) {
                        sl_log_error(m_log_tag, "Failed to parse MQTT payload field '%s': must be an array", key);
                        m_status = SL_STATUS_FAIL;
                    } else {
                        array_node = &(*it);
                    }
                }
                return mqtt_payload_array_view<T>(m_status, m_log_tag, array_node, key, out);
            }

            sl_status_t status() const noexcept
            {
                return m_status;
            }

        private:
            const char *m_log_tag;
            sl_status_t m_status;
            nlohmann::json m_payload;  // valid only when m_status == SL_STATUS_OK
    };
}  // namespace zwave_command_class

#endif  // ZWAVE_COMMAND_CLASS_MQTT_UTILS_HPP
