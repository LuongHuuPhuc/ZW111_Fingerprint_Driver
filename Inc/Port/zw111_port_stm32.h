/*
 * @file zw111_port_stm23.h
 *
 * @date 27 thg 12, 2025
 * @author LuongHuuPhuc
 *
 * File local danh cho Platform/Port STM32
 * Chua cac API rieng biet cua Platform do de xu ly khoi tao phan cung UART
 */

#ifndef ZW111_LIB_INC_PORT_ZW111_PORT_STM32_H_
#define ZW111_LIB_INC_PORT_ZW111_PORT_STM32_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "../zw111_port.h"

#if defined(STM32_PLATFORM)
#include "main.h"

typedef struct {
  UART_HandleTypeDef *huart;
} zw111_port_stm32_cfg_t;

/* Khoi tao uart truoc (o file khac) roi gan bien nay cho no */
extern __attribute__()

// ============= SPECIFIC PORT/PLATFORM PROTOTYPE FUNCTION =============

#endif // STM32_PLATFORM

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ZW111_LIB_INC_PORT_ZW111_PORT_STM32_H_ */
