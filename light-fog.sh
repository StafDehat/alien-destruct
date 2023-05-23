#!/bin/bash
# Author: Andrew Howard


#############
# Constants #
#############
POWER_PIN=10
PUMP_PIN=11

PUMP_INTERVAL=180
PUMP_DURATION=5

DEBUG=true


###################
# Non-concurrency #
###################
logger "$0 ($$): Starting execution"
LOCK_FILE=/tmp/`basename $0`.lock
function cleanup {
 logger "$0 ($$): Caught exit signal - deleting trap file"
 rm -f $LOCK_FILE
 exit 2
}
logger "$0 ($$): Using lockfile ${LOCK_FILE}"
(set -C; echo "$$" > "${LOCK_FILE}") 2>/dev/null
if [ $? -ne 0 ]; then
 logger "$0 ($$): Lock File exists - exiting"
 exit 1
else
  trap 'cleanup' 1 2 15 19 23 EXIT
fi


####################
# Helper Functions #
####################
function debug() { 
  logger -t qwatch "$0: $@"
  if [[ "$DEBUG" != "true" ]]; then
    return 0
  fi
  local ts=$( date +"%F %T.%N" )
  echo "${ts}: ${@}" >&2
}

function gpio()
{
  local verb=$1
  local pin=$2
  local value=$3

  local pins=($GPIO_PINS)
  if [[ "$pin" -lt ${#pins[@]} ]]; then
    local pin=${pins[$pin]}
  fi

  local gpio_path=/sys/class/gpio
  local pin_path=$gpio_path/gpio$pin

  case $verb in
    read)
      cat $pin_path/value
    ;;

    write)
      echo $value > $pin_path/value
    ;;

    mode)
      if [[ ! -e $pin_path ]]; then
        echo $pin > $gpio_path/export
      fi
      if [[ "$( cat $pin_path/direction )" != "$value" ]]; then
        echo $value > $pin_path/direction
      fi
    ;;

    state)
      if [ -e $pin_path ]; then
        local dir=$(cat $pin_path/direction)
        local val=$(cat $pin_path/value)
        echo "$dir $val"
      fi
    ;;

    *)
      echo "Control the GPIO pins of the Raspberry Pi"
      echo "Usage: $0 mode [pin] [in|out]"
      echo "       $0 read [pin]"
      echo "       $0 write [pin] [0|1]"
      echo "       $0 state [pin]"
      echo "If GPIO_PINS is an environment variable containing"
      echo "a space-delimited list of integers, then up to 17"
      echo "logical pins (0-16) will map to the physical pins"
      echo "specified in the list."
    ;;
  esac
}

function powerOn() {
  gpio mode ${POWER_PIN} out
  gpio write ${POWER_PIN} 1
}

function pumpOn() {
  gpio mode ${PUMP_PIN} out
  gpio write ${PUMP_PIN} 1
}

function pumpOff() {
  gpio mode ${PUMP_PIN} out
  gpio write ${PUMP_PIN} 0
}


########
# MAIN #
########
# Power-up the IoT relay immediately
# This will turn on the lights and start warming-up the fogger
powerOn

# Run pump for ${PUMP_DURATION} seconds, every ${PUMP_INTERVAL} seconds.
# Repeat forever (until Pi is shutoff)
while true; do
  debug "Waiting ${PUMP_INTERVAL}s before initiating pump"
  sleep ${PUMP_INTERVAL}
  debug "Activating pump"
  pumpOn
  debug "Leaving pump on for ${PUMP_DURATION}s"
  sleep ${PUMP_DURATION}
  debug "Deactivating pump"
  pumpOff
done

