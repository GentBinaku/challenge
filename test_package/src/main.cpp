#include <plugin/plugin.h>
#include <plugin_segfault/plugin_segfault.h>

#include <cstdio>

int main() {
    std::printf("plugin name: %s\n", plugin_get_name());
    std::printf("plugin_segfault name: %s\n", plugin_segfault_get_name());
    return 0;
}
