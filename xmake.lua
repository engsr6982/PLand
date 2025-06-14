add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
add_repositories("engsr6982-repo https://github.com/engsr6982/xmake-repo.git")
add_repositories("miracleforest-repo https://github.com/MiracleForest/xmake-repo.git")
add_repositories("OTOTYAN https://github.com/OEOTYAN/xmake-repo.git")


-- LeviMc(LiteLDev)
add_requires("levilamina 1.3.1", {configs = {target_type = "server"}})
add_requires("levibuildscript 0.4.0")
add_requires("legacymoney 0.9.0-rc.1")

-- OTOTYAN
add_requires("bsci 0.1.6")

-- MiracleForest
add_requires("ilistenattentively 0.6.0")

-- xmake
add_requires("exprtk 0.0.3")

if has_config("devtool") then
    add_requires("imgui v1.91.6-docking", {configs = { opengl3 = true, glfw = true }})
    add_requires("glew 2.2.0")
end

if not has_config("vs_runtime") then
    set_runtimes("MD")
end


option("test")
    set_default(false)
    set_showmenu(true)
option_end()

option("devtool") -- 开发工具
    set_default(true)
    set_showmenu(true)
option_end()

rule("gen_version")
    before_build(function(target)
        import("scripts.gen_version")()
    end)


target("PLand") -- Change this to your mod name.
    add_rules("gen_version")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_rules("plugin.compile_commands.autoupdate")
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_defines("NOMINMAX", "UNICODE", "LDAPI_EXPORT", "_HAS_CXX23=1")
    add_files("src/**.cpp", "src/**.cc")
    add_includedirs("src", "include")
    add_packages(
        "levilamina",
        "exprtk",
        "ilistenattentively",
        "legacymoney",
        "bsci"
    )

    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")

    add_defines("PLUGIN_NAME=\"[PLand] \"")

    if is_mode("debug") then
        add_defines("DEBUG")
        -- add_defines("LL_I18N_COLLECT_STRINGS")
    end 

    if has_config("test") then
        add_defines("LD_TEST")
        add_files("test/**.cc")
        add_includedirs("test")
    end

    if has_config("devtool") then
        add_packages(
            "imgui",
            "glew"
        )
        add_includedirs("devtool", "devtool/deps")
        add_files("devtool/**.cc", "devtool/**.cpp")
        add_defines("LD_DEVTOOL")
    end

    if is_plat("windows") then
        add_files("resource/Resource.rc")
    end

    after_build(function (target)
        local bindir = path.join(os.projectdir(), "bin")
        local outputdir = path.join(bindir, target:name())

        local assetsdir = path.join(os.projectdir(), "assets")
        local langDir = path.join(assetsdir, "lang")
        os.mkdir(path.join(outputdir, "lang"))
        os.cp(langDir, outputdir)
    end)
