from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, cmake_layout


class Manufacture(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.user_presets_path = "conan-presets.json"
        toolchain.generate()

    def layout(self):
        cmake_layout(self)
