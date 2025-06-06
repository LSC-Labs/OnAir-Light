# OnAir Light
![OnAirLight](doc/IMG_7826.png)
Informs those around you about active transmission and prevents accidental "disturbance" or other unpleasant situations by third parties while you are conducting a meeting. Clients for different platforms can use the light to automatic signal their state. A Windows notifier is available - when a session starts, the light will be switched on and of automatically by this module.

For this reason this software supports standard OnAir Lights by extending them with the following features:
- Automatic detection of active cameras.
- Automatic detection of active microphones.
- Multi-client support.
- Multiple luminaires in a network, with their own states, or interconnecting multiple luminaires into one system.
- Integration into smart home automation solutions.
- Provide REST APIs (Public and Authenticated)
- MQTT client implementation with authentication against the server. Is supporting heartbeats and last will mechanism.
- Support for commercially available radio remote controls.
- OTA (Over The Air) update.
- Authentication protection of the admin settings.
- Detecting new published software, with direct download.
- Backup / Restore of configuration settings.
- Reboot and Factory Reset functions.

## What is "inside"
![OnAirLight](doc/IMG_7761.jpeg)
Based on a Wemos D1 mini, additional hardware is in place:
- Li-Ion Battery charger / USV
- Li-Ion Battery itself
- RF433 Radio module to receive remote controls.
- Power amplifier to support LED stripes up to 50W.
- LED stripe(s)
- Enhanced WLAN antenna, replaces the Wemos D1 on board
- Enhanced RF433 antenna.
- Additional button to toggle the light or to restart / reset to default settings.

## Build the program
The program is per default designed for the Wemos D1 mini in debug mode and is using currently supported runtimes. The debug version writes a lot of information at the serial port and you can configure what should be reported by the -D compiler flags.
You can identify the debug version in the GUI by the attched "-D" in the release string.

If you want to build the "release" version, you have to select it explicit in the PlatformIO GUI. Be aware, the build counter will increase the build number.

The firmware is stored in the "bin" folder (debug and release) and you can upload it to the module via the OTA feature.