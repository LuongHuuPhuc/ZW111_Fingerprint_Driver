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
#include "stdarg.h"
#include "zw111_types.h"

/* Prefix giup de doc function noi bo cua Platform */
#if defined(EFR32_PLATFORM)
#include "uartdrv.h"
#include "sl_udelay.h"
#include "sl_sleeptimer.h"
#include "ecode.h"
#include "../../autogen/sl_uartdrv_instances.h"
#include "../../config/sl_uartdrv_usart_USART_UART2_config.h"
#define EFR32_PLATFORM_TAG

#elif defined(STM32_PLATFORM)
#define STM32_PLATFORM_TAG

#elif defined(ESP32_PLATFORM)
#define STM32_PLATFORM_TAG

#else
#endif // PLATFORM_CONFIG

/* Che do transaction cua giao thuc UART muon dung ^^ */
#define UART_NON_BLOCKING_MODE         1
//#define UART_BLOCKING_MODE             1

/* Level debug (de tranh spam ra man hinh) */
#ifndef ZW111_UART_LOG_DEBUG_LEVEL
#define ZW111_UART_LOG_DEBUG_LEVEL     1  // 0 = off, 1 = error, 2 = info, 3 = verbose
#endif  // ZW111_UART_LOG_DEBUG_LEVEL

#define DEBUG_LOG(lvl, fmt, ...) do { \
    if((lvl) <= ZW111_UART_LOG_DEBUG_LEVEL){ \
        printf(fmt, ##__VA_ARGS__);\
    }\
} while(0)

/* Struct luu trang thai giao dich (transaction) UART cua port */
typedef enum ZW111_PORT_UART_STATE {
  /* Khong co transaction dang chay  */
  UART_IDLE = 0,

  /* Transaction dang chay (TX/RX pending) */
  UART_BUSY,

  /* Transaction hoan tat thanh cong */
  UART_DONE,

  /* Transaction loi (Driver tra ve loi) */
  UART_ERROR,

  /* Transaction timeout (port layer chu dong abort RX) */
  UART_TIMEOUT,

  /* Abort transaction co chu dich (co du N byte yeu cau) */
  UART_ABORT_OK
} zw111_port_uart_state_t;

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
 * @brief Huy khoi tao phan cung UART o muc Platform/port
 *
 * @details
 * Tuy vao platform/MCU, ham nay co the huy khoi tao hoan toan UART phan cung
 * hoac detach handle hay abort cac transfer
 *
 * @return true neu deinit thanh cong; false neu that bai hoac chua init
 */
bool zw111_port_uart_deinit(void);

/**
 * @brief Primitive function de gui 1 buffer data qua UART
 *
 * @details
 * - O che do @c UART_BLOCKING_MODE: ham se BLOCK cho den khi nhan du `len` byte
 * sau do cap nhat @c s_rx_state = UART_DONE ngay trong ham
 *
 * - O che do @c UART_NON_BLOCKING_MODE: Ham chi "kick" UARTDRV de nhan `len` byte
 * va tra ve ngay. Trang thai hoan tat duoc cap nhat trong Callback UARTDRV
 * (thuong la @c s_rx_state = UART_DONE khi nhan du byte)
 *
 * Non-blocking mode Recommended:
 *  - Ham chi kick transaction va tra ve ngay
 *  - Trang thai hoan tat - xem bang ham `zw111_port_uart_tx_poll()`
 *
 * @param buf Buffer chua data can Transmit (De la `const` vi Driver chi doc, khong duoc sua du lieu)
 * @param len Chieu dai Buffer data (So bytes can nhan)
 *
 * @note
 * Lowlevel xay Packet roi truyen gui xuong Port
 * Port chi gui, khong duoc sua *buf
 * Compiler se bat loi neu co gia tri vo tinh ghi vao buf
 *
 * @return true neu da kick TX thanh cong; false neu UART chua san sang hoac dang BUSY
 */
bool zw111_port_uart_tx(const uint8_t *buf, uint16_t len);

/**
 * @brief Primitive function de nhan data 1 luong byte qua UART
 *
 * @details
 * - O che do @c UART_BLOCKING_MODE: ham se BLOCK cho den khi nhan du `len` byte
 * sau do cap nhat @c s_rx_state = UART_DONE ngay trong ham
 *
 * - O che do @c UART_NON_BLOCKING_MODE: Ham chi "kick" UARTDRV de nhan `len` byte
 * va tra ve ngay. Trang thai hoan tat duoc cap nhat trong Callback UARTDRV
 * (thuong la @c s_rx_state = UART_DONE khi nhan du byte)
 *
 * Non-blocking mode Recommended:
 *  - Ham chi kick transaction va tra ve ngay
 *  - Trang thai hoan tat - xem bang ham `zw111_port_uart_rx_poll()`
 *
 * @param buf Buffer de ghi du lieu da nhan duoc vao
 * @param len Chieu dai Buffer data (So bytes can nhan)
 * @param timeout_ms Thoi gian cho toi da de nhan data
 *
 * @warning (NON-BLOCKING - quan trong)
 * Voi @c UARTDRV_Receive(handle, buf, len, callback), callback (va trang thai DONE)
 * thong thuong chi xay ra khi UART nhan du dung @p len byte
 * UART la stream nen du lieu co the den rai rac, bi gian doan, hoac co byte rac truoc frame
 * Khi do, viec yeu cau mot lan RX co dinh (vi du len = 9 byte header) co the dan toi
 * @c UART_DONE hong bao gio xay ra va ham poll timeout
 *
 * @return true neu da kick RX thanh cong; false neu UART chua san sang hoac dang BUSY
 */
bool zw111_port_uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms);

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
 * @brief Flush(Xoa/Lam sach) hoac Abort(Huy bo) transaction (giao dich) RX UART tai Port
 *
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

/* Chi transaction non-blocking moi dung cai nay */
#if defined(UART_NON_BLOCKING_MODE)

/**
 * @brief Poll trang thai TX hien tai
 *
 * @param timeout_ms Thoi gian quyet dinh timeout khi poll
 * @return Trang thai TX (IDLE/BUSY/DONE/ERROR/TIMEOUT)
 */
zw111_port_uart_state_t zw111_port_uart_tx_poll(uint32_t timeout_ms);

/**
 * @brief Poll trang thai RX hien tai va xu ly timeout
 *
 * @details CO CHE TIMEOUT
 * - Ham chi hop le khi truoc do da kick RX bang `zw111_port_uart_rx()` (NON-BLOCKING MODE)
 * - Timeout duoc tinh bang cach lay thoi gian bat dau ke tu luc goi kick RX cho den luc ham poll duoc goi
 * - Khi Timeout xay ra, ham se snapshot trang thai tu UARTDRV de debug bang
 *  `UARTDRV_GetReceiveStatus(handle, &p, &rxCount, &rxRemaining).`
 * - Neu RX dang BUSY va vuot qua timeout cho phep, port Layer co the abort RX va tra ve TIMEOUT
 *
 * @details DONE logic (reach bytes)
 * UART la byte stream: du lieu co the den rai rac. DONE chi xay ra khi dung so byte len da yeu cau
 * Ham nay dung UARTDRV_GetReceiveStatus() de biet rxRemaining (so byte con lai), tranh phu thuoc vao callback
 * De doc packet theo kieu stream (header roi payload luon) trong 1 transaction dai, ham co ho tro kiem tra
 * trang thai transaction RX xem co nhan du bytes khong thong qua 1 bien noi bo `s_rx_need_bytes`
 * Neu nhan du bytes thi return `DONE` de tiep tuc doc tiep cho den khi transaction duoc ABORT boi 1 API khac
 *
 * @param timeout_ms Thoi gian quyet dinh timeout khi poll
 * @return Trang thai TX (IDLE/BUSY/DONE/ERROR/TIMEOUT)
 */
zw111_port_uart_state_t zw111_port_uart_rx_poll(uint32_t timeout_ms);

/**
 * @brief[wrapper] Chờ cho đến khi transaction RX hiện tại nhận đủ ít nhất @p need_bytes byte
 *
 * @details
 * Ham nay la wrapper su dung lai co che static inline API `wait_rx_done()`
 *
 * @param need_bytes
 * @param timeout_ms
 *
 * @return ZW111_
 *
 * @note Khong de la `static inline` duoc do ben trong co bien static noi bo cua ham
 * De the nay de cac ham khac layer cao hon co the dung
 *
 * @note Ham nay khong duoc set ABORT_OK ma chi Poll va tra ve DONE cho den khi doc du N bytes
 * Abort transaction chi duoc lam 1 lan cuoi duy nhat bang 1 ham `zw111_`
 */
zw111_status_t zw111_port_uart_wait_rx_reach(uint16_t need_bytes, uint32_t timeout_ms);

/**
 * @brief Ham cho phep Abort (huy) 1 transaction RX sau khi doc du bytes cua packet
 *
 * @details
 * Trong mot so luong nhan Packet, su dung 1 transaction dai de thu duoc het hdr + payload trong 1 luot
 * de tranh co gap gay nhan thieu bytes (UART la byte-stream). Khi da nhan du byte can thiet (da parse/verrify/checksum),
 * transaction RX van dang o trang thai BUSY va neu de mac, no se ket thuc bang TIMEOUT
 *
 * @note
 * Nen dung ham nay khi da xong 1 transaction va doi state tu BUSY -> ABORT_OK de ket thuc phien giao dich
 *
 * @param timeout_ms Thoi gian de tranh abort ok treo vo han
 * @return
 */
zw111_status_t zw111_port_uart_abort_rx_ok(uint32_t timeout_ms);

#endif // UART_NON_BLOCKING_MODE


/* --------------- HELPER FUNCTION --------------- */

/**
 * @brief Cho TX hoan tat (poll), co timeout
 * Ham nay la wrapper cho tx_poll() vi de tach bach trang thai tra ve cua transaction
 * voi trang thai tra ve cua ham
 *
 * @param timeout_ms
 * @return
 * - ZW111_STATUS_OK - Neu RX hoan tat (UART_DONE)
 * - ZW111_STATUS_ERROR - Neu RX loi (UART_ERROR)
 * - ZW111_STATUS_TIMEOUT - Neu RX timeout (UART_TIMEOUT)
 */
static inline zw111_status_t wait_tx_done(uint32_t timeout_ms){
  while(1){
        zw111_port_uart_state_t ret = zw111_port_uart_tx_poll(timeout_ms);

        if(ret == UART_DONE) return ZW111_STATUS_OK;
        if(ret == UART_ERROR){
            DEBUG_LOG(1, "[PORT] TX done Error...\r\n"); // Debug
            return ZW111_STATUS_ERROR;
        }

        if(ret == UART_TIMEOUT){
            DEBUG_LOG(1, "[PORT] TX done Timeout...\r\n"); // Debug
            return ZW111_STATUS_TIMEOUT;
        }
    }
}

/* ----------------------------------------------------------- */

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
  /* return (uint32_t)(((uint64_t)ticks * 1000u) / 32768u); */

  // Hoac co the dung luon API chinh thuc cua SDK (neu co) sl_sleeptimer_tick_to_ms(ticks)
  return sl_sleeptimer_tick_to_ms(ticks);

#endif // EFR32_PLATFORM

#if defined(STM32_PLATFORM)
  // ...
#endif // STM32_PLATFORM

#if defined(ESP32_PLATFORM)
  // ...
#endif // ESP32_PLATFORM
}

/* ----------------------------------------------------------- */

/**
 * @brief Tinh thoi gian da troi qua (ms) dua tren tick, co xy ly wrap-around
 * @param start_tick Thoi gian dau vao (ticks)
 * @param now_tick Thoi gian tai thoi diem hien tai (ticks)
 * @return Thoi gian chenh lech (chuyen tu tu ticks -> ms) 32-bit
 */
__attribute__((unused)) static inline uint32_t elapsed_ticks(uint32_t start_tick, uint32_t now_tick){
  return (uint32_t)(now_tick - start_tick);
}

__attribute__((unused)) static inline uint32_t elapsed_ms(uint32_t start_tick, uint32_t now_tick){
  uint32_t dt_ticks = elapsed_ticks(start_tick, now_tick);
  return ticks_to_ms_approx(dt_ticks);
}

/* ----------------------------------------------------------- */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ZW111_LIB_INC_ZW111_PORT_H_ */
