#
# MRS CC16 glue
#

import umachine

# External OUTx pins
#
# set_mode(MODE_DIGITAL | MODE_PWM | MODE_ANALOG_IN)
# on()
# off()
# toggle()
# set_duty()
# voltage()
# current()
# status()
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
#
POWER = umachine.Output(8)      # force power on

# External INx pins
#
# get()
# voltage()
# set_mode(MODE_ANALOG | MODE_DIGITAL)
# set_pull(PULL_NONE | PULL_UP | PULL_DOWN)
# set_range(RANGE_16V | RANGE_32V)
#
IN0 = umachine.Input(0)
IN1 = umachine.Input(1)
IN2 = umachine.Input(2)
IN3 = umachine.Input(3)
IN4 = umachine.Input(4)
IN5 = umachine.Input(5)

# External analog signals
#
# get()
# voltage()
# set_mode(MODE_ANALOG | MODE_DIGITAL)
# set_pull(PULL_NONE | PULL_UP | PULL_DOWN)
#
ID = umachine.Input(6)
KL30_1 = umachine.Input(7)      # requires POWER set
KL30_2 = umachine.Input(8)      # requires POWER set

# External digital signals
#
# get()
# set_pull(PULL_NONE | PULL_UP | PULL_DOWN)
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
