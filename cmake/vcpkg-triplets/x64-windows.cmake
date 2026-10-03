# vcpkg's x64-windows triplet, plus: record only the PDB file name in the DLLs, not the full build path
# (the release zip must not contain paths from the machine it was built on).
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_PROVIDED_FORTRAN ON)
set(VCPKG_LINKER_FLAGS "/PDBALTPATH:%_PDB%")
