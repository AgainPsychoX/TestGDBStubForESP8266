
# Using GDBStub for ESP8266 in VS Code with PlatformIO

With my current setup it works with one click. :)

I know the wall of text "Notes" below might be scary, but **please read** them to avoid trouble later.





## Notes

#### Setup

+ See the `platformio.ini` file (and the code in general) for few more notes.
	+ Unlike `monitor_port`, the `debug_port` must be in full path even on Windows (i.e. `\\.\COM13` instead just `COM13`).
	+ If you don't want to set the port in the file, you will need to manually type `target remote \\\\.\\COM13` into the Debug Console tab in VS Code.
	+ You can use `pio device list` to list the devices.
	+ If you get upload before every debug session - even if you didn't change the code - you might want to set `debug_load_mode = manual`. In this project the `modified` option works okay, but in my other, more complex project, I can't get it to working (no files compiled, but still linking something I don't know).
+ Care! Adding `#include <GDBStub.h>` is enough to enable the GDB, since PlatformIO will build the `gdbstub.c` too, which includes proper (non-nop) implementation for `gdb_init()` and `gdb_break()` and so on. The `gdb_init()` is called even before user' `setup()` (see `core_esp8266_main.cpp`).
	+ Guarding the include with `#ifdef ENABLE_GDB` is possible, but requires also using `lib_ldf_mode = chain+` (or `deep+`) in `platformio.ini`, to instruct LDF to evaluate conditional syntax. Read about the [Library Dependency Finder](https://docs.platformio.org/en/stable/librarymanager/ldf.html). You can `#include <gdb_hooks.h>` if you want to use `gdb_do_break()` without extra macros.
	+ You can check if your binary is built with the GDBStub using something like: `grep 'gdbstub_init' ./.pio/build/esp8266_debug/firmware.elf`.
+ Sometimes, if using GDBStub, it's hard to upload firmware to the device, requiring: multiple tries, power cycling, changing boot mode etc; most importantly: **luck**. Maybe it's because no grace time before first break? I decided not to use `GDBSTUB_BREAK_ON_INIT`, and go with `gdb_do_break()` only after some delay. Notabene, PlatformIO takes few seconds from uploading to attaching the debugger (incl. serial monitor), so the **delay might be required** anyways.
+ If you use `GDBSTUB_BREAK_ON_INIT=1` it can break before your expected serial baud rate is set, which might be confusing.

#### Serial port

+ GDBStub works by taking over the serial port, using special protocol to talk with debugger on the other side.
+ I haven't figured out way to send "normal" serial data to the target device while using debugger (neither in VS Code or command line). **However**, you can stop debugger and open regular serial port. You can use `$D#44` to skip debugger break and then interact almost normally - except sending 0x03 which is interpreted as debugger interrupt - until hitting the breakpoint of course. Thing is, you can manually send the interrupt, if `GDBSTUB_CTRLC_BREAK=1` (default): in PlatformIO serial console you use `Ctrl+T` followed by `Ctrl+C`. Then, you can close the serial monitor and attach debugger again, and continue from the point. 

#### Other

+ Any software break, like `gdb_do_break()` (incl. on init if `GDBSTUB_BREAK_ON_INIT=1`) holds execution if GDBStub is enabled, **even if the debugger is not attached**.
+ VS Code debugging sometimes hangs up.
	+ Either fully use VS Code interface - which looks nice, but can hang up and feels buggy.
		+ If you manually type some GDB commands in Debug Console tab, for example: `c`/`continue` it will continue, but you have no way of interrupting that anymore, `interrupt` nor `signal SIGINT` doesn't work; can't send the `Ctrl+C`... 
		+ On hang ups, you can try pressing stop few times and it disconnects, and you can reconnect by starting the debugging again; some instructions may have passed, but it doesn't forcefully restart the whole thing (unless you modified your files or )
		+ Many commands are safe if you are on breakpoint, like `print`, `info`, `tbreak`, etc.
		+ Using `step` (or similar) works, but makes your typing focus jump to the editor which is annoying (BTW: `Ctrl`+`Shift`+`Y` jumps back to the Debug Console tab).
		+ No support for temporary breakpoints requires you to disable previous breakpoints.
		+ You need disable all breakpoints before starting the debug if you use `debug_init_break` (actually inside `debug_init_cmds`).
		+ Arduino `Serial.print`s in my code are mangled into separate lines, which makes it hard to read. In large amount of prints and fast rate it's a lot less readable.
	+ Or use GDB manually - which is harder, requires knowledge and manual operation but feels more solid.
		+ `Ctrl+C` works nicely as interrupt, of course if `GDBSTUB_CTRLC_BREAK=1` (default).
		+ Arduino `Serial.print`s work as expected (single lines, to tearing).
		+ If used [inside VS Code terminal window, there might be issues with arrow keys](https://github.com/microsoft/vscode/issues/221349) navigation in command buffer. In separate window it works okay.
+ Some stuff just doesn't work at all:
	+ `call`ing functions from GDB, incl. in expressions.
	+ Multiple breakpoints. **There is limit of only single breakpoint**. Tip: use temporary breakpoints, example: `tbreak loop`. 
+ When `step`ping thought the code, larger functions can take quite longer to execute. _Well, the whole code works slower with less optimization and GDB attached, what did you expect? ;)_ No, but seriously, it steps over functions can lag a bit even if the function executes very fast in free run outside breakpoint.
+ If you use Wi-Fi or Bluetooth or any other buffered peripherals, stopping the execution (on breakpoint) for longer times can cause undefined behaviors, hang ups or crashes, due to buffer overflows or missed frames.
+ If you use precise-timed peripherals, like bit banging the data it can return invalid data and cause invalid behaviour. In one of my projects I used OneWire which is all about timing and while it seemingly worked okay without breakpoint set, using the `step` by step approach after hitting breakpoint returned invalid data.
+ In my more complex project, sometimes somehow I get random `SIGTRAP` stopping the execution in seemingly random places in the code. Normal break point is also `SIGTRAP`, from `break 0, 0` assembly instruction, meanwhile this one is... out of thin air? By the way, external interrupt is `SIGINT` (by CTRL+C in command line or pause button in VS Code). Some undocumented feature? Or random alignment of memory or something detected by the debugger? 
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

+ Serial port input fix
	+ Layer/middleware between VS Code (PlatformIO), GDB and serial port
	+ Pass everything to GDB unless it starts with `stdin `, or while GDB prompt is not visible.
	+ Hook Ctrl+C to send `0x03`/interrupt; and maybe e.
	+ Allow creating log from debugging session, both GDB inputs/outputs and the serial port communications
+ Some issues I reported:
	+ https://community.platformio.org/t/debugging-esp8266-with-gdbstub-and-how-can-i-add-extra-parameters-to-autogenerated-launch-configurations/41686
+ Auto-detect for debug port as well? 
	+ Use advanced scripting; maybe reuse the monitor port variable (or code). 
	+ See https://community.platformio.org/t/auto-detect-port-explained/16062/2
+ Detect whenever the debugger is attached?
	+ Maybe some global variable and setting/clearing it on init/exit?
+ Prevent `core_esp8266_main.cpp` coming up on `gdb_do_break()`
+ Research:
	+ https://sming-slaff.readthedocs.io/en/fix-readthedocs-generation/_inc/Sming/Arch/Esp8266/Components/gdbstub/index.html#known-issues-and-limitations
	+ https://github.com/SmingHub/Sming/blob/0706c25e9fc7b42939cd420a0dea23df4c1e94c5/Sming/Arch/Esp8266/Components/gdbstub/


