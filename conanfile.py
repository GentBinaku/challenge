from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout

from conan import ConanFile


class ChallengeProjectConan(ConanFile):
    name = "challenge_project"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"

    # Keep third-party dependencies shared as required by the challenge.
    default_options = {
        "boost/*:shared": True,
        "gtest/*:shared": True,
    }

    def configure(self):
        self.options["boost"].without_cobalt = True

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

    def validate(self):
        cppstd = self.settings.get_safe("compiler.cppstd")
        if cppstd:
            check_min_cppstd(self, 20)
