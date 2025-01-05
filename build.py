#!/usr/bin/env python3

import argparse
import pathlib
import platform
import re
import shutil
import sys
import tarfile
import urllib.request

from tools.autd3_build_utils.autd3_build_utils import (
    BaseConfig,
    err,
    fetch_submodule,
    info,
    remove,
    run_command,
    substitute_in_file,
    working_dir,
)


class Config(BaseConfig):
    no_examples: bool
    cmake_extra: list[str] | None
    arch: str

    def __init__(self, args) -> None:  # noqa: ANN001
        super().__init__(args)

        self.no_examples = getattr(args, "no_examples", False)
        self.cmake_extra = args.cmake_extra.split(" ") if hasattr(args, "cmake_extra") and args.cmake_extra is not None else None


def should_update_lib(config: Config, version: str) -> bool:
    if config.is_windows():
        if not pathlib.Path("lib/autd3capi_link_soem.lib").exists():
            return True
    elif not pathlib.Path("lib/libautd3capi_link_soem.a").exists():
        return True
    if not pathlib.Path("VERSION").exists():
        return True
    with pathlib.Path("VERSION").open() as f:
        old_version = f.read().strip()
    return old_version != version


def copy_lib(config: Config) -> None:
    with pathlib.Path("include/autd3_link_soem.hpp").open() as f:
        content = f.read()
        version = re.search(r"static inline std::string version = \"(.*)\";", content).group(1).split(".")
        version = ".".join(version) if version[2].endswith("-rc") else ".".join(version[:3])

    if not should_update_lib(config, version):
        return

    if config.is_windows():
        if config.arch == "x64":
            url = f"https://github.com/shinolab/autd3-capi-link-soem/releases/download/v{version}/autd3-link-soem-v{version}-win-x64-static.zip"
        elif config.arch == "aarch64":
            url = f"https://github.com/shinolab/autd3-capi-link-soem/releases/download/v{version}/autd3-link-soem-v{version}-win-aarch64-static.zip"
        else:
            err(f"Unsupported platform: {platform.machine()}")
            sys.exit(-1)
        urllib.request.urlretrieve(url, "tmp.zip")
        shutil.unpack_archive("tmp.zip", ".")
        remove("tmp.zip")
    elif config.is_macos():
        url = f"https://github.com/shinolab/autd3-capi-link-soem/releases/download/v{version}/autd3-link-soem-v{version}-macos-aarch64-static.tar.gz"
        urllib.request.urlretrieve(url, "tmp.tar.gz")
        with tarfile.open("tmp.tar.gz", "r:gz") as tar:
            tar.extractall(filter="fully_trusted")
        remove("tmp.tar.gz")
    elif config.is_linux():
        url = f"https://github.com/shinolab/autd3-capi-link-soem/releases/download/v{version}/autd3-link-soem-v{version}-linux-x64-static.tar.gz"
        urllib.request.urlretrieve(url, "tmp.tar.gz")
        with tarfile.open("tmp.tar.gz", "r:gz") as tar:
            tar.extractall(filter="fully_trusted")
        remove("tmp.tar.gz")
    remove("bin")

    with pathlib.Path("VERSION").open("w") as f:
        f.write(version)


def cpp_build(args) -> None:  # noqa: ANN001
    config = Config(args)

    copy_lib(config)

    if not config.no_examples:
        info("Building examples...")
        with working_dir("examples"):
            pathlib.Path("build").mkdir(exist_ok=True)
            with working_dir("build"):
                command = ["cmake", "..", "-DAUTD_LOCAL_TEST=ON"]
                if config.cmake_extra is not None:
                    command.extend(config.cmake_extra)
                if config.is_windows() and config.arch == "arm64":
                    command.extend(["-A", "ARM64"])
                if config.is_windows() and hasattr(args, "vs") and args.vs is not None:
                    if args.vs == "2022":
                        command.append("-GVisual Studio 17 2022")
                    else:
                        err(f"Unsupported Visual Studio version: {args.vs}")
                        sys.exit(-1)
                run_command(command)
                command = ["cmake", "--build", "."]
                if config.release:
                    command.extend(["--config", "Release"])
                    run_command(command)


def cpp_test(args) -> None:  # noqa: ANN001
    args.no_examples = True
    cpp_build(args)

    config = Config(args)
    with working_dir("tests"):
        pathlib.Path("build").mkdir(exist_ok=True)
        with working_dir("build"):
            command = ["cmake", ".."]
            if config.cmake_extra is not None:
                command.extend(config.cmake_extra)
            run_command(command)
            command = ["cmake", "--build", ".", "--parallel", "8"]
            if config.release:
                command.extend(["--config", "Release"])
            if config.is_windows():
                command.extend(["--", "/maxcpucount:8"])
            run_command(command)

            target_dir = "."
            if config.is_windows():
                target_dir = "Release" if config.release else "Debug"
            run_command([f"{target_dir}/test_autd3{config.exe_ext()}"])


def check_if_all_native_methods_called() -> None:
    defined_methods = set()
    pattern = re.compile(".* (AUTD.*)\\(.*")
    for file in pathlib.Path("include/autd3_link_soem/native_methods").rglob("autd3capi*.h"):
        with file.open() as f:
            for line in f.readlines():
                result = pattern.match(line)
                if result:
                    defined_methods.add(result.group(1))
    defined_methods = set(filter(lambda x: not x.endswith("TracingInitWithFile"), defined_methods))
    defined_methods = set(filter(lambda x: not x.endswith("TracingInit"), defined_methods))

    used_methods = set()
    pattern = re.compile(r".*(AUTD.*?)[\(|,|\)].*")
    for file in list(pathlib.Path("include").rglob("*.hpp")) + list(pathlib.Path("tests").rglob("*.cpp")):
        with file.open() as f:
            for line in f.readlines():
                result = pattern.match(line)
                if result:
                    used_methods.add(result.group(1))

    unused_methods = defined_methods.difference(used_methods)
    if len(unused_methods) > 0:
        err("Following native methods are defined but not used.")
        for method in unused_methods:
            print(f"\t{method}")
        sys.exit(-1)


def check_all_headers_is_tested() -> None:  # noqa: C901, PLR0912, PLR0915
    with working_dir("include/autd3_link_soem"):
        headers = [str(header.relative_to(pathlib.Path.cwd())) for header in pathlib.Path.cwd().rglob("*.hpp")]
        headers = [header.replace("\\", "/") for header in headers if not header.startswith("native_methods")]
        headers = {header.replace(".hpp", ".cpp") for header in headers}

    def load_sources(base_path: pathlib.Path) -> set[pathlib.Path]:
        tested = set()
        with (base_path / "CMakeLists.txt").open() as f:
            for line in f.readlines():
                subdir = re.search(r"add_subdirectory\((.*)\)", line)
                if subdir:
                    tested |= load_sources(base_path / subdir.group(1))

        with (base_path / "CMakeLists.txt").open() as f:
            sources = False
            for line in f.readlines():
                if line.strip().startswith("target_sources(test_autd3 PRIVATE"):
                    sources = True
                if not sources:
                    continue
                if line.strip().startswith(")"):
                    sources = False
                    continue
                src = re.search(r"\s*(.*.cpp)", line)
                if src:
                    tested.add(base_path / src.group(1))
        return tested

    with working_dir("tests"):
        base_path = pathlib.Path.cwd()
        tested = [str(tested.relative_to(base_path)).replace("\\", "/") for tested in load_sources(base_path)]

        untested_headers = headers.difference(tested)
        if len(untested_headers) > 0:
            err("Following headers are not tested.")
            for header in sorted(untested_headers):
                print(f"\t{header}")
            sys.exit(-1)

        unincluded_headers = set()
        for cpp in tested:
            hpp = cpp.replace(".cpp", ".hpp")
            with (base_path / cpp).open() as f:
                found_include = False
                for line in f.readlines():
                    if re.search(rf"#include <autd3_link_soem/{hpp}>", line):
                        found_include = True
                        break
                if not found_include:
                    unincluded_headers.add(f"{cpp} does not include target header file {hpp}")
        if len(unincluded_headers) > 0:
            err("Following source files do not include target header file.")
            for header in sorted(unincluded_headers):
                print(f"\t{header}")
            sys.exit(-1)

        empty_test = set()
        for cpp in tested:
            with (base_path / cpp).open() as f:
                found_test = False
                for line in f.readlines():
                    if re.search(r"TEST\(", line):
                        found_test = True
                        break
                if not found_test:
                    empty_test.add(cpp)
        if len(empty_test) > 0:
            err("Following source files do not have any test.")
            for header in sorted(empty_test):
                print(f"\t{header}")
            sys.exit(-1)


def cpp_cov(args) -> None:  # noqa: ANN001
    check_if_all_native_methods_called()
    check_all_headers_is_tested()

    config = Config(args)
    if not config.is_linux():
        err("Coverage is only supported on Linux.")
        return

    args.no_examples = True
    cpp_build(args)

    with working_dir("tests"):
        pathlib.Path("build").mkdir(exist_ok=True)
        with working_dir("build"):
            command = ["cmake", "..", "-DCOVERAGE=ON", "-DAUTD3_USE_PCH=OFF"]
            if config.cmake_extra is not None:
                command.extend(config.cmake_extra)
            run_command(command)
            command = ["cmake", "--build", ".", "--parallel", "8"]
            if config.release:
                command.extend(["--config", "Release"])
            run_command(command)
            with working_dir("CMakeFiles/test_autd3.dir"):
                run_command(["lcov", "-c", "-i", "-d", ".", "--ignore-errors", "mismatch", "-o", "coverage.baseline"])
            target_dir = "."
            if config.is_windows():
                target_dir = "Release" if config.release else "Debug"
            run_command([f"{target_dir}/test_autd3{config.exe_ext()}"])

            with working_dir("CMakeFiles/test_autd3.dir"):
                run_command(["lcov", "-c", "-d", ".", "--ignore-errors", "mismatch", "-o", "coverage.out"])
                run_command(["lcov", "-a", "coverage.baseline", "-a", "coverage.out", "-o", "coverage.raw.info"])
                run_command(
                    ["lcov", "-r", "coverage.raw.info", "*/_deps/*", "*/usr/*", "*/tests/*", "--ignore-errors", "unused", "-o", "coverage.info"]
                )
                if args.html:
                    run_command(["genhtml", "-o", "html", "--num-spaces", "4", "coverage.info"])


def cpp_run(args) -> None:  # noqa: ANN001
    args.no_examples = False
    cpp_build(args)

    config = Config(args)
    target_dir = ("Release" if args.release else "Debug") if config.is_windows() else "."

    run_command([f"examples/build/{target_dir}/{args.target}{config.exe_ext()}"])


def cpp_clear(_) -> None:  # noqa: ANN001
    remove("lib")
    remove("bin")
    remove("examples/build")
    remove("tests/build")


def util_update_ver(args) -> None:  # noqa: ANN001
    version = args.version

    version_cmake = version.split(".")
    if version_cmake[2].endswith("-rc"):
        version_cmake[2] = version_cmake[2].replace("-rc", "")
        version_cmake = ".".join(version_cmake[:3])
    else:
        version_cmake = ".".join(version_cmake)

    substitute_in_file(
        "CMakeLists.txt",
        [
            (
                r"^project\(autd3_link_soem VERSION (.*)\)",
                f"project(autd3_link_soem VERSION {version_cmake})",
            ),
            (
                r"v.*/autd3-v.*-(win|macos|linux)",
                rf"v{version}/autd3-v{version}-\1",
            ),
        ],
        flags=re.MULTILINE,
    )
    substitute_in_file(
        "include/autd3_link_soem.hpp",
        [
            (
                r'^static inline std::string version = "(.*)";',
                f'static inline std::string version = "{version}";',
            ),
        ],
        flags=re.MULTILINE,
    )

    substitute_in_file(
        "examples/CMakeLists.txt",
        [
            (
                r"v.*/autd3-v.*-(win|macos|linux)",
                rf"v{version}/autd3-v{version}-\1",
            ),
            (
                r"v.*/autd3-link-soem-v.*-(win|macos|linux)",
                rf"v{version}/autd3-link-soem-v{version}-\1",
            ),
        ],
        flags=re.MULTILINE,
    )


def util_gen_wrapper(_) -> None:  # noqa: ANN001
    if shutil.which("cargo") is not None:
        with working_dir("tools/wrapper-generator"):
            run_command(["cargo", "run"])
    else:
        err("cargo is not installed. Skip generating wrapper.")


def command_help(args) -> None:  # noqa: ANN001
    print(parser.parse_args([args.command, "--help"]))


if __name__ == "__main__":
    with working_dir(pathlib.Path(__file__).parent):
        fetch_submodule()

        parser = argparse.ArgumentParser(description="autd3 library build script")
        subparsers = parser.add_subparsers()

        # build
        parser_cpp_build = subparsers.add_parser("build", help="see `build -h`")
        parser_cpp_build.add_argument("--release", action="store_true", help="release build")
        parser_cpp_build.add_argument("--no-examples", action="store_true", help="skip building examples")
        parser_cpp_build.add_argument("--cmake-extra", help="cmake extra args")
        parser_cpp_build.add_argument("--arch", help="architecture (x64, arm64)")
        parser_cpp_build.add_argument("--vs", help="Visual Studio version")
        parser_cpp_build.set_defaults(handler=cpp_build)

        # test
        parser_cpp_test = subparsers.add_parser("test", help="see `test -h`")
        parser_cpp_test.add_argument("--release", action="store_true", help="release build")
        parser_cpp_test.add_argument("--cmake-extra", help="cmake extra args")
        parser_cpp_test.set_defaults(handler=cpp_test)

        # cov
        parser_cpp_cov = subparsers.add_parser("cov", help="see `cov -h`")
        parser_cpp_cov.add_argument("--release", action="store_true", help="release build")
        parser_cpp_cov.add_argument("--html", action="store_true", help="generate html report", default=False)
        parser_cpp_cov.add_argument("--cmake-extra", help="cmake extra args")
        parser_cpp_cov.set_defaults(handler=cpp_cov)

        # run
        parser_cpp_run = subparsers.add_parser("run", help="see `run -h`")
        parser_cpp_run.add_argument("target", help="binary target")
        parser_cpp_run.add_argument("--release", action="store_true", help="release build")
        parser_cpp_run.set_defaults(handler=cpp_run)

        # clear
        parser_cpp_clear = subparsers.add_parser("clear", help="see `clear -h`")
        parser_cpp_clear.set_defaults(handler=cpp_clear)

        # util
        parser_util = subparsers.add_parser("util", help="see `util -h`")
        subparsers_util = parser_util.add_subparsers()

        # util update version
        parser_util_upver = subparsers_util.add_parser("upver", help="see `util upver -h`")
        parser_util_upver.add_argument("version", help="version")
        parser_util_upver.set_defaults(handler=util_update_ver)

        # util update version
        parser_util_gen_wrap = subparsers_util.add_parser("gen_wrap", help="see `util gen_wrap -h`")
        parser_util_gen_wrap.set_defaults(handler=util_gen_wrapper)

        # help
        parser_help = subparsers.add_parser("help", help="see `help -h`")
        parser_help.add_argument("command", help="command name which help is shown")
        parser_help.set_defaults(handler=command_help)

        args = parser.parse_args()
        if hasattr(args, "handler"):
            args.handler(args)
        else:
            parser.print_help()
