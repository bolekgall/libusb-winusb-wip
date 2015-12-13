# Activity Log #
```
MAIN:
o detect WinUSB during enum to prevent WinUSB calls on non WinUSB devives [DONE using driver string: 2009.12.13]
o detect driverless devices during enum for future automated WinUSB driver addon
  - conn_info.CurrentConfigurationValue false => priv->driver = "no_driver" [DONE: 2009.12.14]
o actually read active configuration from conn_info [DONE: 2009.12.15]
o comment the inf with regards to CoInstallers' choice and add provision for Multiple Interfaces (MI_##) [DONE: 2010.01.08]
o xusb winusb test application
  - add string I/O to xusb [DONE: 2009.12.15]
  - full XBox controller support using control requests [DONE: 2010.01.03]
  - bulk I/O against USB key (Bulk-only Mass Storage) [DONE: 2010.01.04]
o async (polled/overlapped) I/O
  - crude poll using OVERLAPPED pointers as fds + HasOverlappedIoCompleted [DONE: 2009.12.16]
  - control transfers [DONE: 2010.01.02]
  - retrieve actual length [DONE: 2009.12.16]
  - bulk/interrupt [DONE: 2010.01.04]
  - I/O cancellation [DONE: 2010.01.05]
o device reset [DONE: 2010.01.05]
o the never ending multiple interfaces handling
  - generic readout of registry keys and values [DONE: 2009.12.18]
  - retrieve full device path from HKLM\SYSTEM\ControlSet001\Control\DeviceClasses\* [DONE: 2009.12.18]
  - WinUSB interface selection through MI_## [DONE: 2009.12.18]
  - new non-controversial interface enumeration through SetupDi [DONE: 2009.12.30]
  - WinUSB detection for interfaces [DONE: 2009.12.30]
o poll & pipe redesign
  - support for control fd ("fish in the pipe") [(actually) DONE: 2009.12.22]
  - overlapped support all the way [DONE: 2009.12.22]
  - mutex fd locking [DONE: 2009.12.23]
o better/multiple API handling (refer to libusb-win32-v1) [API_CALL macro - DONE: 2009.12.18]
o sanitize_path & windows_error_string improvements [DONE: 2009.12.18]
o test on WinXP [OK: 2009.12.30]
o test against openocd/libftdi [OK: 2010.01.08]
o test x64 builds in MSVC
o multithreading
o better composite interfaces handling, with fully independent interface and composite device drivers
o write some detailed notes for pthread-win32 integration on project page
o write a detailed guide for manual installation of the WinUSB driver

EXTRAS:
o MSVC full compilation (preferred) or MSVC compatible MinGW lib with MSVC test sample
  - MSVC6 compatibility for windows_usb.c & windows_usb.h only (Michael Plante) [DONE: 2010.01.11]
  - MSVC9 full compatibility [DONE: 2010.01.11]
  - MSVC6 full compatibility [DON]
o auto-claiming interface for control transfers if none available [DONE: 2010.01.08]
o add elementary HID (non WinUSB) handling so that we get the feel of how APIs should be broken down 
  => without interface and endpoint access through HID layer, not sure how far we can get
o hotplug
o automated driver installation. 
o is there a way to disable google's annoying <pre> colouring on this wiki?
o use usbi list functions for our hcd chained list
o Win2k support? (Not from me!)

BUGFIXES:
o deviceless external hub random failure on descriptors. Is there anything we can do?
  => check what's happening on the bus
o SetupDI### call enumerates composite devices at the root ("usbccgp") rather than the leaves ("WinUSB") 
  => trying to obtain the FULL path (i.e. path that works with CreateFile) of the WinUSB leaves of a composite device handled by usbccgp is an absolute nightmare! 
     For now, just replace the composite driver with a WinUSB one. Install is a pain (must force upgrade), but it works. ["FIXED": 2009.12.16]
o Well, above doesn't work for interface selection through WinUsb_GetAssociatedInterface (what the ???)
  => back to the (more versatile) separate MI_## for composite devices' interfaces ["RE-FIXED": 2009.12.17]
o poll says control fd overlapped IO complete [FIXED with a "poll we can believe in": 2009.12.22]
o bad composite device detection when device and hub have same port number [FIXED 2009.12.23: Don't use sizeof when you mean strlen!]
o use of ControlSet001 instead of CurrentControlSet[FIXED 2009.12.26]
o incorrect function on force_hcd_device_descriptor on XP [FIXED 2009.12.26: misplaced _EX]
o Location Information on XP does NOT return location information! [WORKAROUND: 2009.12.30]
o everlasting wait on devices that have gone to sleep? / timeouts don't work [FIXED: 2010.01.07]
o reset of Mass Storage devices sure doesn't seem to work as expected... [IT DOES when you don't forget the CSW query: 2010.01.07]
o crashes in pthreadGC2.dll when poll is unhappy about an fd [FIXED: 2010.01.07]
o doesn't use the default pthread-win32 library name on MinGW [FIXED: 2010.01.11]
o the infamous device with serial => no port# issue on XP [FIXED: 2010.01.12]
o 'CM_GETIDLIST_FILTER_BITS' macro redefinition error in cfgmgr32.h for Windows 7
```