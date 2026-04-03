import os

from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy

from conan import ConanFile


class ChallengeProjectConan(ConanFile):
    name = "challenge_project"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"

    def configure(self):
        self.options["boost"].without_cobalt = True
        self.options["boost"].shared = True
        self.options["gtest"].shared = True

    def requirements(self):
        self.requires("boost/1.90.0")
        self.requires("gtest/1.17.0")

    def layout(self):
        cmake_layout(self)
        self.folders.generators = self.folders.build

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
