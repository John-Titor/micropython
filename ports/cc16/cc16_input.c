// External input pins
//

#include "cc16.h"

typedef struct _cc16_input_obj_t {
    mp_obj_base_t base;             // object type
    Pin_t digital_pin;              // input pin in digital mode
    Pin_t analog_pin;               // input pin in analog mode
    Pin_t pullup_pin;               // pin switching external pull-up
    Pin_t pulldown_pin;             // pin switching external pull-down
    Pin_t rangesel_pin;             // analog range select pin
    uint16_t vrefl;                 // low-range analog full scale
    uint16_t vrefh;                 // high-range analog full scale
} cc16_input_obj_t;

static const cc16_input_obj_t cc16_input_obj[] = {
    // external INx pins
    {{&cc16_input_type}, DI_IN0,            AI_IN0,     PU_A_IN0, PD_A_IN0, DO_RS0, 16920, 32250},
    {{&cc16_input_type}, DI_IN1,            AI_IN1,     PU_A_IN1, PD_A_IN1, DO_RS1, 16920, 32250},
    {{&cc16_input_type}, DI_IN2,            AI_IN2,     PU_A_IN2, PD_A_IN2, DO_RS2, 16920, 32250},
    {{&cc16_input_type}, DI_IN3,            AI_IN3,     PU_A_IN3, PD_A_IN3, DO_RS3, 16920, 32250},
    {{&cc16_input_type}, DI_IN4,            AI_IN4,     PU_A_IN4, PD_A_IN4, DO_RS4, 16920, 32250},
    {{&cc16_input_type}, DI_IN5,            AI_IN5,     PU_A_IN5, PD_A_IN5, DO_RS5, 16920, 32250},
    // external analog signals
    {{&cc16_input_type}, DI_ID,             AI_ID,      PIN_NONE, PIN_NONE, PIN_NONE, 16920, 0},
    {{&cc16_input_type}, AI_KL30_1,         AI_KL30_1,  PIN_NONE, PIN_NONE, PIN_NONE, 39000, 0},
    {{&cc16_input_type}, AI_KL30_2,         AI_KL30_2,  PIN_NONE, PIN_NONE, PIN_NONE, 39000, 0},
    // external digital signals
    {{&cc16_input_type}, DI_KL15,           PIN_NONE,   PIN_NONE, PIN_NONE, PIN_NONE, 0, 0},
    {{&cc16_input_type}, DI_INTERFACE2_A,   PIN_NONE,   PIN_NONE, PIN_NONE, PIN_NONE, 0, 0},
    {{&cc16_input_type}, DI_INTERFACE2_B,   PIN_NONE,   PIN_NONE, PIN_NONE, PIN_NONE, 0, 0},
};

enum {
    INPUT_PULL_NONE = Pin_PullNone,
    INPUT_PULL_UP   = Pin_PullUp,
    INPUT_PULL_DOWN = Pin_PullDown,
};

enum {
    INPUT_MODE_ANALOG,
    INPUT_MODE_DIGITAL,
};

enum {
    INPUT_RANGE_16V,
    INPUT_RANGE_32V,
};

#define NUM_INPUT MP_ARRAY_SIZE(cc16_input_obj)
#define INPUT_ID(obj) ((obj) - &cc16_input_obj[0])

void cc16_input_configure(void) {

    // configure all pins (digital mode)
    for (int i = 0; i < NUM_INPUT; i++) {
        cc16_pin_configure(cc16_input_obj[i].digital_pin);
        cc16_pin_configure(cc16_input_obj[i].pullup_pin);
        cc16_pin_configure(cc16_input_obj[i].pulldown_pin);
        cc16_pin_configure(cc16_input_obj[i].rangesel_pin);
    }
}

static void cc16_input_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cc16_input_obj_t *self = self_in;
    mp_printf(print, "Input%u", INPUT_ID(self));
}

static mp_obj_t cc16_input_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_int_t input_id = mp_obj_get_int(args[0]);
    if (!(0 <= input_id && input_id < NUM_INPUT)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("IN%d does not exist"), input_id);
    }
    return (mp_obj_t)&cc16_input_obj[input_id];
}

// fast method for getting pin value
static mp_obj_t cc16_input_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    cc16_input_obj_t *self = self_in;
    if (n_args == 0) {
        // get pin
        return MP_OBJ_NEW_SMALL_INT(cc16_pin_get(self->digital_pin));
    } else {
        return mp_const_none;
    }
}

static mp_obj_t cc16_input_get(mp_obj_t self_in) {
    cc16_input_obj_t *self = self_in;
    return MP_OBJ_NEW_SMALL_INT(cc16_pin_get(self->digital_pin));
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_input_get_obj, cc16_input_get);

static mp_obj_t cc16_input_voltage(mp_obj_t self_in) {
    cc16_input_obj_t *self = self_in;
    if (!cc16_pin_is_none(self->analog_pin)) {
        uint32_t sample = cc16_adc_sample(self->analog_pin);
        uint32_t fsd = (!cc16_pin_is_none(self->rangesel_pin) &&
            cc16_pin_get(self->rangesel_pin)) ? self->vrefh : self->vrefl;
        uint32_t mv = (sample * fsd) / 4096;
        return MP_OBJ_NEW_SMALL_INT(mv);
    }
    mp_raise_ValueError(MP_ERROR_TEXT("pin does not support analog input"));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_input_voltage_obj, cc16_input_voltage);

static mp_obj_t cc16_input_pull(mp_obj_t self_in, mp_obj_t pull_in) {
    cc16_input_obj_t *self = self_in;
    if (!cc16_pin_is_none(self->pullup_pin)) {
        switch (mp_obj_get_int(pull_in)) {
            case INPUT_PULL_NONE:
                cc16_pin_set(self->pullup_pin, false);
                cc16_pin_set(self->pulldown_pin, false);
                break;
            case INPUT_PULL_UP:
                cc16_pin_set(self->pulldown_pin, false);
                cc16_pin_set(self->pullup_pin, true);
                break;
            case INPUT_PULL_DOWN:
                cc16_pin_set(self->pullup_pin, false);
                cc16_pin_set(self->pulldown_pin, true);
                break;
            default:
                mp_raise_ValueError(MP_ERROR_TEXT("invalid pull value"));
        }
    } else if (!cc16_pin_is_none(self->digital_pin)) {
        Pin_t dpin = self->digital_pin;
        switch (mp_obj_get_int(pull_in)) {
            case INPUT_PULL_NONE:
                dpin.pull = Pin_PullNone;
                break;
            case INPUT_PULL_UP:
                dpin.pull = Pin_PullUp;
                break;
            case INPUT_PULL_DOWN:
                dpin.pull = Pin_PullDown;
                break;
            default:
                mp_raise_ValueError(MP_ERROR_TEXT("invalid pull value"));
        }
        cc16_pin_configure(dpin);
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("pin does not support pull change"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc16_input_pull_obj, cc16_input_pull);

static mp_obj_t cc16_input_mode(mp_obj_t self_in, mp_obj_t mode_in) {
    cc16_input_obj_t *self = self_in;

    switch (mp_obj_get_int(mode_in)) {
        case INPUT_MODE_ANALOG:
            if (cc16_pin_is_none(self->analog_pin)) {
                mp_raise_ValueError(MP_ERROR_TEXT("pin does not support analog mode"));
            } else {
                cc16_pin_configure(self->analog_pin);
            }
            break;
        case INPUT_MODE_DIGITAL:
            if (cc16_pin_is_none(self->digital_pin)) {
                mp_raise_ValueError(MP_ERROR_TEXT("pin does not support digital mode"));
            } else {
                cc16_pin_configure(self->digital_pin);
            }
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid mode value"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc16_input_mode_obj, cc16_input_mode);

static mp_obj_t cc16_input_range(mp_obj_t self_in, mp_obj_t range_in) {
    cc16_input_obj_t *self = self_in;
    if (cc16_pin_is_none(self->rangesel_pin)) {
        mp_raise_ValueError("pin does not support range change");
    } else {
        switch (mp_obj_get_int(range_in)) {
            case INPUT_RANGE_16V:
                cc16_pin_set(self->rangesel_pin, false);
                break;
            case INPUT_RANGE_32V:
                cc16_pin_set(self->rangesel_pin, true);
                break;
            default:
                mp_raise_ValueError(MP_ERROR_TEXT("invalid range value"));
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc16_input_range_obj, cc16_input_range);

static const mp_rom_map_elem_t cc16_input_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&cc16_input_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_voltage), MP_ROM_PTR(&cc16_input_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&cc16_input_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_pull), MP_ROM_PTR(&cc16_input_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_range), MP_ROM_PTR(&cc16_input_range_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(INPUT_PULL_NONE)},
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(INPUT_PULL_UP)},
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(INPUT_PULL_DOWN)},
    { MP_ROM_QSTR(MP_QSTR_MODE_ANALOG_IN), MP_ROM_INT(INPUT_MODE_ANALOG)},
    { MP_ROM_QSTR(MP_QSTR_MODE_DIGITAL), MP_ROM_INT(INPUT_MODE_DIGITAL)},
    { MP_ROM_QSTR(MP_QSTR_RANGE_16V), MP_ROM_INT(INPUT_RANGE_16V)},
    { MP_ROM_QSTR(MP_QSTR_RANGE_32V), MP_ROM_INT(INPUT_RANGE_32V)},
};

static MP_DEFINE_CONST_DICT(cc16_input_locals_dict, cc16_input_locals_dict_table);

const mp_obj_type_t cc16_input_type = {
    { &mp_type_type },
    .name = MP_QSTR_Input,
    .print = cc16_input_print,
    .make_new = cc16_input_make_new,
    .call = cc16_input_call,
    .locals_dict = (mp_obj_t)&cc16_input_locals_dict,
};
