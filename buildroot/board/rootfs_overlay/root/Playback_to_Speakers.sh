#!/bin/bash


#Playback from AP to Speaker

amixer -Dhw:0 cset name='Speaker Digital Volume' 128
# reset speaker mixer inputs
amixer -Dhw:0 cset name='SPKOUTL Input 1' None
amixer -Dhw:0 cset name='SPKOUTR Input 1' None
amixer -Dhw:0 cset name='SPKOUTL Input 2' None
amixer -Dhw:0 cset name='SPKOUTR Input 2' None
# Route AP to Speaker mixer
amixer -Dhw:0 cset name='SPKOUTL Input 1' AIF1RX1
amixer -Dhw:0 cset name='SPKOUTL Input 1 Volume' 32
amixer -Dhw:0 cset name='SPKOUTR Input 1' AIF1RX2
amixer -Dhw:0 cset name='SPKOUTR Input 1 Volume' 32
# Unmute speaker output
amixer -Dhw:0 cset name='Speaker Digital Switch' on


# The following command can be used to test
# aplay -Dhw:0 -r 44100 -c 2 -f S32_LE <file>
