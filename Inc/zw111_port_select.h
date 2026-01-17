/*
 * @file zw111_port_select.c
 *
 * @date 27 thg 12, 2025
 * @author LuongHuuPhuc
 *
 * File header lua chon Port giao tiep tuy theo tung Board
 * (Thu vien mo rong cho nhieu Board co the dung)
 */

#ifndef ZW111_LIB_INC_ZW111_PORT_SELECT_H_
#define ZW111_LIB_INC_ZW111_PORT_SELECT_H_

#if defined(EFR32_PLATFORM)
#include "Port/zw111_port_efr32.h"

#elif defined(STM32_PLATFORM)
#include "Port/zw111_port_stm32.h"

#elif defined(ESP32_PLATFORM)
#include "Port/zw111_port_esp32.h"

#else
/* Neu PORT chua duoc define */
#error "ZW11 port not selected (unknown platform)"
#endif // PLATFORM_CONFIG

#endif // ZW111_LIB_INC_ZW111_PORT_SELECT_H_
