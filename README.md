# PolluComE
Sensus Pollucom IR reader helper for Linux/Raspberry Pi

reads data from an IR head and sends data to stdout

Nov 2018 J. Seyfried
based on:
* https://www.mikrocontroller.net/topic/113984#3901715
* https://wiki.volkszaehler.org/hardware/channels/meters/warming/sensus_pollucom
* https://github.com/UDOOboard/serial_libraries_examples/blob/master/c/c_serial_example_bidirectional.c

Article describing all the details to follow. Will put a link here, of course.

## compiling and installing
log into your Raspberry Pi, do a
```
git clone https://github.com/JoeSey/PolluComE.git
gcc pollucom.c -o pollucom
```

## usage
Help is available using
`  ./pollucom -h`

## example output
```
pi@zero1:~ $ ./pollucom
Energie[kWh]: 82362
Volumen: 15717.543
Durchfluss[cbm/h]: 0.000
Leistung[W]: 0
Durchflusstemp.[°C]: 42.9
Ruecklauftemp.[°C]: 37.8
Temperaturdiff.[K]: 5.099
```

## what next?
grep the lines you're interested in. Put everything into a cron job and send the data to a volkszaehler instance logging that data. One fine day this might even turn into a volkszaehler plugin, who knows.
