#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

namespace {

using plugin_init_fn    = int (*)();
using plugin_get_name_fn = const char* (*)();

class PluginLoader {
public:
    explicit PluginLoader(const std::string& path) {
#ifdef _WIN32
        handle_ = LoadLibraryA(path.c_str());
#else
        handle_ = dlopen(path.c_str(), RTLD_NOW);
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

    PluginLoader(const PluginLoader&)            = delete;
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

// Returns all crash_*.txt files found in dir written after snapshot was taken.
std::vector<std::filesystem::path> find_new_crash_files(
    const std::filesystem::path& dir,
    const std::filesystem::file_time_type& since)
{
    std::vector<std::filesystem::path> found;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const auto name = entry.path().filename().string();
        if (name.starts_with("crash_") && name.ends_with(".txt") &&
            entry.last_write_time() >= since)
        {
            found.push_back(entry.path());
        }
    }
    return found;
}

class PluginSegfaultTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader_ = std::make_unique<PluginLoader>(get_plugin_path());
        ASSERT_TRUE(loader_->is_loaded())
            << "Plugin library not found. Set PLUGIN_PATH or run from build/bin.";
    }

    void TearDown() override {
        loader_.reset();
        // Clean up any crash files generated during the test.
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            const auto name = entry.path().filename().string();
            if (name.starts_with("crash_") && name.ends_with(".txt"))
                std::filesystem::remove(entry.path());
        }
    }

    std::unique_ptr<PluginLoader> loader_;
};

TEST_F(PluginSegfaultTest, InitReturnsZero) {
    auto fn = loader_->get_symbol<plugin_init_fn>("plugin_init");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn(), 0);
}

TEST_F(PluginSegfaultTest, GetNameReturnsExpected) {
    auto fn = loader_->get_symbol<plugin_get_name_fn>("plugin_get_name");
    ASSERT_NE(fn, nullptr);
    EXPECT_STREQ(fn(), "segfault_plugin");
}

#ifndef _WIN32
TEST_F(PluginSegfaultTest, CrashFileWrittenOnSegfault) {
    namespace fs = std::filesystem;

    const auto work_dir = fs::current_path();
    const auto snapshot = fs::file_time_type::clock::now();

    pid_t pid = fork();
    ASSERT_NE(pid, -1) << "fork() failed";

    if (pid == 0) {
        // Child: install handler then trigger SIGSEGV.
        auto init_fn = loader_->get_symbol<plugin_init_fn>("plugin_init");
        if (init_fn) init_fn();

        volatile int* p = nullptr;
        *p = 42; // SIGSEGV
        _exit(1);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    // Give the handler a moment to flush the file.
    usleep(100'000);

    const auto crash_files = find_new_crash_files(work_dir, snapshot);
    ASSERT_FALSE(crash_files.empty()) << "No crash_*.txt file written after SEGFAULT";

    const auto& crash_file = crash_files.front();
    std::ifstream f(crash_file);
    const std::string content((std::istreambuf_iterator<char>(f)), {});

    EXPECT_FALSE(content.empty()) << "Crash file is empty: " << crash_file;
    EXPECT_NE(content.find("SIGSEGV"), std::string::npos)
        << "Crash file missing SIGSEGV marker:\n" << content;
}
#endif

} // namespace
