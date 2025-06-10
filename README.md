# aml-enc-test
test for AML h264 encoder in VIM4

it encodes 150 frames at one bitrate, changes bitrate and then encodes 150 more frames, displaying the mean bitrate and the target.

The test is to see if the numbers change when the requested bitrate change is made.

compile with cc -o amlenc amlenc.c -lvpcodec
