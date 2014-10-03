#!/bin/bash

#Record from Headset to AP
amixer -Dhw:0 cset name='IN1R Volume' 20
#amixer -Dhw:0 cset name='IN1 Digital Switch' on
amixer -Dhw:0 cset name='AIF1TX1 Input 1' IN1R
amixer -Dhw:0 cset name='AIF1TX1 Input 1 Volume' 32
amixer -Dhw:0 cset name='AIF1TX2 Input 1' IN1R
amixer -Dhw:0 cset name='AIF1TX2 Input 1 Volume' 32
amixer -Dhw:0 cset name='Headset Mic Switch' on
# This is set up as a dual mono record.
# May not be what we want in all scenarios

# The following command can be used to test
# arecord -Dhw:0 -r 44100 -c 2 -f S32_LE <file>



