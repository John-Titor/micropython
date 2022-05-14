// High-side drivers
//

#include "py/runtime.h"

#include "cc16.h"
#include "cc16_pin.h"

typedef struct _cc16_hsd_obj_t {
    mp_obj_base_t   base;
    Pin_t           output_pin;
    Pin_t           vin_pin;
    Pin_t           iout_pin;
    Pin_t           sense_pin;
    Pin_t           sense_mux;
} cc16_hsd_obj_t;

static const cc16_hsd_obj_t cc16_hsd_obj[] = {
    {{&cc16_hsd_type}, DO_HSD1_OUT0, DI_AI_OUT0, DI_AI_INA_OUT0, DI_AI_SNS1, DO_CS_HSD1},
    {{&cc16_hsd_type}, DO_HSD1_OUT1, DI_AI_OUT1, DI_AI_INA_OUT1, DI_AI_SNS2, DO_CS_HSD1},
    {{&cc16_hsd_type}, DO_HSD1_OUT2, DI_AI_OUT2, DI_AI_INA_OUT2, DI_AI_SNS3, DO_CS_HSD1},
    {{&cc16_hsd_type}, DO_HSD1_OUT3, DI_AI_OUT3, DI_AI_INA_OUT3, DI_AI_SNS4, DO_CS_HSD1},
    {{&cc16_hsd_type}, DO_HSD2_OUT4, DI_AI_OUT4, DI_AI_INA_OUT4, DI_AI_SNS1, DO_CS_HSD2},
    {{&cc16_hsd_type}, DO_HSD2_OUT5, DI_AI_OUT5, DI_AI_INA_OUT5, DI_AI_SNS2, DO_CS_HSD2},
    {{&cc16_hsd_type}, DO_HSD2_OUT6, DI_AI_OUT6, DI_AI_INA_OUT6, DI_AI_SNS3, DO_CS_HSD2},
    {{&cc16_hsd_type}, DO_HSD2_OUT7, DI_AI_OUT7, DI_AI_INA_OUT7, DI_AI_SNS4, DO_CS_HSD2},
};

#define NUM_HSD MP_ARRAY_SIZE(cc16_hsd_obj)
#define HSD_ID(obj) ((obj) - &cc16_hsd_obj[0])

static void cc16_hsd_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cc16_hsd_obj_t *self = self_in;
    mp_printf(print, "OUT%u", HSD_ID(self));
}

static mp_obj_t cc16_hsd_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_int_t hsd_id = mp_obj_get_int(args[0]);
    if (!(0 <= hsd_id && hsd_id < NUM_HSD)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("HSD%d does not exist"), hsd_id);
    }
    return (mp_obj_t)&cc16_hsd_obj[hsd_id];
}

mp_obj_t cc16_hsd_on(mp_obj_t self_in) {
    cc16_hsd_obj_t *self = self_in;
    cc16_pin_set(cc16_hsd_obj[HSD_ID(self)].output_pin, true);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_hsd_on_obj, cc16_hsd_on);

mp_obj_t cc16_hsd_off(mp_obj_t self_in) {
    cc16_hsd_obj_t *self = self_in;
    cc16_pin_set(cc16_hsd_obj[HSD_ID(self)].output_pin, false);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_hsd_off_obj, cc16_hsd_off);

mp_obj_t cc16_hsd_toggle(mp_obj_t self_in) {
    cc16_hsd_obj_t *self = self_in;
    cc16_pin_toggle(cc16_hsd_obj[HSD_ID(self)].output_pin);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_hsd_toggle_obj, cc16_hsd_toggle);

static const mp_rom_map_elem_t cc16_hsd_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&cc16_hsd_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&cc16_hsd_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&cc16_hsd_toggle_obj) },
};

static MP_DEFINE_CONST_DICT(cc16_hsd_locals_dict, cc16_hsd_locals_dict_table);

const mp_obj_type_t cc16_hsd_type = {
    { & mp_type_type },
    .name = MP_QSTR_HSD,
    .print = cc16_hsd_print,
    .make_new = cc16_hsd_make_new,
    .locals_dict = (mp_obj_t)&cc16_hsd_locals_dict,
};