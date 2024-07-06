
# Using GDBStub for ESP8266 in VS Code with PlatformIO

With my current setup it works with one click. :)

I know the wall of text "Notes" below might be scary, but **please read** them to avoid trouble later.





## Notes

+ See the `platformio.ini` file (and the code in general) for few more notes.
	+ Unlike `monitor_port`, the `debug_port` must be in full path even on Windows (i.e. `\\.\COM13` instead just `COM13`).
	+ If you don't want to set the port in the file, you will need to manually type `target remote \\\\.\\COM13` into the Debug Console tab in VS Code.
	+ You can use `pio device list` to list the devices.
+ Care! Adding `#include <GDBStub.h>` is enough to enable the GDB, since PlatformIO will build the `gdbstub.c` too, which includes proper (non-nop) implementation for `gdb_init()` and `gdb_break()` and so on. The `gdb_init()` is called even before user' `setup()` (see `core_esp8266_main.cpp`).
	+ Guarding the include with `#ifdef ENABLE_GDB` is possible, but requires also using `lib_ldf_mode = chain+` (or `deep+`) in `platformio.ini`, to instruct LDF to evaluate conditional syntax. Read about the [Library Dependency Finder](https://docs.platformio.org/en/stable/librarymanager/ldf.html). You can `#include <gdb_hooks.h>` if you want to use `gdb_do_break()` without extra macros.
	+ You can check if your binary is built with the GDBStub using something like: `grep 'gdbstub_init' ./.pio/build/esp8266_debug/firmware.elf`.
+ Sometimes, if using GDBStub, it's hard to upload firmware to the device, requiring: multiple tries, power cycling, changing boot mode etc; most importantly: **luck**. Maybe it's because no grace time before first break? I decided not to use `GDBSTUB_BREAK_ON_INIT`, and go with `gdb_do_break()` only after some delay. Notabene, PlatformIO takes few seconds from uploading to attaching the debugger (incl. serial monitor), so the **delay might be required** anyways.
+ Any software break, like `gdb_do_break()` (incl. on init if `GDBSTUB_BREAK_ON_INIT=1`) holds execution if GDBStub is enabled, **even if the debugger is not attached**.
+ If you use `GDBSTUB_BREAK_ON_INIT=1` it can break before your expected serial baud rate is set, which might be confusing.
+ VS Code debugging sometimes hangs up.
	+ Either fully use VS Code interface - which looks nice, but can hang up and feels buggy.
		+ If you manually type some GDB commands in Debug Console tab, for example: `c`/`continue` it will continue, but you have no way of interrupting that anymore, `interrupt` nor `signal SIGINT` doesn't work; can't send the `Ctrl+C`... 
		+ On hang ups, you can try pressing stop few times and it disconnects, and you can reconnect by starting the debugging again; some instructions may have passed, but it doesn't forcefully restart the whole thing (unless you modified your files or )
		+ Many commands are safe if you are on breakpoint, like `print`, `info`, `tbreak`, etc.
		+ No support for temporary breakpoints requires you to disable previous breakpoints.
		+ You need disable all breakpoints before starting the debug if you use `debug_init_break` (actually inside `debug_init_cmds`).
	+ Or use GDB manually - which is harder, requires knowledge and manual operation but feels more solid.
+ Some stuff just doesn't work at all:
	+ `call`ing functions from GDB, incl. in expressions.
+ There is limit of single watchpoint and single breakpoint! Tip: use temporary breakpoints, example: `tbreak loop`. 
+ See also [Arduino core for ESP8266 docs about GDB usage](https://arduino-esp8266.readthedocs.io/en/latest/gdb.html).
+ [ESP8266 boot log shows on baud rate 74880](https://docs.espressif.com/projects/esptool/en/latest/esp8266/advanced-topics/boot-mode-selection.html#boot-log).



### Extra commands

The GDB path in my case:
```
C:\Users\PsychoX\.platformio\packages\toolchain-xtensa\bin\xtensa-lx106-elf-gdb.exe
```

More universally from PowerShell:
```powershell
. "$env:USERPROFILE\.platformio\packages\toolchain-xtensa\bin\xtensa-lx106-elf-gdb.exe"
```

Commands for GDB I manually use:
```gdb
set remote hardware-breakpoint-limit 1
set remote hardware-watchpoint-limit 1
set remote interrupt-on-connect on
set remote kill-packet off
set remote symbol-lookup-packet off
set remote verbose-resume-packet off
mem 0x20000000 0x3fefffff ro cache
mem 0x3ff00000 0x3fffffff rw
mem 0x40000000 0x400fffff ro cache
mem 0x40100000 0x4013ffff rw cache
mem 0x40140000 0x5fffffff ro cache
mem 0x60000000 0x60001fff rw
set serial baud 115200
file .pio/build/esp8266_debug/firmware.elf
target remote \\.\COM13
```

To reset the ESP without manually pressing buttons or reconnecting it I use:
```powershell
Stop-Process -Name "xtensa-lx106-elf-gdb" -Force
. "$env:USERPROFILE\.platformio\packages\tool-esptoolpy\esptool.py" --chip ESP8266 --port COM13 chip_id
```
<sup>In fact it reads the chip ID, but it's simplest thing to do</sup>



### To-do

+ Auto-detect for debug port as well? 
	+ Use advanced scripting; maybe reuse the monitor port variable (or code). 
	+ See https://community.platformio.org/t/auto-detect-port-explained/16062/2
+ Detect whenever the debugger is attached?
	+ Maybe some global variable and setting/clearing it on init/exit?
+ Prevent `core_esp8266_main.cpp` coming up on `gdb_do_break()`


