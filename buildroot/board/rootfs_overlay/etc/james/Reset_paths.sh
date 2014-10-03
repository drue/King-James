#!/bin/bash


# Clean setup
amixer -Dhw:0 cset name='TX Playback Switch' off
amixer -Dhw:0 cset name='RX Playback Switch' off
amixer -Dhw:0 cset name='AIF Playback Switch' off
amixer -Dhw:0 cset name='SPDIF in Switch' off
amixer -Dhw:0 cset name='SPDIF out Switch' off
amixer -Dhw:0 cset name='AIF2TX1 Input 1' None
amixer -Dhw:0 cset name='AIF2TX2 Input 1' None
amixer -Dhw:0 cset name='AIF1TX1 Input 1' None
amixer -Dhw:0 cset name='AIF1TX2 Input 1' None
amixer -Dhw:0 cset name='HPOUT1L Input 1' None
amixer -Dhw:0 cset name='HPOUT1R Input 1' None
amixer -Dhw:0 cset name='HPOUT1L Input 2' None
amixer -Dhw:0 cset name='HPOUT1R Input 2' None
amixer -Dhw:0 cset name='HPOUT2L Input 1' None
amixer -Dhw:0 cset name='HPOUT2R Input 1' None
amixer -Dhw:0 cset name='HPOUT2L Input 2' None
amixer -Dhw:0 cset name='HPOUT2R Input 2' None
#amixer -Dhw:0 cset name='IN3 Digital Switch' off
#amixer -Dhw:0 cset name='IN1 Digital Switch' off
amixer -Dhw:0 cset name='Headset Mic Switch' off
#amixer -Dhw:0 cset name='DMIC Switch' off
amixer -Dhw:0 cset name='IN2 Digital Switch' off
amixer -Dhw:0 cset name='SPKOUTL Input 1' None
amixer -Dhw:0 cset name='SPKOUTR Input 1' None
amixer -Dhw:0 cset name='SPKOUTL Input 2' None
amixer -Dhw:0 cset name='SPKOUTR Input 2' None
amixer -Dhw:0 cset name='Speaker Digital Switch' off




