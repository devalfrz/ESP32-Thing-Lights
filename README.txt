Lights with PIR sensor. Lights can be turned on and off normally
from the power switch.
When the system is on, the lights will turn on for about 1 min until
it detects no precense.
The system can be armed or disarmed through the network.
It can also be updated if it is connected to the internet


Urls:
/system/1      Turns on motion detection system
/system/0      Turns off motion detection system
/lights/1      Turn on lights
/lights/0      Turn off lights
/update        Firmware update


v1.8   2018-05-27
Added body styles.
Wait to listen to PIR sensor if lights are turned on/off by the web.
Show on and off options on web interface.

v1.7   2017-12-17
Fixed Reconnection issues

v1.6   2017-11-17
System ON by default

v1.5   2017-06-23
Remaped relay pin

v1.4.2   2017-06-23
Ignore wifi if unable to connect, try again after some time

v1.4.1   2017-06-23
Removed touch button

v1.4     2017-06-22
Added url to controll lights

v1.3     2017-06-20
Added firmware update

v1.2     2017-06-20
Added server to activate or deactivate system

v1.1     2017-06-20
Post to alarm URL if it is activated
Avoid interrupts while changing state

v1.0     2017-06-20


Alfredo Rius
alfredo.rius@gmail.com
