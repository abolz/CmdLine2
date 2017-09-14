local build_dir = "build/" .. _ACTION

newoption {
    trigger = "cxxflags",
    description = "Additional build options",
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

    configuration { "gmake" }
        buildoptions {
            "-march=native",
            "-Wformat",
            -- "-Wsign-compare",
            -- "-Wsign-conversion",
            -- "-pedantic",
            -- "-fno-omit-frame-pointer",
            -- "-ftime-report",
        }

    configuration { "gmake", "debug", "linux" }
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
        configuration { "gmake" }
            buildoptions {
                "-std=c++14",
            }
    end

--------------------------------------------------------------------------------
group "Libs"

project "cmdline"
    language "C++"
    kind "StaticLib"
    files {
        "src/**.h",
        "src/**.cc",
    }
    configuration { "gmake" }
        buildoptions {
            "-Wsign-compare",
            "-Wsign-conversion",
            "-Wold-style-cast",
            "-Wshadow",
            "-Wconversion",
            "-pedantic",
        }

--------------------------------------------------------------------------------
group "Tests"

project "Test"
    language "C++"
    kind "ConsoleApp"
    includedirs {
        "src/",
    }
    files {
        "test/Test.cc",
    }
    links {
        "cmdline",
    }

project "Example"
    language "C++"
    kind "ConsoleApp"
    includedirs {
        "src/",
    }
    files {
        "test/Example.cc",
    }
    links {
        "cmdline",
    }
