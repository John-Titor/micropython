// VRef generator
//

#include "cc16.h"

typedef struct _cc16_vref_obj_t {
    mp_obj_base_t   base;           // object type
} cc16_vref_obj_t;

static const cc16_vref_obj_t cc16_vref_obj = {{&cc16_vref_type}};

enum {
    VREF_NONE,
    VREF_5V,
    VREF_8V5,
    VREF_10V,
};

void cc16_vref_configure(void) {

    // configure pins
    cc16_pin_configure(DCDC_8V5);
    cc16_pin_configure(DCDC_10V);
    cc16_pin_configure(DI_PGD);
    cc16_pin_configure(DO_VREF_EN);
    cc16_pin_configure(DI_AI_VREF);
}

static void cc16_vref_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "Vref");
}

static mp_obj_t cc16_vref_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    return (mp_obj_t)&cc16_vref_obj;
}


mp_obj_t cc16_vref_set_voltage(mp_obj_t self_in, mp_obj_t voltage_in) {
    (void)self_in;
    switch (mp_obj_get_int(voltage_in)) {
    case VREF_NONE:
        cc16_pin_set(DO_VREF_EN, false);
        break;
    case VREF_5V:
        cc16_pin_set(DO_VREF_EN, true);
        cc16_pin_set(DCDC_8V5, false);
        cc16_pin_set(DCDC_10V, false);
        break;
    case VREF_8V5:
        cc16_pin_set(DO_VREF_EN, true);
        cc16_pin_set(DCDC_8V5, true);
        cc16_pin_set(DCDC_10V, false);
        break;
    case VREF_10V:
        cc16_pin_set(DO_VREF_EN, true);
        cc16_pin_set(DCDC_8V5, false);
        cc16_pin_set(DCDC_10V, true);
        break;
    default:
        mp_raise_ValueError(MP_ERROR_TEXT("invalid Vref value"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(cc16_vref_set_voltage_obj, cc16_vref_set_voltage);

static const mp_rom_map_elem_t cc16_vref_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_set_voltage), MP_ROM_PTR(&cc16_vref_set_voltage_obj) },
//    { MP_ROM_QSTR(MP_QSTR_get_voltage), MP_ROM_PTR(&cc16_vref_get_voltage_obj) },
//    { MP_ROM_QSTR(MP_QSTR_get_status), MP_ROM_PTR(&cc16_vref_get_status_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_VREF_NONE), MP_ROM_INT(VREF_NONE)},
    { MP_ROM_QSTR(MP_QSTR_VREF_5V), MP_ROM_INT(VREF_5V)},
    { MP_ROM_QSTR(MP_QSTR_VREF_8V5), MP_ROM_INT(VREF_8V5)},
    { MP_ROM_QSTR(MP_QSTR_VREF_10V), MP_ROM_INT(VREF_10V)},
};

static MP_DEFINE_CONST_DICT(cc16_vref_locals_dict, cc16_vref_locals_dict_table);

const mp_obj_type_t cc16_vref_type = {
    { & mp_type_type },
    .name = MP_QSTR_Vref,
    .print = cc16_vref_print,
    .make_new = cc16_vref_make_new,
    .locals_dict = (mp_obj_t)&cc16_vref_locals_dict,
};