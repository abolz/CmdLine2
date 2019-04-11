local build_dir = "build/" .. _ACTION

newoption {
    trigger = "cxxflags",
    description = "Additional build options",
}

newoption {
    trigger = "linkflags",
    description = "Additional linker options",
}

--------------------------------------------------------------------------------
solution "Cmdline"
    configurations { "release", "debug" }
    architecture "x64"

    location    (build_dir)
    objdir      (build_dir .. "/obj")

    warnings "Extra"

    -- exceptionhandling "Off"
    -- rtti "Off"

    flags {
        "StaticRuntime",
    }

    configuration { "debug" }
        targetdir (build_dir .. "/bin/debug")

    configuration { "release" }
        targetdir (build_dir .. "/bin/release")

    configuration { "debug" }
        defines { "_DEBUG" }
        symbols "On"

    configuration { "release" }
        defines { "NDEBUG" }
        symbols "Off"
        optimize "Full"
            -- On ==> -O2
            -- Full ==> -O3

    configuration { "gmake*" }
        buildoptions {
            "-march=native",
            "-pedantic",
            "-Wdouble-promotion",
            "-Wformat",
            "-Wshadow",
            "-Wsign-compare",
            "-Wsign-conversion",
            -- "-fno-omit-frame-pointer",
            -- "-ftime-report",
            "-fstrict-overflow",
            "-Wstrict-overflow",
        }

    configuration { "gmake*", "debug", "linux" }
        buildoptions {
            -- "-fno-omit-frame-pointer",
            -- "-fsanitize=undefined",
            -- "-fsanitize=address",
            -- "-fsanitize=memory",
            -- "-fsanitize-memory-track-origins",
        }
        linkoptions {
            -- "-fsanitize=undefined",
            -- "-fsanitize=address",
            -- "-fsanitize=memory",
        }

    configuration { "vs*" }
        buildoptions {
            -- "/std:c++17",
            -- "/permissive-",
            -- "/arch:AVX2",
            -- "/GR-",
        }
        defines {
            -- "_CRT_SECURE_NO_WARNINGS=1",
            -- "_SCL_SECURE_NO_WARNINGS=1",
            -- "_HAS_EXCEPTIONS=0",
        }

    configuration { "windows" }
        characterset "MBCS"

    if _OPTIONS["cxxflags"] then
        configuration {}
            buildoptions {
                _OPTIONS["cxxflags"],
            }
    else
        configuration { "gmake*" }
            buildoptions {
                "-std=c++14",
            }
    end

    if _OPTIONS["linkflags"] then
        configuration {}
            linkoptions {
                _OPTIONS["linkflags"],
            }
    end

--------------------------------------------------------------------------------
group "Tests"

project "Test"
    language "C++"
    kind "ConsoleApp"
    includedirs {
        "src/",
    }
    files {
        "src/**.*",
        "test/doctest.cc",
        "test/doctest.h",
        "test/Test.cc",
    }

project "Example"
    language "C++"
    kind "ConsoleApp"
    includedirs {
        "src/",
    }
    files {
        "src/**.*",
        "test/Example.cc",
    }
