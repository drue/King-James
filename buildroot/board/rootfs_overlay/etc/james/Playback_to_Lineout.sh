#!/bin/bash


#Playback from AP to Headset
amixer -Dhw:0 cset name='HPOUT2 Digital Switch' on
amixer -Dhw:0 cset name='HPOUT2L Input 1' AIF1RX1
amixer -Dhw:0 cset name='HPOUT2L Input 1 Volume' 32
amixer -Dhw:0 cset name='HPOUT2R Input 1' AIF1RX2
amixer -Dhw:0 cset name='HPOUT2R Input 1 Volume' 32


# The following command can be used to test
# aplay -Dhw:0 -r 44100 -c 2 -f S32_LE <file>



