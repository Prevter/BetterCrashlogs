<div align="center">
   <a href="https://github.com/prevter/gdopenhack">
      <img src="logo.png" alt="Logo" width="80" height="80">
   </a>
   <h3 align="center">Better Crashlogs</h3>
   <p align="center">
        A mod that overhauls the crash log system in Geometry Dash.
   </p>
</div>

> Note: This mod is still work in progress!

![image](resources/screenshot.png)

## Features
- [x] Resizable window using ImGui
- [x] More information about the exception
- [x] Registers and stack memory display strings/pointers (if available)
- [x] Saves crash logs to a file
- [x] Ability to copy crash log to clipboard
- [x] Restart button to restart the game
- [x] Stack trace with better formatting
- [x] Stack memory view
- [x] Handling breakpoints
- [x] Ability to continue execution (may crash again)
- [x] Motivational quotes (like in Minecraft)
- [ ] Base game method names (no more GeometryDash.exe+0x123456)
- [ ] Fetch .pdb files from mod's GitHub repository (if available)
- [ ] Basic disassembly view (using Zydis)

## TODO
- [ ] Parse C++ exceptions
- [ ] Automatically get the latest broma file for the game
- [ ] Add a settings menu to configure the theme/font size