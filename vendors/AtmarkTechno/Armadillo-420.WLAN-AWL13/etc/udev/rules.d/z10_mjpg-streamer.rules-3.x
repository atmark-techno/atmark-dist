KERNEL=="video*", SUBSYSTEM=="video4linux", ENV{ID_USB_DRIVER}=="uvcvideo", DEVPATH=="*/imx27-usb*/usb2/*", ACTION=="add", RUN+="/etc/config/mjpg-streamer.sh start %k"

KERNEL=="video*", ACTION=="remove", RUN+="/etc/config/mjpg-streamer.sh stop %k"
