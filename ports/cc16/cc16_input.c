// High-side drivers
//

#include "py/runtime.h"

#include "cc16.h"
#include "cc16_pin.h"

typedef struct _cc16_input_obj_t {
    mp_obj_base_t   base;
    Pin_t           input_pin;
    Pin_t           pullup_pin;
    Pin_t           pulldown_pin;
    Pin_t           rangesel_pin;
} cc16_input_obj_t;

static const cc16_input_obj_t cc16_input_obj[] = {
    // external INx pins
    {{&cc16_input_type}, DI_AI_A_IN0,       PU_A_IN0, PD_A_IN0, DO_RS0},
    {{&cc16_input_type}, DI_AI_A_IN1,       PU_A_IN1, PD_A_IN1, DO_RS1},
    {{&cc16_input_type}, DI_AI_A_IN2,       PU_A_IN2, PD_A_IN2, DO_RS2},
    {{&cc16_input_type}, DI_AI_A_IN3,       PU_A_IN3, PD_A_IN3, DO_RS3},
    {{&cc16_input_type}, DI_AI_A_IN4,       PU_A_IN4, PD_A_IN4, DO_RS4},
    {{&cc16_input_type}, DI_AI_A_IN5,       PU_A_IN5, PD_A_IN5, DO_RS5},
    // external analog signals
    {{&cc16_input_type}, DI_AI_ID,          PIN_NONE, PIN_NONE, PIN_NONE},
    {{&cc16_input_type}, DI_AI_KL30_1,      PIN_NONE, PIN_NONE, PIN_NONE},
    {{&cc16_input_type}, DI_AI_KL30_2,      PIN_NONE, PIN_NONE, PIN_NONE},
    // external digital signals
    {{&cc16_input_type}, DI_KL15,           PIN_NONE, PIN_NONE, PIN_NONE},
    {{&cc16_input_type}, DI_INTERFACE2_A,   PIN_NONE, PIN_NONE, PIN_NONE},
    {{&cc16_input_type}, DI_INTERFACE2_B,   PIN_NONE, PIN_NONE, PIN_NONE},
};

enum {
    INPUT_PULL_NONE,
    INPUT_PULL_UP,
    INPUT_PULL_DOWN,
};

#define NUM_INPUT MP_ARRAY_SIZE(cc16_input_obj)
#define INPUT_ID(obj) ((obj) - &cc16_input_obj[0])

static void cc16_input_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cc16_input_obj_t *self = self_in;
    mp_printf(print, "IN%u", INPUT_ID(self));
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
        return MP_OBJ_NEW_SMALL_INT(cc16_pin_get(self->input_pin));
    } else {
        return mp_const_none;
    }
}

mp_obj_t cc16_input_get(mp_obj_t self_in) {
    cc16_input_obj_t *self = self_in;
    return MP_OBJ_NEW_SMALL_INT(cc16_pin_get(self->input_pin));
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_input_get_obj, cc16_input_get);

mp_obj_t cc16_input_set_pull(mp_obj_t self_in, mp_obj_t pull_in) {
    cc16_input_obj_t *self = self_in;
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
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc16_input_set_pull_obj, cc16_input_set_pull);

static const mp_rom_map_elem_t cc16_input_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&cc16_input_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pull), MP_ROM_PTR(&cc16_input_set_pull_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(INPUT_PULL_NONE)},
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(INPUT_PULL_UP)},
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(INPUT_PULL_DOWN)},
};

static MP_DEFINE_CONST_DICT(cc16_input_locals_dict, cc16_input_locals_dict_table);

const mp_obj_type_t cc16_input_type = {
    { & mp_type_type },
    .name = MP_QSTR_HSD,
    .print = cc16_input_print,
    .make_new = cc16_input_make_new,
    .call = cc16_input_call,
    .locals_dict = (mp_obj_t)&cc16_input_locals_dict,
};