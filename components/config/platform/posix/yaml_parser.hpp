/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

/**
 * @file yaml_parser.hpp
 * @brief YAML Parser for program options config file
 *
 * Parse YAML configuration file for ZPC.
 * Each configuration name is prepended with its parent, e.g. for following YAML
 * mqtt:
 *     host: 127.0.0.1
 *     port: 1883
 * zpc:
 *     serial_port: /dev/ttyUSB0
 * will parse to following configurations: "mqtt.host", "mqtt.port",
 * and "zpc.serial_port".
 *
 *
 * @{
 */

#ifndef YAML_PARSER_HPP
#define YAML_PARSER_HPP

#include <algorithm>
#include <set>
#include <string>
#include <stdexcept>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace config
{
    /** Single parsed option: key and one or more string values. */
    struct ParsedOption {
            std::string string_key;
            std::vector<std::string> value;
            bool unregistered = false;
    };

    /** Parsed options from YAML. */
    struct ParsedOptions {
            std::vector<ParsedOption> options;
    };
}  // namespace config

class yaml_parser
{
    public:
        /**
         * @brief Parse YAML configuration from istream
         *
         * @param istrm Istream containing YAML configuration
         * @param valid_keys Set of option names to accept; keys not in this set are ignored
         * @return config::ParsedOptions Parsed options (std-based)
         */
        static config::ParsedOptions parse(std::istream &istrm, const std::set<std::string> &valid_keys)
        {
            YAML::Node config = YAML::Load(istrm);
            return parse(config, valid_keys);
        }

        /**
         * @brief Parse YAML configuration from YAML::Node
         *
         * @param node YAML::Node containing YAML configuration
         * @param valid_keys Set of option names to accept; keys not in this set are ignored
         * @return config::ParsedOptions Parsed options (std-based)
         */
        static config::ParsedOptions parse(const YAML::Node &node, const std::set<std::string> &valid_keys)
        {
            config::ParsedOptions result;
            parse_subnode(node, "", result, valid_keys);
            return result;
        }

    protected:
        static void parse_subnode(const YAML::Node &node, const std::string &key, config::ParsedOptions &result, const std::set<std::string> &valid_keys)
        {
            switch (node.Type()) {
                case YAML::NodeType::Scalar:
                    add_option(key, node.as<std::string>(), result, valid_keys);
                    break;
                case YAML::NodeType::Sequence:
                    parse_subnode_sequence(node, key, result, valid_keys);
                    break;
                case YAML::NodeType::Map:
                    parse_subnode_map(node, key, result, valid_keys);
                    break;
                default:
                    throw std::invalid_argument("Unsupported node type");
                    break;
            }
        }

        static void parse_subnode_sequence(const YAML::Node &node, const std::string &key, config::ParsedOptions &result, const std::set<std::string> &valid_keys)
        {
            for (const auto &subnode: node) {
                parse_subnode(subnode, key, result, valid_keys);
            }
        }

        static void parse_subnode_map(const YAML::Node &node, const std::string &key, config::ParsedOptions &result, const std::set<std::string> &valid_keys)
        {
            for (const auto &pair: node) {
                std::string node_key = pair.first.as<std::string>();
                try {
                    std::stoul(node_key);  // check if we have an integer
                    node_key = key;        // treat this as if it were a Sequence
                } catch (std::invalid_argument &e) {
                    // not an integer
                    if (!key.empty()) {
                        node_key = key + '.' + node_key;
                    }
                }
                parse_subnode(pair.second, node_key, result, valid_keys);
            }
        }

        static void add_option(const std::string &key, const std::string &value, config::ParsedOptions &result, const std::set<std::string> &valid_keys)
        {
            if (key.empty()) {
                throw std::logic_error("Empty key - malformed Config file");
            }
            if (valid_keys.count(key) == 0) {
                // Key not in allowed set; ignore (config file may be shared among modules).
                return;
            }
            auto option_iter = std::find_if(result.options.begin(), result.options.end(), [&key](const config::ParsedOption &test) { return test.string_key == key; });

            if (option_iter == result.options.end()) {
                result.options.emplace_back();
                option_iter               = result.options.end() - 1;
                option_iter->string_key   = key;
                option_iter->unregistered = false;  // we only add keys from valid_keys
            }

            option_iter->value.push_back(value);
        }
};

#endif  // YAML_PARSER_HPP
/** @} end yaml_parser */
