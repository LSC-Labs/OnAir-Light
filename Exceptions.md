# Read exceptions dump

When the ESP crashes and it's connected to the serial port in debug mode, you can get its stacktrace.

But then you need to decode it to see what's going on. To do that you need to:

- save the stacktrace in a file "exception.txt" in the root of your project (where platformio.ini resides).

- install https://github.com/janLo/EspArduinoExceptionDecoder/ (in case of this project it is stored in tools/ESPArduinoExceptionDecoder)

- If you are using Visual Code, you see NPM SCRIPTS in the lower area of the file navigator. Choose "decode d1_mini exception" and run the task.
> Alternate:
run `python3 tools/EspArduinoExceptionDecoder/decoder.py -e .pio/build/d1_mini/firmware.elf exception.txt -s`

References:
- https://github.com/esp8266/Arduino/blob/master/doc/faq/a02-my-esp-crashes.rst
- https://arduino-esp8266.readthedocs.io/en/latest/exception_causes.html
- https://arduino-esp8266.readthedocs.io/en/latest/Troubleshooting/stack_dump.html

