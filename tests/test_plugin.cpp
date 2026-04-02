#include <gtest/gtest.h>
#include "plugin/plugin.h"

#include <cstdlib>
#include <filesystem>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace {

using plugin_init_fn = int (*)();
using plugin_get_name_fn = const char* (*)();
using plugin_add_fn = int (*)(int, int);

class PluginLoader {
public:
    explicit PluginLoader(const std::string& path) {
#ifdef _WIN32
        handle_ = LoadLibraryA(path.c_str());
#else
        handle_ = dlopen(path.c_str(), RTLD_LAZY);
#endif
    }

    ~PluginLoader() {
        if (!handle_) return;
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif
    }

    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;

    template <typename T>
    T get_symbol(const char* name) const {
        if (!handle_) return nullptr;
#ifdef _WIN32
        return reinterpret_cast<T>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#else
        return reinterpret_cast<T>(dlsym(handle_, name));
#endif
    }

    [[nodiscard]] bool is_loaded() const { return handle_ != nullptr; }

private:
    void* handle_ = nullptr;
};

std::string get_plugin_path() {
    const char* env = std::getenv("PLUGIN_PATH");
    if (env) return env;

    auto exe_dir = std::filesystem::path(".");
#ifdef _WIN32
    return (exe_dir / "plugin.dll").string();
#elif defined(__APPLE__)
    return (exe_dir / "libplugin.dylib").string();
#else
    return (exe_dir / "libplugin.so").string();
#endif
}

class PluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader_ = std::make_unique<PluginLoader>(get_plugin_path());
        ASSERT_TRUE(loader_->is_loaded()) << "Plugin library not found. "
            "Set PLUGIN_PATH or run from the build/bin directory.";
    }

    void TearDown() override {
        loader_.reset();
    }

    std::unique_ptr<PluginLoader> loader_;
};

TEST_F(PluginTest, PluginLoadsSuccessfully) {
    EXPECT_TRUE(loader_->is_loaded());
}

TEST_F(PluginTest, InitReturnsZero) {
    auto fn = loader_->get_symbol<plugin_init_fn>("plugin_init");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn(), 0);
}

TEST_F(PluginTest, GetNameReturnsExpected) {
    auto fn = loader_->get_symbol<plugin_get_name_fn>("plugin_get_name");
    ASSERT_NE(fn, nullptr);
    EXPECT_STREQ(fn(), "challange_plugin");
}

TEST_F(PluginTest, AddReturnsCorrectSum) {
    auto fn = loader_->get_symbol<plugin_add_fn>("plugin_add");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn(2, 3), 5);
    EXPECT_EQ(fn(-1, 1), 0);
    EXPECT_EQ(fn(0, 0), 0);
    EXPECT_EQ(fn(100, 200), 300);
}

TEST_F(PluginTest, AllSymbolsResolvable) {
    EXPECT_NE(loader_->get_symbol<plugin_init_fn>("plugin_init"), nullptr);
    EXPECT_NE(loader_->get_symbol<plugin_get_name_fn>("plugin_get_name"), nullptr);
    EXPECT_NE(loader_->get_symbol<plugin_add_fn>("plugin_add"), nullptr);
}

} // namespace
