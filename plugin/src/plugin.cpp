#include "plugin/plugin.h"

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace {

struct LogInitializer {
    LogInitializer() {
        boost::log::add_common_attributes();
        boost::log::add_console_log();
    }
};

void ensure_log_initialized() {
    static LogInitializer init;
}

} // namespace

extern "C" {

PLUGIN_API int plugin_init(void) {
    ensure_log_initialized();
    BOOST_LOG_TRIVIAL(info) << "Plugin loaded and initialized successfully!";
    return 0;
}

PLUGIN_API const char* plugin_get_name(void) {
    return "challange_plugin";
}

PLUGIN_API int plugin_add(int a, int b) {
    BOOST_LOG_TRIVIAL(debug) << "plugin_add(" << a << ", " << b << ") called";
    return a + b;
}

}
