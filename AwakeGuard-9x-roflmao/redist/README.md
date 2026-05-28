# Optional MSVCRT.DLL (legacy builds only)

**Default ROFLMAO builds do not use this folder.**

The project links with **`-mcrtdll=msvcrt20`**, so the exe loads **`MSVCRT20.DLL`** from `C:\Windows\System` on stock Windows 95.

Only if you remove that flag and return to MinGW’s default **`msvcrt`** target would you need to ship a **VC6-era `MSVCRT.DLL`** (~270–290 KB, version **6.x**) next to the exe. **Never** use `%SystemRoot%\SysWOW64\msvcrt.dll` (version **7.x**) on Win95.

To stage such a build on floppy: `.\scripts\prepare-floppy.ps1 -IncludeMsvcrt`
