# Local dependency patches

## SPIRV-Tools VS Code activation

spirv-tools-vscode-activation.patch changes the bundled VS Code extension activation
event from * to onLanguage:spirv. It only affects the editor extension.

The patch is kept here because SPIRV-Tools is an upstream submodule. Its pinned
commit remains available from the original upstream repository.

After initializing submodules, apply from the repository root:

```powershell
git -C 3rdparty/SPIRV-Tools apply --check ../../patches/spirv-tools-vscode-activation.patch
git -C 3rdparty/SPIRV-Tools apply ../../patches/spirv-tools-vscode-activation.patch
```

It is already applied in the current development checkout. To check that state:

```powershell
git -C 3rdparty/SPIRV-Tools apply --reverse --check ../../patches/spirv-tools-vscode-activation.patch
```
