/*
 * @file zw111_app.h
 *
 * @date 27 thg 12, 2025
 * @author: LuongHuuPhuc
 *
 * File chua cac API cho Application Layer (cao nhat) trong Zigbee AF
 */

#ifndef ZW111_LIB_APP_ZW111_APP_H_
#define ZW111_LIB_APP_ZW111_APP_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "stdio.h"
#include "stdint.h"
#include "../Inc/zw111.h"

/* Timeout cho finger */
#ifndef ZW111_APP_TIMEOUT_GET_IMAGE_MS
#define ZW111_APP_TIMEOUT_GET_IMAGE_MS  3000
#endif // ZW111_APP_TIMEOUT_GET_IMAGE_MS

/* So lan thu cho moi buoc */
#ifndef ZW111_APP_ENROLL_MAX_TRIES
#define ZW111_APP_ENROLL_MAX_TRIES      8
#endif // ZW111_APP_ENROLL_MAX_TRIES

/* Nguong score toi thieu */
#ifndef ZW111_APP_MATCH_SCORE_MIN
#define ZW111_APP_MATCH_SCORE_MIN       50
#endif // ZW111_APP_MATCH_SCORE_MIN

/* Struct luu trang thai tra ve cua API o Application Layer cho cam bien */
typedef enum {
  /* Trang thai nhan roi cua he thong, chua thuc hien bat ky thao tac nao voi he thong */
  ZW111_APP_IDLE = 0,

  /* Handshake state - kiem tra va khoi tao cam bien
   * - Trang thai nay dung de kiem tra xem cam bien co phan hoi hay khong
   * - Verify password/doc thong tin co ban
   *
   * Neu Probe thanh cong -> Chuyen sang WAIT_FINGER state
   * Neu that bat -> Chuyen sag ERROR state */
  ZW111_APP_PROBE,

  /* Finger - cho USER dat ngon tay len cam bien
   * O trang thai nay, he thong lap lai:
   *  - Gui lenh GetImage va kiem tra ACK tra ve
   *
   * Neu chua co ngon tay -> giu nguyen trang thai
   * Khi da nhan duoc anh hop le -> Chuyen sang GEN_CHAR */
  ZW111_APP_WAIT_FINGER,

  /* Trich xuat dac trung van tay
   * Thuc hien lenh GenChar (tao ra feature file tu ImageBuffer)
   * Feature file duoc luu vao CharBuffer (CB1 hoac CB2)
   *
   * Neu thanh cong -> Chuyen sang MATCH
   * Neu that bai -> chuyen sang ERROR */
  ZW111_APP_GEN_CHAR,

  /* So khop van tay
   * Neu thanh cong -> DONE
   * Neu that bai -> quay lai WAIT_FINGER */
  ZW111_APP_MATCH,

  /* Hoan tat 1 chu ky xu ly van tay
   * Trang thai ket thuc cua 1 chu ky:
   *  - Nhan dien thanh cong hoac enroll (dang ky) thanh cong
   * Sau khi xu ly xong, quay ve IDLE  */
  ZW111_APP_DONE,

  /* */
  ZW111_APP_ENROLL_STEP1,

  /* */
  ZW111_APP_ENROLL_STEP2,

  /* */
  ZW111_APP_ENROLL_STORE,

  /* Loi trong qua trinh xu ly */
  ZW111_APP_ERROR
} zw111_app_state_t;

/* ----------------------------------------------------------- */

#ifdef EFR32_PLATFORM

/**
 * @brief API cho application layer de khoi tao phan cung UART cho cam bien ZW111
 *
 * @details
 * Ham nay se thiet lap giao thuc thong qua cac thong so noi bo dau vao duoc thiet lap tai ham (co the Override boi USER)
 * Sau do se dung API cap thap de khoi tao phan cung cho UART
 *
 * @param[in] baudrate Toc do Baud mong muon
 * @param[in] timeout_ms Thoi gian cho khoi tao
 * @param[in] password Mat khau cho cam bien ZW111
 */
zw111_app_state_t zw111_app_uart_init(uint32_t baudrate, uint32_t timeout_ms, uint32_t password);

/**
 * @brief Ham kiem tra
 */
zw111_app_state_t zw111_app_sensor_probe(void);

/**
 * @brief
 */
zw111_app_state_t zw111_app_process(void);

/**
 * @brief Ham kich hoat FSM (May trang thai) cua chuong trinh
 * `zw111_app_process()` se bat dau flow 1 chu ky cua chuong trinh
 *  sau khi chuyen tu `ZW111_APP_IDLE` sang `ZW111_APP_PROBE`
 */
void zw111_app_start(void);

/**
 * @brief API bat dau qua trinh Enroll (dang ky) van tay moi
 */
void zw111_app_start_enroll(uint16_t page_id);

/**
 *
 */
zw111_app_state_t zw111_app_get_state(void);

#endif // EFR32_PLATFORM

/* ----------------------------------------------------------- */

#ifdef STM32_PLATFORM
/* */
#endif // STM32_PLATFORM

/* ----------------------------------------------------------- */

#ifdef ESP32_PLATFORM
/* */
#endif // ESP32_PLATFORM

/* ----------------------------------------------------------- */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ZW111_LIB_APP_ZW111_APP_H_ */
