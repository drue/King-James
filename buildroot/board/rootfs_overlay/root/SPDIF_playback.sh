#!/bin/bash

#Playback to SPDIF Out

#SPDIF Playback: 

amixer -Dhw:0 cset name='SPDIF out Switch' on
amixer -Dhw:0 cset name='TX Playback Switch' on
amixer -Dhw:0 cset name='Input Source' AIF
amixer -Dhw:0 cset name='AIF Playback Switch' on
amixer -Dhw:0 cset name='AIF2TX1 Input 1' AIF1RX1
amixer -Dhw:0 cset name='AIF2TX1 Input 1 Volume' 32
amixer -Dhw:0 cset name='AIF2TX2 Input 1' AIF1RX2
amixer -Dhw:0 cset name='AIF2TX2 Input 1 Volume' 32

# The following command can be used to test
# aplay -Dhw:0 -r 44100 -c 2 -f S32_LE <file>
