#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

using plugin_init_fn = int (*)();
using plugin_get_name_fn = const char *(*)();

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
    const char* env = std::getenv("PLUGIN_SEGFAULT_PATH");
    if (env) return env;

    auto exe_dir = std::filesystem::path(".");
#ifdef _WIN32
    return (exe_dir / "plugin_segfault.dll").string();
#elif defined(__APPLE__)
    return (exe_dir / "libplugin_segfault.dylib").string();
#else
    return (exe_dir / "libplugin_segfault.so").string();
#endif
}

class PluginSegfaultTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader_ = std::make_unique<PluginLoader>(get_plugin_path());
        ASSERT_TRUE(loader_->is_loaded())
            << "Plugin library not found. Set PLUGIN_SEGFAULT_PATH or run from build/bin.";
    }

    void TearDown() override { loader_.reset(); }

    std::unique_ptr<PluginLoader> loader_;
};

TEST_F(PluginSegfaultTest, InitReturnsZero) {
    auto fn = loader_->get_symbol<plugin_init_fn>("plugin_segfault_init");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn(), 0);
}

TEST_F(PluginSegfaultTest, GetNameReturnsExpected) {
    auto fn = loader_->get_symbol<plugin_get_name_fn>("plugin_segfault_get_name");
    ASSERT_NE(fn, nullptr);
    EXPECT_STREQ(fn(), "segfault_plugin");
}

#ifndef _WIN32
TEST_F(PluginSegfaultTest, CrashFileWrittenOnSegfault) {
    namespace fs = std::filesystem;

    // Use a temp directory so we don't pollute cwd
    auto work_dir = fs::temp_directory_path() / "segfault_test";
    fs::create_directories(work_dir);

    // EXPECT_DEATH automatically handles fork(), waitpid(), and verifying the SIGSEGV
    EXPECT_DEATH({
        // Change the directory ONLY in the child process
        fs::current_path(work_dir);

        // Since fork() clones memory on POSIX, we can safely use the existing loader_
        auto init_fn = loader_->get_symbol<plugin_init_fn>("plugin_segfault_init");
        if (init_fn) init_fn();

        // Trigger SIGSEGV
        volatile int* p = nullptr;
        *p = 42;
    }, ""); // We expect it to die, output doesn't matter

    // Wait for the Crash Handler to finish writing the file
    bool found_crash_file = false;
    std::string crash_content;

    for (int i = 0; i < 50; ++i) { // Poll for up to 5 seconds
        if (fs::exists(work_dir)) {
            for (const auto& entry : fs::directory_iterator(work_dir)) {
                const auto name = entry.path().filename().string();
                if (name.starts_with("crash_") && name.ends_with(".txt")) {
                    std::ifstream f(entry.path());
                    if (f.is_open()) {
                        crash_content.assign(std::istreambuf_iterator<char>(f), {});
                        if (!crash_content.empty()) {
                            found_crash_file = true;
                            break;
                        }
                    }
                }
            }
        }
        if (found_crash_file) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_TRUE(found_crash_file) << "No crash_*.txt file written in " << work_dir;

    if (found_crash_file) {
        // Verify it contains signal info and stacktrace content
        EXPECT_NE(crash_content.find("Signal"), std::string::npos)
            << "Crash file missing 'Signal' marker:\n" << crash_content;
        EXPECT_NE(crash_content.find("Stacktrace"), std::string::npos)
            << "Crash file missing 'Stacktrace' marker:\n" << crash_content;
    }

    // Safe cleanup
    std::error_code ec;
    fs::remove_all(work_dir, ec);
    if (ec) {
        std::cerr << "[ Warning  ] Could not remove work_dir: " << ec.message() << "\n";
    }
}
#endif

#ifdef _WIN32
TEST_F(PluginSegfaultTest, CrashFileWrittenOnSegfault) {
    namespace fs = std::filesystem;

    auto work_dir = fs::temp_directory_path() / "segfault_test";
    fs::create_directories(work_dir);

    EXPECT_DEATH({
      fs::current_path(work_dir);

        // Load plugin fresh in the child process (parent's function pointer is invalid here)
        PluginLoader child_loader(get_plugin_path());
        auto child_init_fn = child_loader.get_symbol<plugin_init_fn>("plugin_segfault_init");
        if (child_init_fn) child_init_fn();

        volatile int* p = nullptr;
        *p = 42;
    }, "");        // any death output

    // Wait for the OS/Crash Handler to finish writing the file
    bool found = false;
    std::string content;
    
    for (int i = 0; i < 50; ++i) { // Poll for up to 5 seconds
        if (fs::exists(work_dir)) {
            for (const auto& entry : fs::directory_iterator(work_dir)) {
                const auto name = entry.path().filename().string();
                if (name.starts_with("crash_") && name.ends_with(".txt")) {
                    std::ifstream f(entry.path());
                    if (f.is_open()) {
                        content.assign(std::istreambuf_iterator<char>(f), {});
                        if (!content.empty()) {
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        if (found) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_TRUE(found) << "Crash file was never written to disk!";
    
    if (found) {
        EXPECT_NE(content.find("Signal"), std::string::npos);
        EXPECT_NE(content.find("Stacktrace"), std::string::npos);
    }

    // Safe cleanup that will NOT throw an exception if the OS still holds a lock
    std::error_code ec;
    fs::remove_all(work_dir, ec);
    if (ec) {
        std::cerr << "[ Warning  ] Could not remove work_dir: " << ec.message() << "\n";
    }
}
#endif

} // namespace

// Custom main so we can handle --crash-child mode on Windows (the child
// process spawned by CrashFileWrittenOnSegfault to trigger the crash handler).
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  testing::GTEST_FLAG(catch_exceptions) = false;
  return RUN_ALL_TESTS();
}
