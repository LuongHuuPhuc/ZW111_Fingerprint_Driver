/*
 * @file zw111_port.h
 *
 * @date 27 thg 12, 2025
 * @author LuongHuuPhuc
 *
 * - File nay chua cac API interface hardware cua cac MCU (thap hon lowlevel)
 * - Cong (Port) phan cung - driver chung cho cac MCU khac nhau
 * - Chua cac API thao tac thap nhat ma moi MCU deu co (primitive)
 * - Cac prototype se duoc dinh nghia/implementation trong cac file MCU khac
 *
 * @note
 * Day la noi duy nhat duoc phep su dung va dinh nghia cac API khoi tao va giao tiep phan cung
 *
 * @remark
 * Cach hoat dong giong tinh da hinh (Polymorphism) trong C++
 * Dinh nghia cac API duoi day su dung cac API low-level cua tung Platform
 */

#ifndef ZW111_LIB_INC_ZW111_PORT_H_
#define ZW111_LIB_INC_ZW111_PORT_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

// ============= COMMON PORT/PLATFORM PROTOTYPE FUNCTION =============

/**
 * @brief Khoi tao UART phan cung cho module cam bien (Muc Port/Platform-specific)
 *
 * @details
 * Ham nay cau hinh phan cung cho UART/USART (Baudrate, pin route, enable clock,...)
 * Phu thuoc vao Platform/MCU ma Implement khac nhau (do API duoc cung cap khac nhau)
 *
 * @param[in] baudrate Toc do Baud yeu cau (toi thieu la 56700 -> 115200,...)
 * @param port_cfg Con tro tro den cau hinh Platform can dung (EFR32/STM32/ESP32)
 * @param port_cfg_size Kich thuoc cau truc cua platform/port do
 *
 * @return true neu init thanh cong - false neu that bai (UART chua san sang/init loi)
 */
bool zw111_port_uart_init(uint32_t baudrate, const void *port_cfg, uint32_t port_cfg_size);

/**
 * @brief Huy khoi tao phan cung UART
 * @details
 * Tuy vao platform/MCU, ham nay co the huy khoi tao hoan toan UART phan cung
 * hoac detach handle hay abort cac transfer
 */
bool zw111_port_uart_deinit(void);

/**
 * @brief Primitive function de gui data USART
 * @param buf Buffer chua data can Transmit (De la `const` vi Driver chi doc, khong duoc sua du lieu)
 * @param len Chieu dai Buffer data (So bytes can nhan)
 *
 * @note
 * Lowlevel xay Packet roi truyen gui xuong Port
 * Port chi gui, khong duoc sua *buf
 * Compiler se bat loi neu co gia tri vo tinh ghi vao buf
 */
void zw111_port_uart_tx(const uint8_t *buf, uint16_t len);

/**
 * @brief Primitive function de nhan data USART
 * @param buf Buffer de ghi du lieu da nhan duoc vao
 * @param len Chieu dai Buffer data (So bytes can nhan)
 * @param timeout_ms Thoi gian cho toi da de nhan data
 * @return Chieu dai Payload nhan duoc
 */
uint32_t zw111_port_uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms);

/**
 * @brief Primitive function thoi gian cho - delay (ms)
 * @param ms Thoi gian yeu cau
 */
void zw111_port_delay_ms(uint32_t ms);

/**
 * @brief Ham lay tick he thong tai thoi diem hien tai
 * @return Gia tri tick tang don dieu theo thoi gian
 */
uint32_t zw111_port_get_ticks(void);

/**
 * @brief Flush(Xoa/Lam sach) hoac Abort(Huy bo) RX UART tai Port
 * @details Dung de Recover khi bi lech khung (framing) hoac desync giao thuc
 * No chu yeu dung 1 transaction Receive dang pending
 * Reset trang thai driver RX
 *
 * @return true - Neu
 */
bool zw111_port_uart_flush(void);

/**
 * @brief Kiem tra UART da san sang su dung hay chua (handle da duoc gan hay chua)
 * Thuong duoc goi truoc khi thuc thi cac lenh o Application Layer
 *
 * @return true neu san sang, false neu chua
 */
bool zw111_port_uart_ready(void);

/* --------------- HELPER FUNCTION --------------- */

/**
 * @brief Ham chuyen doi tu Ticks sang ms (xap xi)
 * @param ticks Gia tri ticks dau vao
 * @return Gia tri sau khi chuyen doi tu Ticks sang ms
 */
__attribute__((unused)) static inline uint32_t ticks_to_ms_approx(uint32_t ticks){

#if defined(EFR32_PLATFORM)

  /* Nhieu project dung sl_sleeptimer_get_tick_count() dua tren 32768Hz */
  /* 32768 ticks/s => ~32.768 ticks/ms */
  /* ms ≈ ticks * 1000 / 32768 */
  return (uint32_t)(((uint64_t)ticks * 1000u) / 32768u);

  // Hoac co the dung luon API chinh thuc cua SDK (neu co) sl_sleeptimer_tick_to_ms(ticks)

#endif // EFR32_PLATFORM

#if defined(STM32_PLATFORM)
  // ...
#endif // STM32_PLATFORM

#if defined(ESP32_PLATFORM)
  // ...
#endif // ESP32_PLATFORM
}

/**
 * @brief Tinh thoi gian da troi qua (ms) dua tren tick, co xy ly wrap-around
 * @param start_tick
 * @param now_tick
 * @return Thoi gian chenh lech (ms) 32-bit
 */
__attribute__((unused)) static inline uint32_t elapsed_ms(uint32_t start_tick, uint32_t now_tick){
  uint32_t dt = (uint32_t)(now_tick - start_tick);
  return ticks_to_ms_approx(dt);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ZW111_LIB_INC_ZW111_PORT_H_ */
