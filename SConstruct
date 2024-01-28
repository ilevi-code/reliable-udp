from glob import glob
from pathlib import Path

AddOption(
    "--release",
    dest="build_mode",
    action="store_const",
    const="release",
    default="debug",
)


def get_build_mode() -> str:
    return GetOption("build_mode")


def get_build_path(filename) -> str:
    parts = list(Path(str(filename)).parts)
    parts[:1] = ["build", get_build_mode()]
    return Path(*parts)


def to_dep_file(filename: str) -> str:
    return str(get_build_path(filename).with_suffix(".d"))


def to_obj_file(filename: str) -> str:
    return str(get_build_path(filename).with_suffix(env["OBJSUFFIX"]))


env = Environment(
    CXX="clang++", CXXFLAGS='-std=c++20', CPPFLAGS="-Isrc", LDLIBS="-levent", to_dep_file=to_dep_file
)

env.Tool('compilation_db')
compilation_db = env.CompilationDatabase()
AlwaysBuild(compilation_db)

if GetOption("build_mode") == "debug":
    env["CXXFLAGS"] += " -g"
env["CCFLAGS"] += "-MMD -MF ${to_dep_file(SOURCE)}"


def collect_objects(dir):
    objects = []
    for file in glob(dir + "/*.cpp"):
        dep_path = to_dep_file(file)
        obj_path = to_obj_file(file)
        obj = env.Object(obj_path, file)
        env.SideEffect(dep_path, obj)
        env.ParseDepends(dep_path)
        objects.append(obj_path)
    return objects

demo_objects = collect_objects("src")
env.Program("build/demo", demo_objects, LIBS=['event'])

test_objects = collect_objects("src/test")
test_objects.extend(filter(lambda file: "main.o" not in file, demo_objects))
env.Program("build/test", test_objects, LIBS=['event', 'gtest'])
