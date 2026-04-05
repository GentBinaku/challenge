import os

from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy

from conan import ConanFile


class ChallengeProjectConan(ConanFile):
    name = "challenge_project"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"
    exports_sources = (
        "CMakeLists.txt",
        "CMakePresets.json",
        "app/*",
        "plugin/*",
        "plugin_segfault/*",
        "tests/*",
    )

    def configure(self):
        self.options["boost"].without_cobalt = True
        self.options["boost"].shared = True
        self.options["gtest"].shared = True

    def requirements(self):
        self.requires("boost/1.90.0")
        self.requires("gtest/1.17.0")

    def layout(self):
        cmake_layout(self)
        build_type = str(self.settings.build_type)
        self.folders.source = "."
        self.folders.build = os.path.join("build", build_type)
        self.folders.generators = os.path.join("build", build_type)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False
        tc.variables["BUILD_TESTING"] = True
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

        lib_dir = os.path.join(self.build_folder, "lib")
        bin_dir = os.path.join(self.build_folder, "bin")
        for dep in self.dependencies.values():
            if self.settings.os == "Windows":
                for libdir in dep.cpp_info.libdirs:
                    copy(self, "*.lib", libdir, lib_dir)
                for bindir in dep.cpp_info.bindirs:
                    copy(self, "*.dll", bindir, bin_dir)
            else:
                for libdir in dep.cpp_info.libdirs:
                    copy(self, "*.so*", libdir, lib_dir)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        # Headers
        copy(self, "*.h", os.path.join(self.source_folder, "plugin", "include"),
             os.path.join(self.package_folder, "include"))
        copy(self, "*.h", os.path.join(self.source_folder, "plugin_segfault", "include"),
             os.path.join(self.package_folder, "include"))

        # Shared libraries
        if self.settings.os == "Windows":
            copy(self, "*.dll", os.path.join(self.build_folder, "bin"),
                 os.path.join(self.package_folder, "bin"))
            copy(self, "*.lib", os.path.join(self.build_folder, "lib"),
                 os.path.join(self.package_folder, "lib"))
        else:
            copy(self, "*.so*", os.path.join(self.build_folder, "lib"),
                 os.path.join(self.package_folder, "lib"))
            copy(self, "*.so*", os.path.join(self.build_folder, "bin"),
                 os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.components["plugin"].libs = ["plugin"]
        self.cpp_info.components["plugin"].includedirs = ["include"]
        if self.settings.os == "Windows":
            self.cpp_info.components["plugin"].bindirs = ["bin"]

        self.cpp_info.components["plugin_segfault"].libs = ["plugin_segfault"]
        self.cpp_info.components["plugin_segfault"].includedirs = ["include"]
        if self.settings.os == "Windows":
            self.cpp_info.components["plugin_segfault"].bindirs = ["bin"]
