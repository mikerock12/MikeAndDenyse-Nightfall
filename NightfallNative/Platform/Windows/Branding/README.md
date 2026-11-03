# Windows branding

`AppIconSource.png` is the approved source artwork; its origin is recorded in `provenance.json`. Regenerate containers with `python encode-icon.py` (Pillow). This performs only aspect-preserving resizing, transparent padding and ICO encoding.

`AppIcon.ico` contains 16–256 px sizes and is embedded in both the game PE resources and Inno Setup. `AppIcon.png` is embedded as RCDATA 102 in the game and loaded after InitWindow for its window/taskbar icon. Shortcuts and the uninstall entry use the game EXE icon, index 0. No external image file is needed at runtime.

Windows resources are compiled through CMake; Rua de Aço also supports its direct MSVC build script. VERSIONINFO follows the maintenance release.
