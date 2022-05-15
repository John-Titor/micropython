
#include <stdint.h>

#include "py/runtime.h"
#include "extmod/machine_mem.h"
#include "cc16.h"
#include "modmachine.h"

typedef struct {
    uint16_t id;
    uint32_t serial_number;
    uint32_t device_number;
    uint8_t part_number[20u];
    uint8_t drawing_number[20u];
    uint8_t name[20u];
    uint8_t order_number[20u];
    uint8_t test_date[12u];
    uint16_t mcu_type;
    uint8_t hw_version[2u];
} __attribute__((packed)) MRS_factory_data_t;

// static const MRS_factory_data_t  *factory_data = (const MRS_factory_data_t *)0x14000002;

// extern const mp_obj_type_t machine_pin_type;

STATIC mp_obj_t machine_reset(void) {
    NVIC_SystemReset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

STATIC mp_obj_t machine_freq(void) {
    return MP_OBJ_NEW_SMALL_INT(80000000);  // Constant unless HSRUN is implemented
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_freq_obj, machine_freq);

STATIC mp_obj_t machine_unique_id(void) {

    // map the SIM UID registers directly
    return mp_obj_new_bytes((byte *)&(SIM->UIDH), 16);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_unique_id_obj, machine_unique_id);

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_umachine) },
    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&machine_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq),                MP_ROM_PTR(&machine_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem8),                MP_ROM_PTR(&machine_mem8_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem16),               MP_ROM_PTR(&machine_mem16_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem32),               MP_ROM_PTR(&machine_mem32_obj) },
    { MP_ROM_QSTR(MP_QSTR_unique_id),           MP_ROM_PTR(&machine_unique_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_Output),              MP_ROM_PTR(&cc16_output_type) },
    { MP_ROM_QSTR(MP_QSTR_Input),               MP_ROM_PTR(&cc16_input_type) },
    { MP_ROM_QSTR(MP_QSTR_Vref),                MP_ROM_PTR(&cc16_vref_type) },
};
STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&machine_module_globals,
};
