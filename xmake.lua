add_rules("mode.debug", "mode.release")

add_requires("conan::gflags/2.2.2", {alias = "gflags"})
add_requires("conan::flatbuffers/22.12.06", {alias = "flatbuffers"})
add_requires("conan::boost/1.81.0", {alias = "boost"})
add_requires("conan::openssl/1.1.1s", {alias = "openssl"})

set_languages("c++20")
package("mysql")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), "library/mysql"))
    set_policy("package.install_always", true)
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_OSX_ARCHITECTURES=" .. ("arm64"))
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

add_requires("mysql")

target("data-accessor")
    set_kind("binary")
    add_files("**.cpp")
    add_includedirs("library/mysql/include")
    add_packages("gflags", "boost", "flatbuffers", "openssl", "mysql")
