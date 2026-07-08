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
#include "config.h"
#include "log.h"
#include <libgen.h>

#include <any>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <iomanip>
#include "yaml_parser.hpp"

namespace
{
    enum class OptionKind {
        Int,
        Double,
        Bool,
        String,
        Flag,
    };

    struct OptionInfo {
            std::string name;
            OptionKind kind;
            std::any default_value;
            std::string help;
    };

    bool parse_bool(const std::string &s)
    {
        if (s == "1" || s == "true" || s == "yes" || s == "on") {
            return true;
        }
        if (s == "0" || s == "false" || s == "no" || s == "off") {
            return false;
        }
        std::cerr << "Invalid boolean value: " << s << std::endl;
        return false;
    }
}  // namespace

std::string mapper(std::string env_var)
{
    std::transform(env_var.begin(), env_var.end(), env_var.begin(), ::toupper);
    if (env_var == "ZPC_CONF") {
        std::cout << "Environment variable ZPC_CONF is found: \n";
        return "conf";
    }
    return "";
}

/**
 * @brief Config class supporting adding, removing and parsing configurations
 *
 * Configurations are parsed from both command line and config file.
 * If the same option is given in both command line and config file,
 * the command line option takes precedence
 */
class Config
{
    private:
        std::vector<OptionInfo> options_;
        std::map<std::string, size_t> name_to_index_;
        std::map<std::string, std::pair<std::any, bool>> values_;  // value + was_explicitly_set (CLI wins over file)
        std::set<std::string> valid_keys;

        void set_value(const std::string &name, std::any value, bool explicitly_set)
        {
            auto it = name_to_index_.find(name);
            if (it == name_to_index_.end()) {
                return;
            }
            // Only overwrite if not already set from CLI (command line takes precedence)
            if (explicitly_set) {
                values_[name] = {std::move(value), true};
            } else {
                if (!values_.contains(name) || !values_[name].second) {
                    values_[name] = {std::move(value), false};
                }
            }
        }

        bool set_value_from_string(const std::string &name, const std::string &str, bool explicitly_set)
        {
            auto it = name_to_index_.find(name);
            if (it == name_to_index_.end()) {
                return false;
            }
            const OptionKind kind = options_[it->second].kind;
            try {
                if (kind == OptionKind::Int) {
                    set_value(name, std::stoi(str), explicitly_set);
                } else if (kind == OptionKind::Double) {
                    set_value(name, std::stod(str), explicitly_set);
                } else if (kind == OptionKind::Bool) {
                    set_value(name, parse_bool(str), explicitly_set);
                } else if (kind == OptionKind::String || kind == OptionKind::Flag) {
                    set_value(name, str, explicitly_set);
                } else {
                    return false;
                }
            } catch (...) {
                return false;
            }
            return true;
        }

    public:
        Config()
        {
            this->config_add(CONFIG_KEY_LOG_LEVEL, "Log Level (d,i,w,e,c)", std::string("i"));
            this->config_add(CONFIG_KEY_LOG_TAG_LEVEL, "Tag-based log level\nFormat: <tag>:<severity>, <tag>:<severity>, ...", std::string(""));
        }

        template<typename T> config_status_t config_add(const char *name, const char *help, const T &default_value)
        {
            valid_keys.insert(name);
            OptionKind kind = OptionKind::String;
            if (std::is_same_v<T, int>) {
                kind = OptionKind::Int;
            } else if (std::is_same_v<T, double>) {
                kind = OptionKind::Double;
            } else if (std::is_same_v<T, bool>) {
                kind = OptionKind::Bool;
            } else if (std::is_same_v<T, std::string>) {
                kind = OptionKind::String;
            }
            name_to_index_[name] = options_.size();
            options_.push_back({name, kind, std::any(default_value), help});
            values_[name] = {std::any(default_value), false};
            return CONFIG_STATUS_OK;
        }

        config_status_t config_add(const char *name, const char *help)
        {
            valid_keys.insert(name);
            name_to_index_[name] = options_.size();
            options_.push_back({name, OptionKind::Flag, std::any(false), help});
            values_[name] = {std::any(false), false};
            return CONFIG_STATUS_OK;
        }

        void add_mqtt_config_default(char **argv)
        {
            this->config_add(CONFIG_KEY_MQTT_HOST, "MQTT broker hostname or IP", std::string("localhost"));
            this->config_add(CONFIG_KEY_MQTT_PORT, "MQTT broker port", 1883);
            this->config_add(CONFIG_KEY_MQTT_CAFILE,
                             "Path to file containing the PEM-encoded CA certificate "
                             "to connect to Mosquitto MQTT broker for TLS encryption",
                             std::string(""));
            this->config_add(CONFIG_KEY_MQTT_CERTFILE,
                             "Path to file containing the PEM-encoded client "
                             "certificate to connect to Mosquitto MQTT broker for TLS encryption",
                             std::string(""));
            this->config_add(CONFIG_KEY_MQTT_KEYFILE,
                             "Path to a file containing the PEM-encoded unencrypted "
                             "private key for this client",
                             std::string(""));
            this->config_add(CONFIG_KEY_MQTT_CLIENT_ID, "Set the MQTT client ID of the application.", std::string(basename(argv[0])));
        }

        static void dump_yaml_key_value(std::ostream &out, std::vector<std::string> key_parts, std::vector<std::string> last_key_parts, std::any value, size_t indent)
        {
            if (key_parts.size() == 1) {
                if (indent > 0) {
                    out << std::setfill(' ') << std::setw(indent * 2) << ' ';
                }
                out << key_parts.at(0) << ": ";
                if (value.type() == typeid(int)) {
                    out << std::any_cast<int>(value);
                } else if (value.type() == typeid(std::string)) {
                    out << "'" << std::any_cast<std::string>(value) << "'";
                } else if (value.type() == typeid(double)) {
                    out << std::any_cast<double>(value);
                } else if (value.type() == typeid(bool)) {
                    out << (std::any_cast<bool>(value) ? "true" : "false");
                } else {
                    assert(0);
                }
                out << std::endl;
            } else {
                if (last_key_parts.back() == key_parts.back()) {
                    last_key_parts.pop_back();
                } else {
                    if (indent > 0) {
                        out << std::setfill(' ') << std::setw(indent * 2) << ' ';
                    }
                    out << key_parts.back() << ":" << std::endl;
                }
                key_parts.pop_back();
                indent++;
                dump_yaml_key_value(out, key_parts, last_key_parts, value, indent);
            }
        }

        void dump_config(std::ostream &out) const
        {
            std::vector<std::string> last_key_parts;
            out << "# ZPC sample conf file" << std::endl << std::endl;

            for (const auto &[key, pr]: values_) {
                if (key == "dump-config") {
                    continue;
                }
                const std::any &value = pr.first;
                if (value.type() == typeid(void)) {
                    continue;
                }

                std::vector<std::string> key_parts;
                {
                    std::istringstream ss(key);
                    std::string part;
                    while (std::getline(ss, part, '.')) {
                        key_parts.push_back(part);
                    }
                }
                std::reverse(key_parts.begin(), key_parts.end());
                dump_yaml_key_value(out, key_parts, last_key_parts, value, 0);
                last_key_parts = key_parts;
            }
        }

        void parse_environment(const std::function<std::string(std::string)> &name_mapper)
        {
#if !defined(_WIN32) && !defined(_WIN64)
#ifndef __linux__
            extern char **environ;
#endif
#endif
            for (char **env = environ; *env != nullptr; ++env) {
                std::string entry(*env);
                size_t eq = entry.find('=');
                if (eq == std::string::npos) {
                    continue;
                }
                std::string var_name = entry.substr(0, eq);
                std::string opt_name = name_mapper(var_name);
                if (opt_name.empty()) {
                    continue;
                }
                std::string var_value = entry.substr(eq + 1);
                set_value_from_string(opt_name, var_value, false);
            }
        }

        bool parse_command_line(int argc, char **argv)
        {
            for (int i = 1; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg.size() < 2 || arg.substr(0, 2) != "--") {
                    continue;
                }
                std::string key = arg.substr(2);
                std::string value_str;
                size_t eq = key.find('=');
                if (eq != std::string::npos) {
                    value_str = key.substr(eq + 1);
                    key       = key.substr(0, eq);
                } else {
                    auto it = name_to_index_.find(key);
                    if (it != name_to_index_.end() && options_[it->second].kind == OptionKind::Flag) {
                        set_value(key, std::any(true), true);
                        continue;
                    }
                    if (i + 1 < argc) {
                        value_str = argv[i + 1];
                        ++i;
                    }
                }
                if (!value_str.empty()) {
                    set_value_from_string(key, value_str, true);
                }
            }
            return true;
        }

        std::string format_help(const char *argv0) const
        {
            std::ostringstream out;
            out << "\nUsage: " << argv0 << " [Options]\n\n";
            for (const auto &opt: options_) {
                out << "  --" << opt.name;
                if (opt.kind != OptionKind::Flag) {
                    out << " <arg>";
                }
                out << "\n      " << opt.help << "\n";
            }
            out << std::endl;
            return out.str();
        }

        config_status_t config_parse(int argc, char **argv, const char *version, std::ostream &out)
        {
            bool dump_requested = false;

            // Add cmdline and MQTT options once (they are required for parsing)
            if (!name_to_index_.contains("conf")) {
                config_add("conf", "Config file in YAML format. ZPC_CONF env variable can be set to override the default config file path", std::string(DEFAULT_CONFIG_PATH));
                config_add("help", "Print this help message and quit");
                config_add("dump-config", "Dump the current configuration in a YAML config file format that can be passed to the --conf option");
                config_add("version", "Print version information and quit");
                add_mqtt_config_default(argv);
            }

            // Reset to defaults (all options now registered)
            values_.clear();
            for (const auto &opt: options_) {
                values_[opt.name] = {opt.default_value, false};
            }

            parse_environment(std::function<std::string(std::string)>(mapper));

            std::string conf_path;
            try {
                if ((values_.contains("conf")) && values_["conf"].first.type() == typeid(std::string)) {
                    conf_path = std::any_cast<std::string>(values_["conf"].first);
                }
            } catch (...) {
            }
            if (conf_path.empty()) {
                std::cout << "Empty ZPC_CONF env found. Consider unsetting the env variable\n";
            }

            try {
                parse_command_line(argc, argv);

                if ((values_.contains("help")) && values_["help"].first.type() == typeid(bool) && std::any_cast<bool>(values_["help"].first)) {
                    out << format_help(argv[0]);
                    return CONFIG_STATUS_INFO_MESSAGE;
                }
                if ((values_.contains("version")) && values_["version"].first.type() == typeid(bool) && std::any_cast<bool>(values_["version"].first)) {
                    out << "Version: " << version << std::endl;
                    return CONFIG_STATUS_INFO_MESSAGE;
                }
                if ((values_.contains("dump-config")) && values_["dump-config"].first.type() == typeid(bool) && std::any_cast<bool>(values_["dump-config"].first)) {
                    dump_requested = true;
                }
                if ((values_.contains("conf")) && values_["conf"].first.type() == typeid(std::string)) {
                    const std::string config_file = std::any_cast<std::string>(values_["conf"].first);
                    std::cout << "Checking for config file: " << config_file << std::endl;
                    std::ifstream istrm(config_file);
                    if (istrm) {
                        std::cout << "Using config file: " << config_file << std::endl;
                        config::ParsedOptions our_parsed = yaml_parser::parse(istrm, valid_keys);
                        for (const auto &opt: our_parsed.options) {
                            if (!opt.value.empty()) {
                                set_value_from_string(opt.string_key, opt.value[0], false);
                            }
                        }
                    } else {
                        std::cout << "Warning! config file [" << config_file << "] not found! using default configurations" << std::endl;
                    }
                }
            } catch (const std::exception &e) {
                std::cerr << "error: " << e.what() << std::endl;
                return CONFIG_STATUS_ERROR;
            } catch (...) {
                std::cerr << "Exception of unknown type!" << std::endl;
                return CONFIG_STATUS_ERROR;
            }

            if (dump_requested) {
                dump_config(out);
                return CONFIG_STATUS_INFO_MESSAGE;
            }
            return CONFIG_STATUS_OK;
        }

        config_status_t has_flag(const char *name)
        {
            auto it = values_.find(name);
            if (it == values_.end()) {
                return CONFIG_STATUS_ERROR;
            }
            if (it->second.first.type() == typeid(bool) && std::any_cast<bool>(it->second.first)) {
                return CONFIG_STATUS_OK;
            }
            return CONFIG_STATUS_ERROR;
        }

        template<typename T> config_status_t config_get(const char *name, T &result)
        {
            auto it = values_.find(name);
            if (it == values_.end()) {
                return CONFIG_STATUS_DOES_NOT_EXIST;
            }
            try {
                result = std::any_cast<T>(it->second.first);
            } catch (const std::bad_any_cast &) {
                return CONFIG_STATUS_INVALID_TYPE;
            }
            return CONFIG_STATUS_OK;
        }

        config_status_t config_get_as_string(const char *name, const char **result)
        {
            auto it = values_.find(name);
            if (it == values_.end()) {
                return CONFIG_STATUS_DOES_NOT_EXIST;
            }
            try {
                *result = std::any_cast<const std::string &>(it->second.first).c_str();
            } catch (const std::bad_any_cast &) {
                return CONFIG_STATUS_INVALID_TYPE;
            }
            return CONFIG_STATUS_OK;
        }
};

static Config *config_object;

class Config_singleton
{
    public:
        Config_singleton()
        {
            config_object = new Config();
        }
        ~Config_singleton()
        {
            delete config_object;
            config_object = nullptr;
        }
};
static Config_singleton config_singleton;

void config_reset()
{
    delete config_object;
    config_object = new Config();
}

config_status_t config_add_string(const char *name, const char *help, const char *default_value)
{
    return config_object->config_add(name, help, std::string(default_value));
}

config_status_t config_add_int(const char *name, const char *help, int default_value)
{
    return config_object->config_add(name, help, default_value);
}

config_status_t config_add_double(const char *name, const char *help, double default_value)
{
    return config_object->config_add(name, help, default_value);
}

config_status_t config_add_bool(const char *name, const char *help, bool default_value)
{
    return config_object->config_add(name, help, default_value);
}

config_status_t config_add_flag(const char *name, const char *help)
{
    return config_object->config_add(name, help);
}

config_status_t config_parse_with_output(int argc, char **argv, const char *version, std::ostream &out)
{
    return config_object->config_parse(argc, argv, version, out);
}

config_status_t config_parse(int argc, char **argv, const char *version)
{
    return config_parse_with_output(argc, argv, version, std::cout);
}

config_status_t config_get_as_string(const char *name, const char **result)
{
    return config_object->config_get_as_string(name, result);
}

config_status_t config_get_as_int(const char *name, int *result)
{
    return config_object->config_get(name, *result);
}

config_status_t config_get_as_double(const char *name, double *result)
{
    return config_object->config_get(name, *result);
}

config_status_t config_get_as_bool(const char *name, bool *result)
{
    return config_object->config_get(name, *result);
}

config_status_t config_has_flag(const char *name)
{
    return config_object->has_flag(name);
}

config_status_t config_apply_log_settings(void)
{
    const char *log_level_str = nullptr;
    const char *tag_level_str = nullptr;
    if (config_get_as_string(CONFIG_KEY_LOG_LEVEL, &log_level_str) != CONFIG_STATUS_OK) {
        log_level_str = nullptr;
    }
    if (config_get_as_string(CONFIG_KEY_LOG_TAG_LEVEL, &tag_level_str) != CONFIG_STATUS_OK) {
        tag_level_str = nullptr;
    }
    if (sl_log_apply_config(log_level_str, tag_level_str) != SL_STATUS_OK) {
        return CONFIG_STATUS_ERROR;
    }
    return CONFIG_STATUS_OK;
}
