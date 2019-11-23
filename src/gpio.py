#!/usr/bin/env python

import Jetson.GPIO as GPIO
from subprocess import Popen, PIPE
import sys

stop = 12
rec = 11
menu = 18

key_r = "key R "
key_s = "key S "
key_m = "key M "

# emmulates keyboard press
def keypress(key):
    p = Popen(['xte'], stdin=PIPE)
    p.communicate(input=key)


# callbacks to handle presses
def stop_cb(channel = 0):
    keypress(key_s)

def rec_cb(channel = 0):
    keypress(key_r)

def menu_cb(channel = 0):
    keypress(key_m)


def main():
    GPIO.setwarnings(False)

    GPIO.setmode(GPIO.BOARD)

    # GPIO.setup([stop, rec, menu], GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup([stop, rec, menu], GPIO.IN)

    # # attach callbacks to events
    # GPIO.add_event_detect(stop, GPIO.RISING, callback=stop_cb, bouncetime=200)
    # GPIO.add_event_detect(rec, GPIO.RISING, callback=rec_cb, bouncetime=200)
    # GPIO.add_event_detect(menu, GPIO.RISING, callback=menu_cb, bouncetime=200)

    GPIO.add_event_detect(stop, GPIO.RISING)
    GPIO.add_event_detect(rec, GPIO.RISING)
    GPIO.add_event_detect(menu, GPIO.RISING)


    while True:
        try:
            if GPIO.event_detected(stop):
                stop_cb()
            if GPIO.event_detected(rec):
                rec_cb()
            if GPIO.event_detected(menu):
                menu_cb()
            pass
        except KeyboardInterrupt:
            GPIO.cleanup()
            sys.exit()

   

if __name__ == '__main__':
    main()
