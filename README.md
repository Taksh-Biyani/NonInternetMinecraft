# PineconeMC Offline

A portable Minecraft: Java Edition launcher for Windows computers **without internet**.

Everything new (a Minecraft version, Forge, NeoForge, Fabric or Quilt, a modpack, Java) arrives as **one `.zip` "offline bundle"**, carried on a USB stick from a computer that has internet and installed with **Import Bundle**. The same launcher, run on the online computer, makes those bundles with **Export Bundle**.

With internet it behaves like a normal launcher: Microsoft and Ely.by sign-in, Modrinth and CurseForge browsing, version refresh.

## Features

- **Offline mode:** Automatic, Always offline or Always online. Offline, the launcher never tries to download anything and only uses what was imported.
- **Offline bundles:**
  - Export a set of Minecraft versions with loaders, or a whole instance (including Modrinth and CurseForge modpacks, and optionally worlds).
  - Imports are checked and atomic: a damaged or incomplete bundle changes nothing.
- **LAN with offline accounts:** two or more offline players can play together over LAN with no internet, using a small local sign-in agent and the bundled [authlib-injector](https://github.com/yushijinhun/authlib-injector).
- **Ready to run offline:** the release zip includes Eclipse Temurin Java 25, 21, 17 and 8, all launcher translations, and the Visual C++ runtime.
- **Easy to use:**
  - a first-run welcome wizard;
  - plain-language error messages with a "what to do";
  - a complete offline guide (`Guide.html`, also opened with F1);
  - text size and high-contrast options, keyboard and screen-reader support.

## Using it

Download the release zip, unzip it anywhere (not under `C:\Program Files`) and run `pineconemc-offline.exe`. `README-FIRST.txt` and `Guide.html` in the folder explain the rest.

You need to own Minecraft: Java Edition.

## Building (Windows x64)

Requirements: Visual Studio 2026 Build Tools (C++ workload), Git, Python 3 on `PATH`, and PowerShell 5.1.

```powershell
git clone --recurse-submodules <this repository>
cd <repository folder>
powershell -ExecutionPolicy Bypass -File scripts\offline\setup-toolchain.ps1   # Qt 6.10.2, vcpkg and a JDK into .deps\
powershell -ExecutionPolicy Bypass -File scripts\offline\build.ps1 -Package    # builds and installs a portable copy into dist\
powershell -ExecutionPolicy Bypass -File scripts\offline\test.ps1              # unit tests
```

The first build compiles the vcpkg dependencies and takes a while.

### Making a release

```powershell
powershell -ExecutionPolicy Bypass -File scripts\offline\fetch-runtimes.ps1   # Temurin JREs and translations (checksummed, cached)
powershell -ExecutionPolicy Bypass -File scripts\offline\make-release.ps1     # dist\release\*.zip
```

`make-release.ps1` builds the zip from a fresh install, so it never contains user data. `check-release.ps1` verifies it.

## License and credits

PineconeMC Offline is free software under the **GNU General Public License v3.0** (see `LICENSE` and `COPYING.md`). It is a fork of:

- [PineconeMC / ElyPrismLauncher](https://github.com/ElyPrismLauncher/Launcher), a fork of [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher) (and before it PolyMC and MultiMC), whose authors keep their copyrights;
- [authlib-injector](https://github.com/yushijinhun/authlib-injector) by yushijinhun (AGPL-3.0 with the authlib-injector exception), shipped unmodified;
- [Eclipse Temurin](https://adoptium.net/) Java runtimes (GPL-2.0 with the Classpath Exception), shipped in the release zip.

Minecraft is a trademark of Mojang Synergies AB. This launcher is not an official Minecraft product and is not approved by or associated with Mojang or Microsoft.
