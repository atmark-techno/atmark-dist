#!/bin/sh

. /etc/init.d/functions

PATH=/bin:/sbin:/usr/bin:/usr/sbin

CARD_NAME=armadillo4x0_wm8978
HEADPHONE_VOLUME=50%
SPEAKER_VOLUME=50%
MIC_VOLUME=75%

check_audio_device_exist() {
	aplay -l |grep $CARD_NAME > /dev/null 2>&1
}

check_audio_device_exist
if [ $? -eq 0 ]; then
	echo -n "Initializing the audio setting: "
	amixer -D hw:0 sset 'Headphone' $HEADPHONE_VOLUME > /dev/null 2>&1
	amixer -D hw:0 sset 'Speaker'   $SPEAKER_VOLUME > /dev/null 2>&1
	amixer -D hw:0 sset 'PGA Boost (+20dB)' off > /dev/null 2>&1
	amixer -D hw:0 sset 'Input PGA' $MIC_VOLUME > /dev/null 2>&1
	check_status
fi
