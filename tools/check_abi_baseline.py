#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Проверка ABI-базовой линии Linux-сборки VanessaExt.

Скрипт читает у собранных .so таблицу требуемых версий символов
(.gnu.version_r) и падает, если артефакт требует более новых символов
glibc или libstdc++, чем разрешено базовой линией.

Зачем это нужно: версия символа в libstdc++/glibc зависит не от кода, а от
того, против какой библиотеки выполнялась компоновка. Например, вызов
std::condition_variable::wait() на машине с libstdc++ из GCC 12 помечает
символ как GLIBCXX_3.4.30, и .so перестаёт загружаться на Ubuntu 18.04
(GLIBCXX_3.4.22) и Ubuntu 20.04 (GLIBCXX_3.4.28): dlopen падает, 1С не
подключает внешнюю компоненту. Такая ошибка не видна ни компилятору,
ни CI без отдельной проверки.

Примеры:
    python3 tools/check_abi_baseline.py bin64/libVanessaExt.so
    python3 tools/check_abi_baseline.py --glibc 2.17 --glibcxx 3.4.19 *.so
    python3 tools/check_abi_baseline.py --list *.so

Код возврата: 0 — все файлы в пределах базовой линии, 1 — есть нарушения.
"""

import argparse
import os
import struct
import sys

# Базовая линия по умолчанию: Ubuntu 18.04 — glibc 2.27 и libstdc++ 8.4
# (GLIBCXX_3.4.25, CXXABI_1.3.11). Ubuntu 20.04 (glibc 2.31, GLIBCXX_3.4.28)
# новее, поэтому проверки на 18.04 автоматически покрывают и её.
DEFAULT_GLIBC = "2.27"
DEFAULT_GLIBCXX = "3.4.25"
DEFAULT_CXXABI = "1.3.11"

SHT_GNU_VERNEED = 0x6FFFFFFE


def version_key(value):
    """'2.29' -> (2, 29); пустая строка -> (0,)."""
    parts = []
    for item in str(value).split("."):
        try:
            parts.append(int(item))
        except ValueError:
            parts.append(0)
    return tuple(parts) or (0,)


class Elf:
    """Минимальный ELF-парсер: только то, что нужно для .gnu.version_r."""

    def __init__(self, path):
        with open(path, "rb") as handle:
            self.data = handle.read()
        data = self.data
        if len(data) < 0x34 or data[:4] != b"\x7fELF":
            raise ValueError("not an ELF file")
        self.is64 = data[4] == 2
        if data[5] not in (1, 2):
            raise ValueError("unknown data encoding")
        self.end = "<" if data[5] == 1 else ">"
        if self.is64:
            (self.shoff,) = struct.unpack_from(self.end + "Q", data, 0x28)
            self.shentsize, self.shnum, self.shstrndx = struct.unpack_from(
                self.end + "HHH", data, 0x3A)
        else:
            (self.shoff,) = struct.unpack_from(self.end + "I", data, 0x20)
            self.shentsize, self.shnum, self.shstrndx = struct.unpack_from(
                self.end + "HHH", data, 0x2E)
        self.sections = []
        for index in range(self.shnum):
            offset = self.shoff + index * self.shentsize
            if self.is64:
                fields = struct.unpack_from(self.end + "IIQQQQIIQQ", data, offset)
            else:
                fields = struct.unpack_from(self.end + "IIIIIIIIII", data, offset)
            name, kind, flags, addr, sh_offset, size, link, info, align, entsize = fields
            self.sections.append({
                "name_off": name, "type": kind, "offset": sh_offset,
                "size": size, "link": link, "entsize": entsize,
            })
        if not self.sections or self.shstrndx >= len(self.sections):
            raise ValueError("broken section headers")
        strtab = self.sections[self.shstrndx]
        self.shstr = data[strtab["offset"]:strtab["offset"] + strtab["size"]]
        self.by_name = {}
        for section in self.sections:
            section["name"] = self.cstring(self.shstr, section["name_off"])
            self.by_name[section["name"]] = section

    def cstring(self, blob, offset):
        end = blob.find(b"\0", offset)
        if end < 0:
            end = len(blob)
        return blob[offset:end].decode("utf-8", "replace")

    def section(self, name):
        return self.by_name.get(name)

    def section_data(self, name):
        section = self.by_name.get(name)
        if not section:
            return b""
        start = section["offset"]
        return self.data[start:start + section["size"]]

    def comment(self):
        text = self.section_data(".comment").decode("utf-8", "replace")
        items = [item.strip() for item in text.split("\0") if item.strip()]
        return "; ".join(items)

    def version_needs(self):
        """Список (библиотека, версия) из .gnu.version_r."""
        blob = self.section_data(".gnu.version_r")
        dynstr = self.section_data(".dynstr")
        result = []
        offset = 0
        while offset < len(blob):
            _, count, file_off, aux_off, next_off = struct.unpack_from(
                self.end + "HHIII", blob, offset)
            library = self.cstring(dynstr, file_off)
            aux = offset + aux_off
            for _ in range(count):
                _, _, _, name_off, aux_next = struct.unpack_from(
                    self.end + "IHHII", blob, aux)
                result.append((library, self.cstring(dynstr, name_off)))
                if aux_next == 0:
                    break
                aux += aux_next
            if next_off == 0:
                break
            offset += next_off
        return result

    def version_symbols(self):
        """Список (библиотека, версия, символ) для всех импортируемых символов."""
        needs = self.version_needs()
        blob = self.section_data(".gnu.version_r")
        dynstr = self.section_data(".dynstr")
        index_map = {}
        offset = 0
        while offset < len(blob):
            _, count, file_off, aux_off, next_off = struct.unpack_from(
                self.end + "HHIII", blob, offset)
            library = self.cstring(dynstr, file_off)
            aux = offset + aux_off
            for _ in range(count):
                _, _, other, name_off, aux_next = struct.unpack_from(
                    self.end + "IHHII", blob, aux)
                index_map[other] = (library, self.cstring(dynstr, name_off))
                if aux_next == 0:
                    break
                aux += aux_next
            if next_off == 0:
                break
            offset += next_off

        version_table = self.section_data(".gnu.version")
        dynsym = self.section_data(".dynsym")
        dynsym_section = self.section(".dynsym")
        if not version_table or not dynsym or dynsym_section is None:
            return []
        entry_size = dynsym_section["entsize"] or (24 if self.is64 else 16)
        count = len(dynsym) // entry_size
        result = []
        for index in range(count):
            raw = struct.unpack_from(self.end + "H", version_table, index * 2)[0]
            mapped = index_map.get(raw & 0x7FFF)
            if not mapped:
                continue
            entry = dynsym[index * entry_size:(index + 1) * entry_size]
            if self.is64:
                name_off, info, _, shndx = struct.unpack_from(self.end + "IBBH", entry, 0)
            else:
                name_off, _, _, info, _, shndx = struct.unpack_from(self.end + "IIIBBH", entry, 0)
            if shndx != 0:
                continue  # определённый в файле символ, не импорт
            name = self.cstring(dynstr, name_off)
            if not name:
                continue
            result.append((mapped[0], mapped[1], name))
        return result


def family_of(library, version):
    if version.startswith("GLIBCXX_"):
        return "GLIBCXX"
    if version.startswith("CXXABI_"):
        return "CXXABI"
    if version.startswith("GLIBC_"):
        return "GLIBC"
    return None


def strip_prefix(family, version):
    for prefix in ("GLIBCXX_", "CXXABI_", "GLIBC_"):
        if version.startswith(prefix):
            return version[len(prefix):]
    return version


def check_file(path, limits, show_all):
    elf = Elf(path)
    print("== %s" % path)
    comment = elf.comment()
    if comment:
        print("   собран: %s" % comment)

    grouped = {}
    for library, version, symbol in elf.version_symbols():
        family = family_of(library, version)
        if family is None:
            continue
        grouped.setdefault(family, {}).setdefault(version, []).append(symbol)
    # Библиотеки могут объявлять версии без импортируемых символов.
    for library, version in elf.version_needs():
        family = family_of(library, version)
        if family is not None:
            grouped.setdefault(family, {}).setdefault(version, [])

    failed = False
    for family in ("GLIBC", "GLIBCXX", "CXXABI"):
        versions = grouped.get(family, {})
        if not versions:
            print("   %-8s: нет требований" % family)
            continue
        limit = limits[family]
        actual = max(versions, key=lambda item: version_key(strip_prefix(family, item)))
        ok = version_key(strip_prefix(family, actual)) <= version_key(limit)
        print("   %-8s: максимум %-10s предел %-10s %s"
              % (family, strip_prefix(family, actual), limit, "OK" if ok else "FAIL"))
        if not ok:
            failed = True
        for version in sorted(versions, key=lambda item: version_key(strip_prefix(family, item)),
                              reverse=True):
            bad = version_key(strip_prefix(family, version)) > version_key(limit)
            if not bad and not show_all:
                continue
            symbols = sorted(set(versions[version]))
            print("      %s%s -> %s" % ("" if not bad else "! ", version,
                                        ", ".join(symbols) if symbols else "-"))
    return failed


def main():
    # Консоль Windows по умолчанию не умеет UTF-8 — вывод не должен падать.
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):
            pass

    parser = argparse.ArgumentParser(
        description="Проверка ABI-базовой линии Linux-сборки VanessaExt")
    parser.add_argument("files", nargs="+", help="собранные .so")
    parser.add_argument("--glibc", default=DEFAULT_GLIBC,
                        help="максимум GLIBC (по умолчанию %s)" % DEFAULT_GLIBC)
    parser.add_argument("--glibcxx", default=DEFAULT_GLIBCXX,
                        help="максимум GLIBCXX (по умолчанию %s)" % DEFAULT_GLIBCXX)
    parser.add_argument("--cxxabi", default=DEFAULT_CXXABI,
                        help="максимум CXXABI (по умолчанию %s)" % DEFAULT_CXXABI)
    parser.add_argument("--list", action="store_true",
                        help="печатать все версии символов, а не только нарушения")
    args = parser.parse_args()

    limits = {"GLIBC": args.glibc, "GLIBCXX": args.glibcxx, "CXXABI": args.cxxabi}
    failed_files = []
    for path in args.files:
        if not os.path.exists(path):
            print("== %s\n   файл не найден" % path)
            failed_files.append(path)
            continue
        try:
            if check_file(path, limits, args.list):
                failed_files.append(path)
        except (ValueError, struct.error) as error:
            print("== %s\n   не удалось разобрать ELF: %s" % (path, error))
            failed_files.append(path)

    print()
    if failed_files:
        print("ABI-базовая линия нарушена (%d): %s" % (len(failed_files), ", ".join(failed_files)))
        print("Соберите библиотеку в окружении с нужной базовой линией "
              "(например, Ubuntu 18.04 + gcc-7) или уберите из кода вызовы, "
              "которые приносят новые версии символов.")
        return 1
    print("ABI-базовая линия соблюдена: GLIBC <= %s, GLIBCXX <= %s, CXXABI <= %s"
          % (limits["GLIBC"], limits["GLIBCXX"], limits["CXXABI"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
