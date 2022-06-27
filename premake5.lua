newoption {
  trigger = "test",
  description = "Compile tests.",
}

workspace "XBDM"
  startproject "XBDM"
  location (path.join(".", "build"))
  targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/obj"

  architecture "x86_64"

  configurations {
    "Debug",
    "Release",
  }

  filter "system:not macosx"
    systemversion "latest"

  filter "configurations:Debug"
    defines "DEBUG"
    runtime "Debug"
    symbols "on"

  filter "configurations:Release"
    defines "RELEASE"
    runtime "Release"
    optimize "on"

project "XBDM"
  kind "StaticLib"
  language "C++"
  cppdialect "C++17"

  local includedir = path.join(".", "include")
  local srcdir = path.join(".", "src")

  pchheader "pch.h"
  pchsource (path.join(srcdir, "pch.cpp"))

  files {
    path.join(includedir, "**.h"),
    path.join(srcdir, "**.h"),
    path.join(srcdir, "**.cpp"),
  }

  includedirs {
    includedir,
    srcdir,
  }

  filter "system:windows"
    links "Ws2_32.lib"

if _OPTIONS["test"] then
  project "Tests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    local xbdmincludedir = path.join(".", "include")
    local testdir = path.join(".", "test")

    files {
      path.join(testdir, "**.h"),
      path.join(testdir, "**.cpp"),
    }

    includedirs {
      testdir,
      xbdmincludedir,
    }
end
