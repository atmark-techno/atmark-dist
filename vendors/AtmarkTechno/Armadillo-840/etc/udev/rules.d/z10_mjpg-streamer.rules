KERNEL=="video*", SUBSYSTEM=="video4linux", DRIVERS=="uvcvideo", DRIVERS=="usb", ATTRS{busnum}=="2", ACTION=="add", RUN+="/etc/config/mjpg-streamer.sh start $KERNEL"

KERNEL=="video*", ACTION=="remove", RUN+="/etc/config/mjpg-streamer.sh stop $KERNEL"
