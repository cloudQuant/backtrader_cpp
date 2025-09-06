"""
Backtrader C++ Python Bindings Setup Script
High-performance C++ backend for Python Backtrader
"""

import os
import sys
import subprocess
import platform
from pathlib import Path

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext as pybind11_build_ext


class CMakeExtension(Extension):
    """A CMake extension for setuptools"""
    
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    """Custom build command that uses CMake"""
    
    def build_extension(self, ext):
        if isinstance(ext, CMakeExtension):
            self.build_cmake(ext)
        else:
            super().build_extension(ext)
    
    def build_cmake(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        
        # Required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        debug = int(os.environ.get("DEBUG", 0)) if self.debug is None else self.debug
        cfg = "Debug" if debug else "Release"

        # CMake lets you override the generator - we check this
        cmake_generator = os.environ.get("CMAKE_GENERATOR", "")

        # Set Python_EXECUTABLE instead if you use PYBIND11_FINDPYTHON
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={cfg}",
            f"-DBUILD_PYTHON_BINDINGS=ON",
        ]
        
        build_args = []
        
        # Adding CMake arguments set as environment variable
        if "CMAKE_ARGS" in os.environ:
            cmake_args += [item for item in os.environ["CMAKE_ARGS"].split(" ") if item]

        if self.compiler.compiler_type != "msvc":
            # Using Ninja-build since it a) is available as a wheel and b) multithreads automatically
            if not cmake_generator or cmake_generator == "Ninja":
                try:
                    import ninja  # noqa: F401
                    cmake_args += ["-GNinja"]
                except ImportError:
                    pass

        else:
            # Single config generators are handled "normally"
            single_config = any(x in cmake_generator for x in {"NMake", "Ninja"})

            # CMake allows an arch-in-generator style for backward compatibility
            contains_arch = any(x in cmake_generator for x in {"ARM", "Win64"})

            # Specify the arch if using MSVC generator, but only if it doesn't
            # contain a backward-compatibility arch spec already in the
            # generator name.
            if not single_config and not contains_arch:
                cmake_args += ["-A", "x64" if platform.machine().endswith("64") else "Win32"]

            # Multi-config generators have a different way to specify configs
            if not single_config:
                cmake_args += [f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}"]
                build_args += ["--config", cfg]

        if sys.platform.startswith("darwin"):
            # Cross-compile support for macOS - respect ARCHFLAGS if set
            archs = re.findall(r"-arch (\S+)", os.environ.get("ARCHFLAGS", ""))
            if archs:
                cmake_args += ["-DCMAKE_OSX_ARCHITECTURES={}".format(";".join(archs))]

        # Set CMAKE_BUILD_PARALLEL_LEVEL to control the parallel build level
        # across all generators.
        if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
            # self.parallel is a Python 3 only way to set parallel jobs by hand
            # using -j in the build_ext call, not supported by pip or PyPA-build.
            if hasattr(self, "parallel") and self.parallel:
                # CMake 3.12+ only.
                build_args += [f"-j{self.parallel}"]

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)

        subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)
        subprocess.check_call(["cmake", "--build", "."] + build_args, cwd=build_temp)


# Read README for long description
def read_file(filename):
    with open(Path(__file__).parent / filename, encoding="utf-8") as f:
        return f.read()


# Version information
version_info = {}
with open(Path(__file__).parent / "src" / "_version.py") as f:
    exec(f.read(), version_info)


# Extension modules
ext_modules = [
    CMakeExtension("backtrader_cpp"),
]

setup(
    name="backtrader-cpp",
    version=version_info["__version__"],
    author="Backtrader C++ Team",
    author_email="team@backtrader-cpp.org",
    description="High-performance C++ backend for Backtrader quantitative trading framework",
    long_description=read_file("README.md"),
    long_description_content_type="text/markdown",
    url="https://github.com/yunzed/backtrader_cpp",
    project_urls={
        "Bug Tracker": "https://github.com/yunzed/backtrader_cpp/issues",
        "Documentation": "https://backtrader-cpp.readthedocs.io/",
        "Source Code": "https://github.com/yunzed/backtrader_cpp",
    },
    ext_modules=ext_modules,
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    python_requires=">=3.8",
    install_requires=[
        "numpy>=1.20.0",
        "pandas>=1.3.0",
        "matplotlib>=3.3.0",
    ],
    extras_require={
        "dev": [
            "pytest>=6.0",
            "pytest-benchmark>=3.4.0",
            "black>=21.0",
            "flake8>=3.8",
            "mypy>=0.800",
            "sphinx>=4.0",
            "sphinx-rtd-theme>=0.5",
        ],
        "test": [
            "pytest>=6.0",
            "pytest-benchmark>=3.4.0",
            "pytest-cov>=2.10",
        ],
        "docs": [
            "sphinx>=4.0",
            "sphinx-rtd-theme>=0.5",
            "myst-parser>=0.15",
        ],
    },
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Financial and Insurance Industry",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: C++",
        "Topic :: Office/Business :: Financial :: Investment",
        "Topic :: Scientific/Engineering :: Mathematics",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    keywords="trading, backtesting, finance, algorithmic trading, quantitative finance, technical analysis",
    include_package_data=True,
    package_data={
        "": ["*.md", "*.txt", "*.yml", "*.yaml"],
    },
)