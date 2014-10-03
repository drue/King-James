#!/bin/bash

#Record from SPDIF in

#SPDIF Record: 
amixer -Dhw:0 cset name='SPDIF in Switch' on
# switch off SPDIF TX in case already active as 
# WM8804 must run at RX rate if enabled

amixer -Dhw:0 cset name='TX Playback Switch' off
amixer -Dhw:0 cset name='RX Playback Switch' on
amixer -Dhw:0 cset name='AIF Playback Switch' on
amixer -Dhw:0 cset name='AIF1TX1 Input 1' AIF2RX1
amixer -Dhw:0 cset name='AIF1TX1 Input 1 Volume' 32
amixer -Dhw:0 cset name='AIF1TX2 Input 1' AIF2RX2
amixer -Dhw:0 cset name='AIF1TX2 Input 1 Volume' 32

# The following command can be used to test
# arecord -Dhw:0 -r 44100 -c 2 -f S32_LE <file>



