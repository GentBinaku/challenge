#pragma once

#ifdef _WIN32
    #ifdef PLUGIN_BUILDING
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define PLUGIN_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

PLUGIN_API int plugin_init(void);

PLUGIN_API const char* plugin_get_name(void);

PLUGIN_API int plugin_add(int a, int b);

#ifdef __cplusplus
}
#endif
