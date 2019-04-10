from conans import ConanFile, CMake

class gscConanFile(ConanFile):
  requires = "boost/1.69.0@conan/stable"
  settings = "os", "compiler", "build_type", "arch"
  generators = "virtualenv"
  build_requires = "cmake_installer/3.13.0@conan/stable"

  def build(self):
    cmake = CMake(self)
    cmake.configure()
    cmake.build()

