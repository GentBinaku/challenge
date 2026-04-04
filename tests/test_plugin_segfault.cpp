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

    pid_t pid = fork();
    ASSERT_NE(pid, -1) << "fork() failed";

    if (pid == 0) {
        // Child: chdir to work_dir, init plugin (installs handler), then crash
        if (chdir(work_dir.c_str()) != 0) _exit(99);

        auto init_fn = loader_->get_symbol<plugin_init_fn>("plugin_segfault_init");
        if (init_fn) init_fn();

        // Trigger SIGSEGV
        volatile int* p = nullptr;
        *p = 42;
        _exit(1);
    }

    // Parent: wait for child
    int status = 0;
    waitpid(pid, &status, 0);

    // Child should have been killed by SIGSEGV
    EXPECT_TRUE(WIFSIGNALED(status));
    EXPECT_EQ(WTERMSIG(status), SIGSEGV);

    // Check crash file was written
    bool found_crash_file = false;
    std::string crash_content;

    for (const auto& entry : fs::directory_iterator(work_dir)) {
        const auto name = entry.path().filename().string();
        if (name.starts_with("crash_") && name.ends_with(".txt")) {
            found_crash_file = true;
            std::ifstream f(entry.path());
            crash_content.assign(std::istreambuf_iterator<char>(f), {});
            break;
        }
    }

    EXPECT_TRUE(found_crash_file) << "No crash_*.txt file written in " << work_dir;

    if (found_crash_file) {
        EXPECT_FALSE(crash_content.empty()) << "Crash file is empty";
        // Verify it contains signal info and stacktrace content
        EXPECT_NE(crash_content.find("Signal"), std::string::npos)
            << "Crash file missing 'Signal' marker:\n" << crash_content;
        EXPECT_NE(crash_content.find("Stacktrace"), std::string::npos)
            << "Crash file missing 'Stacktrace' marker:\n" << crash_content;
    }

    // Cleanup
    fs::remove_all(work_dir);
}
#endif

#ifdef _WIN32
TEST_F(PluginSegfaultTest, CrashFileWrittenOnSegfault) {
    namespace fs = std::filesystem;

    auto work_dir = fs::temp_directory_path() / "segfault_test";
    fs::create_directories(work_dir);

    // Get path to this executable so we can re-invoke it as the crash child.
    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);

    std::string plugin_path = get_plugin_path();

    // Build a writable command-line buffer (CreateProcessA may modify it).
    std::string cmd_str =
        std::string("\"") + exe_path + "\" --crash-child \"" + plugin_path + "\"";
    std::vector<char> cmd_buf(cmd_str.begin(), cmd_str.end());
    cmd_buf.push_back('\0');

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessA(
        nullptr,
        cmd_buf.data(),
        nullptr, nullptr,
        FALSE, 0,
        nullptr,
        work_dir.string().c_str(), // child's working directory → crash file written here
        &si, &pi);
    ASSERT_TRUE(ok) << "CreateProcessA failed: error=" << GetLastError();

    WaitForSingleObject(pi.hProcess, 10000 /*ms*/);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // The unhandled-exception filter returns EXCEPTION_CONTINUE_SEARCH, so the
    // OS terminates the process with the exception code as the exit code.
    EXPECT_EQ(exit_code, static_cast<DWORD>(STATUS_ACCESS_VIOLATION))
        << "Expected STATUS_ACCESS_VIOLATION (0xC0000005), got 0x"
        << std::hex << exit_code;

    // Check crash file was written
    bool found_crash_file = false;
    std::string crash_content;

    for (const auto& entry : fs::directory_iterator(work_dir)) {
        const auto name = entry.path().filename().string();
        if (name.starts_with("crash_") && name.ends_with(".txt")) {
            found_crash_file = true;
            std::ifstream f(entry.path());
            crash_content.assign(std::istreambuf_iterator<char>(f), {});
            break;
        }
    }

    EXPECT_TRUE(found_crash_file) << "No crash_*.txt file written in " << work_dir;

    if (found_crash_file) {
        EXPECT_FALSE(crash_content.empty()) << "Crash file is empty";
        EXPECT_NE(crash_content.find("Signal"), std::string::npos)
            << "Crash file missing 'Signal' marker:\n" << crash_content;
        EXPECT_NE(crash_content.find("Stacktrace"), std::string::npos)
            << "Crash file missing 'Stacktrace' marker:\n" << crash_content;
    }

    // Cleanup
    fs::remove_all(work_dir);
}
#endif

} // namespace

// Custom main so we can handle --crash-child mode on Windows (the child
// process spawned by CrashFileWrittenOnSegfault to trigger the crash handler).
int main(int argc, char** argv) {
#ifdef _WIN32
    if (argc >= 3 && std::string(argv[1]) == "--crash-child") {
        // argv[2] = path to the plugin DLL.
        // Working directory is already set by CreateProcessA to the temp dir.
        HMODULE h = LoadLibraryA(argv[2]);
        if (h) {
            auto init_fn =
                reinterpret_cast<int (*)()>(GetProcAddress(h, "plugin_segfault_init"));
            if (init_fn) init_fn(); // installs SetUnhandledExceptionFilter
        }
        // Trigger access violation — the handler writes the crash file.
        volatile int* p = nullptr;
        *p = 42;
        return 1; // unreachable
    }
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
