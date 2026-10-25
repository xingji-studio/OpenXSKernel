#!/usr/bin/env python3
"""OpenXSKernel 独立构建：生成只产出内核 out/kernel.krl 的 build.ninja。"""
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def find(base, suffixes):
    root = ROOT / base
    if not root.exists():
        return []
    return sorted(p for p in root.rglob("*") if p.is_file() and p.suffix in suffixes)


def esc(p):
    return str(p).replace("$", "$$").replace(":", "$:").replace(" ", "$ ")


def main():
    prefix = os.environ.get("COMPILER_PREFIX", "")
    cc = os.environ.get("CC", prefix + "clang")
    cxx = os.environ.get("CPP", prefix + "clang++")
    nasm = os.environ.get("NASM", prefix + "nasm")
    ld = os.environ.get("LD", prefix + "ld")
    objc = os.environ.get("OBJC", prefix + "objcopy")
    opt = "-O" + os.environ.get("OPT_LEVEL", "2")
    color = os.environ.get("DIAGNOSTICS_COLOR", "-fdiagnostics-color=always")
    builtin_xhci = os.environ.get("BUILTIN_XHCI", "0") == "1"

    dirs = ["kernel", "driver", "lib"]
    nasm_files = [p for d in dirs for p in find(d, (".asm",))]
    asm_files = [p for d in dirs for p in find(d, (".S",))]
    c_files = [p for d in dirs for p in find(d, (".c",))]
    cpp_files = [p for d in dirs for p in find(d, (".cpp",))]
    cpp_files = [p for p in cpp_files if p.name != "stress.cpp"]
    if builtin_xhci:
        cpp_files += find("kmod/xhci", (".cpp",))

    asm_flags = (f"{color} {opt} -g -mno-red-zone -mstackrealign -nostdlib -ffreestanding "
                 "-fno-builtin -m64 -fno-stack-protector -fno-exceptions -fno-strict-aliasing "
                 "-mno-80387 -MMD -MP")
    c_flags = (f"{color} {opt} -g -mno-red-zone -mstackrealign -nostdlib -ffreestanding "
               "-fno-builtin -m64 -fno-stack-protector -fno-exceptions -fno-strict-aliasing "
               "-std=c11 -fshort-wchar -nostdinc -mno-80387 -I./include -MMD -MP")
    cpp_flags = (f"{color} {opt} -g -mno-red-zone -mstackrealign -nostdlib -ffreestanding "
                 "-fno-builtin -m64 -fno-stack-protector -fno-exceptions -fno-strict-aliasing "
                 "-fno-rtti -std=gnu++17 -fshort-wchar -nostdinc -fno-use-cxa-atexit "
                 "-fno-threadsafe-statics -mno-80387 -Wno-int-to-pointer-cast -Wno-macro-redefined "
                 "-Wno-c11-extensions -Wno-c99-extensions -Wno-gnu-statement-expression-from-macro-expansion "
                 "-I./include -Wno-writable-strings -Wno-c++11-narrowing -MMD -MP")
    if builtin_xhci:
        cpp_flags += " -DXHCI_BUILTIN"

    L = ["# OpenXSKernel 专用：只构建内核", "ninja_required_version = 1.10"]
    L += [f"cc = {cc}", f"cxx = {cxx}", f"nasm = {nasm}", f"ld = {ld}", f"objc = {objc}"]
    L += [f"asm_flags = {asm_flags}", f"c_flags = {c_flags}", f"cpp_flags = {cpp_flags}",
          "nasm_flags = -f elf64 -DX86_64_TARGET -DUEFI", ""]
    L += [
        "rule nasm",
        "  command = mkdir -p $$(dirname $out) && $nasm $nasm_flags $in -o $out",
        "",
        "rule root_as",
        "  command = mkdir -p $$(dirname $out) && $cc -mcmodel=large $asm_flags -MF $out.d -c $in -o $out",
        "  depfile = $out.d",
        "  deps = gcc",
        "",
        "rule root_cc",
        "  command = mkdir -p $$(dirname $out) && $cc -fno-pic -fno-pie -mcmodel=large $c_flags -MF $out.d -c $in -o $out",
        "  depfile = $out.d",
        "  deps = gcc",
        "",
        "rule root_cxx",
        "  command = mkdir -p $$(dirname $out) && $cxx -fno-pic -fno-pie -mcmodel=large $cpp_flags -MF $out.d -c $in -o $out",
        "  depfile = $out.d",
        "  deps = gcc",
        "",
        "rule objcopy_bin",
        "  command = mkdir -p $$(dirname $out) && $objc -I binary -O elf64-x86-64 $bin_input $out",
        "",
        "rule kernel_link",
        "  command = $ld -z muldefs -T linker.ld --static --wrap=malloc --wrap=calloc --wrap=free --wrap=realloc --wrap=aligned_alloc -o $out $in",
        "",
    ]
    objs = []
    for rule, files in (("nasm", nasm_files), ("root_as", asm_files),
                        ("root_cc", c_files), ("root_cxx", cpp_files)):
        for src in files:
            rel_src = src.relative_to(ROOT)
            obj = Path("out") / rel_src.with_suffix(".o")
            L += [f"build {esc(obj)}: {rule} {esc(rel_src)} | kernel/build_settings.h"]
            objs.append(obj)
        L += [""]

    L += ["build out/font/hankaku.o: objcopy_bin font/hankaku.bin",
          "  bin_input = ./font/hankaku.bin", ""]
    inputs = " ".join(esc(o) for o in objs) + " out/font/hankaku.o liballoc-x86_64.a"
    L += [f"build out/kernel.krl: kernel_link {inputs} | linker.ld", ""]

    # ---- xbps 打包（pack）----
    pkg = {"name": "OpenXSKernel", "ver": "2.1.0_1", "arch": "x86_64",
           "desc": "Leopard kernel", "install": "system/kernel.krl"}
    pkg_args = f"-A {pkg['arch']} -n {pkg['name']}-{pkg['ver']} -s '{pkg['desc']}'"
    xbps_out = f"out/{pkg['name']}-{pkg['ver']}.{pkg['arch']}.xbps"
    L += [
        "rule package",
        f"  command = rm -rf .pkg && mkdir -p .pkg/$$(dirname {pkg['install']}) && "
        f"cp out/kernel.krl .pkg/{pkg['install']} && cd out && xbps-create {pkg_args} ../.pkg",
        "",
        f"build {xbps_out}: package out/kernel.krl",
        "build build: phony out/kernel.krl",
        f"build pack: phony {xbps_out}",
        "default build", "",
        "rule clean", "  command = rm -rf out kernel/build_config.h .pkg", "build clean: clean",
    ]

    (ROOT / "build.ninja").write_text("\n".join(L) + "\n", encoding="utf-8")
    print("已生成 build.ninja（仅内核）。构建：ninja")


if __name__ == "__main__":
    main()

