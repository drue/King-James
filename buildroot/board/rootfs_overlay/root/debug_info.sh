#!/bin/bash

mount -t debugfs nodev /sys/kernel/debug

alias stop_audio='kill $(ps -C aplay,arecord -o pid=)'


echo dumping 5102 dapm controls to dapm_dump ;

cat /sys/kernel/debug/asoc/snd_rpi_wsp/wm5102-codec/dapm/* >dapm_dump ;
echo controls that are on ;
cat dapm_dump | grep On ;

echo Dumping 5102 register map to regdump_5102 ;
cat /sys/kernel/debug/regmap/spi0.1/registers >regdump_5102 ;

echo Dumping 8804 register map to regdump_8804 ;
cat /sys/kernel/debug/regmap/1-003a/registers >regdump_8804


