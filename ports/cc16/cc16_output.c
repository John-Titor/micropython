// High-side drivers
//

#include "cc16.h"

typedef struct _cc16_output_obj_t {
    mp_obj_base_t base;
    Pin_t digital_pin;
    Pin_t pwm_pin;
    Pin_t voltage_pin;
    Pin_t current_pin;
    Pin_t sense_pin;
    Pin_t sense_mux;
} cc16_output_obj_t;

static const cc16_output_obj_t cc16_output_obj[] = {
    {{&cc16_output_type}, DO_HSD1_OUT0, PWM_HSD1_OUT0, DI_AI_OUT0, DI_AI_INA_OUT0, DI_AI_SNS1, DO_CS_HSD1},
    {{&cc16_output_type}, DO_HSD1_OUT1, PWM_HSD1_OUT1, DI_AI_OUT1, DI_AI_INA_OUT1, DI_AI_SNS2, DO_CS_HSD1},
    {{&cc16_output_type}, DO_HSD1_OUT2, PWM_HSD1_OUT2, DI_AI_OUT2, DI_AI_INA_OUT2, DI_AI_SNS3, DO_CS_HSD1},
    {{&cc16_output_type}, DO_HSD1_OUT3, PWM_HSD1_OUT3, DI_AI_OUT3, DI_AI_INA_OUT3, DI_AI_SNS4, DO_CS_HSD1},
    {{&cc16_output_type}, DO_HSD2_OUT4, PWM_HSD2_OUT4, DI_AI_OUT4, DI_AI_INA_OUT4, DI_AI_SNS1, DO_CS_HSD2},
    {{&cc16_output_type}, DO_HSD2_OUT5, PWM_HSD2_OUT5, DI_AI_OUT5, DI_AI_INA_OUT5, DI_AI_SNS2, DO_CS_HSD2},
    {{&cc16_output_type}, DO_HSD2_OUT6, PWM_HSD2_OUT6, DI_AI_OUT6, DI_AI_INA_OUT6, DI_AI_SNS3, DO_CS_HSD2},
    {{&cc16_output_type}, DO_HSD2_OUT7, PWM_HSD2_OUT7, DI_AI_OUT7, DI_AI_INA_OUT7, DI_AI_SNS4, DO_CS_HSD2},
    {{&cc16_output_type}, DO_POWER,     PIN_NONE,      PIN_NONE,   PIN_NONE,       PIN_NONE,   PIN_NONE},
};

enum {
    OUTPUT_MODE_DIGITAL,
    OUTPUT_MODE_PWM,
    OUTPUT_MODE_ANALOG_IN,
};

#define NUM_OUTPUT MP_ARRAY_SIZE(cc16_output_obj)
#define OUTPUT_ID(obj) ((obj) - &cc16_output_obj[0])

void cc16_output_configure(void) {

    // configure pins for all outputs (digital mode)
    //
    for (int i = 0; i < NUM_OUTPUT; i++) {
        cc16_pin_configure(cc16_output_obj[i].digital_pin);
        cc16_pin_configure(cc16_output_obj[i].voltage_pin);
        cc16_pin_configure(cc16_output_obj[i].current_pin);
        cc16_pin_configure(cc16_output_obj[i].sense_pin);
        cc16_pin_configure(cc16_output_obj[i].sense_mux);
    }
}

static void cc16_output_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cc16_output_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Output%u", OUTPUT_ID(self));
}

static mp_obj_t cc16_output_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_int_t output_id = mp_obj_get_int(args[0]);
    if (!(0 <= output_id && output_id < NUM_OUTPUT)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Output%d does not exist"), output_id);
    }
    return (mp_obj_t)&cc16_output_obj[output_id];
}

static mp_obj_t cc16_output_mode(mp_obj_t self_in, mp_obj_t mode_in) {
    cc16_output_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (mp_obj_get_int(mode_in)) {
        case OUTPUT_MODE_DIGITAL:
            cc16_pin_configure(self->digital_pin);
            break;
        case OUTPUT_MODE_PWM:
            if (cc16_pin_is_none(self->pwm_pin)) {
                mp_raise_ValueError(MP_ERROR_TEXT("pin does not support PWM"));
            } else {
                cc16_pin_configure(self->pwm_pin);
                // XXX set base duty cycle
                // XXX enable PWM output from FTM
            }
            break;
        case OUTPUT_MODE_ANALOG_IN:
            if (cc16_pin_is_none(self->voltage_pin)) {
                mp_raise_ValueError(MP_ERROR_TEXT("pin does not support analog input"));
            } else {
                cc16_pin_configure(self->digital_pin);
                cc16_pin_set(self->digital_pin, false);
            }
            break;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc16_output_mode_obj, cc16_output_mode);

static mp_obj_t cc16_output_on(mp_obj_t self_in) {
    cc16_output_obj_t *self = MP_OBJ_TO_PTR(self_in);
    cc16_pin_set(self->digital_pin, true);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_output_on_obj, cc16_output_on);

static mp_obj_t cc16_output_off(mp_obj_t self_in) {
    cc16_output_obj_t *self = MP_OBJ_TO_PTR(self_in);
    cc16_pin_set(self->digital_pin, false);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_output_off_obj, cc16_output_off);

static mp_obj_t cc16_output_toggle(mp_obj_t self_in) {
    cc16_output_obj_t *self = MP_OBJ_TO_PTR(self_in);
    cc16_pin_toggle(self->digital_pin);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_output_toggle_obj, cc16_output_toggle);

static mp_obj_t cc16_output_duty(mp_obj_t self_in, mp_obj_t duty_in) {
    cc16_output_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!cc16_pin_is_none(self->pwm_pin)) {
        cc16_ftm_set_pwm(self->pwm_pin, mp_obj_get_int(duty_in));
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("pin does not support PWM"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc16_output_duty_obj, cc16_output_duty);

static mp_obj_t cc16_output_voltage(mp_obj_t self_in) {
    cc16_output_obj_t *self = self_in;
    if (!cc16_pin_is_none(self->voltage_pin)) {
        uint32_t sample = cc16_adc_sample(self->voltage_pin);
        uint32_t fsd = 39340;
        uint32_t mV = (sample * fsd) / 4096;
        return MP_OBJ_NEW_SMALL_INT(mV);
    }
    mp_raise_ValueError(MP_ERROR_TEXT("pin does not support voltage measurement"));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_output_voltage_obj, cc16_output_voltage);

static mp_obj_t cc16_output_current(mp_obj_t self_in) {
    cc16_output_obj_t *self = self_in;
    if (!cc16_pin_is_none(self->current_pin)) {
        uint32_t sample = cc16_adc_sample(self->current_pin);
        uint32_t fsd = 5000;
        uint32_t mA = (sample * fsd) / 4096;
        return MP_OBJ_NEW_SMALL_INT(mA);
    }
    mp_raise_ValueError(MP_ERROR_TEXT("pin does not support current measurement"));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_output_current_obj, cc16_output_current);

static const mp_rom_map_elem_t cc16_output_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&cc16_output_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&cc16_output_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&cc16_output_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&cc16_output_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&cc16_output_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_voltage), MP_ROM_PTR(&cc16_output_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_current), MP_ROM_PTR(&cc16_output_current_obj) },
//    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&cc16_output_status_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_MODE_DIGITAL), MP_ROM_INT(OUTPUT_MODE_DIGITAL)},
    { MP_ROM_QSTR(MP_QSTR_MODE_PWM), MP_ROM_INT(OUTPUT_MODE_PWM)},
    { MP_ROM_QSTR(MP_QSTR_MODE_ANALOG), MP_ROM_INT(OUTPUT_MODE_ANALOG_IN)},
};

static MP_DEFINE_CONST_DICT(cc16_output_locals_dict, cc16_output_locals_dict_table);

const mp_obj_type_t cc16_output_type = {
    { &mp_type_type },
    .name = MP_QSTR_Output,
    .print = cc16_output_print,
    .make_new = cc16_output_make_new,
    .locals_dict = (mp_obj_t)&cc16_output_locals_dict,
};
