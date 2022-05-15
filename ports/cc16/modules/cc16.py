#
# MRS CC16 glue
#

import umachine

# External OUTx pins
#
# mode(MODE_DIGITAL | MODE_PWM | MODE_ANALOG_IN)
#   Select the output operating mode.
#
# on()
# off()
# toggle()
#   Set output on/off/toggle
#
# duty()
#   Set the PWM duty cycle (0-100%); MODE_PWM only.
#
# voltage()
#   Returns the instantaneous pin voltage in mV; works in MODE_DIGITAL
#   or MODE_ANALOG_IN only; in MODE_PWM may give misleading results.
#
# current()
#   Read the instantaneous pin current in mA; works in MODE_DIGITAL
#   only; in MODE_PWM will give misleading results.
#
# status()
#   TBD
#
OUT0 = umachine.Output(0)
OUT1 = umachine.Output(1)
OUT2 = umachine.Output(2)
OUT3 = umachine.Output(3)
OUT4 = umachine.Output(4)
OUT5 = umachine.Output(5)
OUT6 = umachine.Output(6)
OUT7 = umachine.Output(7)

# Internal digital outputs
#
# on()
# off()
# toggle()
#   Set output on/off/toggle.
#
POWER = umachine.Output(8)      # force power on

# External INx pins
#
# mode(MODE_ANALOG | MODE_DIGITAL)
#   Select analog or digital operating mode.
#
# get()
#   Returns 0/1 based on input logic level; MODE_DIGITAL only.
#
# voltage()
#   Returns the instantaneous pin voltage in mV.
#
# pull(PULL_NONE | PULL_UP | PULL_DOWN)
#   Configures the external pull up/down resistors.
#
# range(RANGE_16V | RANGE_32V)
#   Selects 16V or 32V maximum measurement scale.
#
IN0 = umachine.Input(0)
IN1 = umachine.Input(1)
IN2 = umachine.Input(2)
IN3 = umachine.Input(3)
IN4 = umachine.Input(4)
IN5 = umachine.Input(5)

# External analog signals
#
# mode(MODE_ANALOG_IN | MODE_DIGITAL)
#   Select analog or digital operating mode.
#
#   Returns 0/1 based on input logic level; MODE_DIGITAL only.
#
# voltage()
#   Returns the instantaneous pin voltage in mV.
#
# pull(PULL_NONE | PULL_UP | PULL_DOWN)
#   Configures the internal pull up/down resistors; MODE_DIGIAL
#   only.
#
ID = umachine.Input(6)
KL30_1 = umachine.Input(7)      # requires POWER set
KL30_2 = umachine.Input(8)      # requires POWER set

# External digital signals
#
# get()
# pull(PULL_NONE | PULL_UP | PULL_DOWN)
#
KL15 = umachine.Input(9)
INTERFACE2_A = umachine.Input(10)
INTERFACE2_B = umachine.Input(11)

# Vref generator
#
# Note that there should be an 8.5V setting, but it does not
# appear to work.
#
# set(VREF_NONE | VREF_5V | VREF_7V4 | VREF_10V)
# voltage()
#
Vref = umachine.Vref()
