/*
 * @file zw111_port_efr23.h
 *
 * @date 27 thg 12, 2025
 * @author LuongHuuPhuc
 *
 * File local danh cho Platform/Port EFR32
 * Chua cac API rieng biet cua Platform do de xu ly khoi tao phan cung UART
 */

#ifndef ZW111_LIB_INC_PORT_ZW111_PORT_EFR32_H_
#define ZW111_LIB_INC_PORT_ZW111_PORT_EFR32_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "../zw111_port.h"

#if defined(EFR32_PLATFORM)

//#define USER_PORT_UART_INIT      1

#ifndef SL_PORT_UART_INIT
#define SL_PORT_UART_INIT     1
#endif // SL_PORT_UART_INIT

/* Struct config UART rieng cua EFR32 Platform */
typedef struct {
  /* ID + trang thai cua phan cung UART sau khi da init */
  /* Note: handle la con tro */
  UARTDRV_Handle_t handle;

  /* Cau truc chua cau hinh phan cung de khoi tao UART */
  UARTDRV_InitUart_t init;

  /* Flag luu trang thai khoi tao cua UART (1 = initialized) */
  bool isUART_init;
} zw111_port_efr32_cfg_t;


// ============= SPECIFIC PORT/PLATFORM PROTOTYPE FUNCTION =============

/**
 * @brief Ham de gan handle UARTDRV sau khi UARTDRV_Init khi dinh nghia xong o dau do (Optional API)
 * (Co the la app.c)
 *
 * @param handle Bien luu cau truc phan cung UART
 */
EFR32_PLATFORM_TAG bool zw111_port_efr32_set_handle(UARTDRV_Handle_t uart_handle);

#ifdef USER_PORT_UART_INIT
/**
 * @brief Helper set default configuration init (tuy du an ma USER muon fill cu the) (Optional)
 * @param[in] cfg Con tro den cau truc cau hinh phan cung UART cua EFR32
 *
 * @note USART hien tai mac dinh dung trong ham nay la **USART1**
 */
EFR32_PLATFORM_TAG void zw111_port_efr32_default_cfg(zw111_port_efr32_cfg_t *cfg);

#endif //  USER_PORT_UART_INIT

#endif // EFR32_PLATFORM

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ZW111_LIB_INC_PORT_ZW111_PORT_EFR32_H_ */
