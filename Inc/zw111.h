/*
 * @file zw111.h
 *
 * @date 27 thg 12, 2025
 * @author: LuongHuuPhuc
 *
 * - File chua API cap cao dieu khien cam bien ZW111
 * - API cho Application (Enroll, Search, Match,...)
 * - Duoc dung chung boi cac Platform khac nhau nen khong the chua cac API rieng biet
 * - Cac API o day se goi API cac lop thap hon (duoc dinh nghia theo tung Port khac nhau)
 *
 * @note
 * Layer nay goi xuong LowLevel (`zw111_lowlevel.h`) de thuc hien quan ly giao thuc Packet/ACK
 * va goi xuong Port/Platform (`zw111_port.h`) de thuc hien khoi tao giao thuc phan cung
 */

#ifndef ZW111_LIB_INC_ZW111_H_
#define ZW111_LIB_INC_ZW111_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "stdio.h"
#include "stdint.h"
#include "zw111_lowlevel.h"

/* Struct config chung cua giao thuc UART cho cac Platform/Port MCU khac cung co the dung duoc */
typedef struct ZW111_UART_CONFIG {
  uint32_t baud;
  uint32_t timeout_ms;
  uint32_t password;

  /* Con tro kieu void de linh dong cau hinh cho tung platform */
  /* Moi platform tu define struct rieng */
  const void *port_cfg;
  uint32_t port_cfg_size;
} zw111_cfg_t;

// ============= APPLICATION PROTOTYPE FUNCTION =============

/**
 *　@brief Khoi tao logic driver ZW111 o muc Application (Platform-specific)
 *
 *　@details
 *　Ham nay kep hop khoi tao UART phan cung (UART init thuoc ve cac file port)
 *　Truoc khi ham nay duoc goi phai:
 *　 - Khoi tao UART phan cung truoc (GPIO,...)
 *　 - Set Handle truoc khi goi
 *
 *　Chuc nang chinh cua ham nay chi Wrapper cua ham Flush UART RX (Xoa rac) + UART port init
 *　va chuan bi demo/transaction moi
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_uart_init(const zw111_cfg_t *cfg);

/**
 *
 * @param cfg
 * @return
 */
zw111_status_t zw111_uart_deinit(const zw111_cfg_t *cfg);

/**
 * @brief API xac thuc mat khau handshake voi ZW111
 *
 * @param[in] pwd Mat khau 32-bit (theo datasheet)
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_verify_password(uint32_t pwd);

/**
 * @brief API thay doi mat khau truy cap module ZW111
 *
 * @details
 * Ghi mat khau moi (32-bit) vao bo nho module
 * Sau khi set, cac lan giao tiep sau se dung mat khau moi de Verify
 *
 * @param[in] new_pwd Mat khau moi
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_set_password(uint32_t new_pwd);

/**
 * @brief Chup anh van tay va luu vao ImageBuffer trong module
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 *  - ZW111_STATUS_NO_FINGER neu khong co ngon tay
 */
zw111_status_t zw111_get_image(void);

/* --------- MATCH FLOW ---------  */

/**
 * @brief API tao dac trung (feature file) tu ImageBuffer va luu vao CharBuffer
 *
 * @param[in] buf ID cua CharBuffer (1 hoac 2)
 *
 * @note ZW111 can 2 vung RAM tam de xu ly cac buoc "Extract - Match - Tao Template - Search"
 * theo luong thuc thi thuc te
 * Mot Buffer co chuc nang luu mau hien tai, 1 Buffer co chuc nang so khop va doi chieu (mau thu 2)
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_gen_char(zw111_charbuffer_t buf);

/**
 * @brief Search CharBuffer trong Database
 *
 * @param[in] buf CharBuffer dung de Search (thuong la CharBuffer1)
 * @param[in] start PageID bat dau Search
 * @param[in] count So luong Template can search tinh tu Start
 * @param[out] result Ket qua search(page_id, match_score)
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 *  - ZW111_STATUS_MATCH_FAIL neu khong tim thay
 */
zw111_status_t zw111_search(zw111_charbuffer_t buf, uint16_t start, uint16_t count, zw111_match_result_t *result);

/**
 * @brief API load char (dac tinh) van tay tu FLASH vao RAM de so khop (thay cho Search)
 * @return
 */
zw111_status_t zw111_load_char(zw111_charbuffer_t buf, uint16_t page_id);

/**
 * @brief API de so sanh diem tuong dong giua CharBuffer1 va CharBuffer2
 * @param[out] score Con tro den gia tri Score tra ve tu module
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 *  - ZW111_STATUS_MATCH_FAIL neu khong khop
 */
zw111_status_t zw111_match(uint16_t *score);

/**
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_sleep_mode(void);

/* --------- ENROLL FLOW ---------  */

/**
 * @brief Bat dau 1 phien enroll (luu page id noi bo neu muon dung flow step)
 *
 * @param[in] page_id PageID muon luu template
 * @return
 */
zw111_status_t zw111_enroll_start(uint16_t page_id);

/**
 *
 * @return
 */
zw111_status_t zw111_enroll_step1(void);

/**
 *
 * @return
 */
zw111_status_t zw111_enroll_step2(void);

/**
 *
 * @return
 */
zw111_status_t zw111_enroll_store(void);

/* --------- DATABASE MANAGEMENT ---------  */

/**
 *
 * @param page_id
 * @return
 */
zw111_status_t zw111_delete_template(uint16_t page_id);

/**
 *
 * @return
 */
zw111_status_t zw111_clear_database(void);

/**
 *
 * @param table
 * @param len
 * @return
 */
zw111_status_t zw111_read_index_table(uint8_t *table, uint8_t len);

/**
 *
 * @return
 */
zw111_status_t zw111_read_notepad(void);

/**
 *
 * @param count
 * @return
 */
zw111_status_t zw111_get_valid_template_count(uint16_t *count);

/* --------- SYSTEM & CONFIG ---------  */

/**
 * @brief API doc tham so co ban cua he thong (baudrate, packet size,....)
 * ACK Packet tra ve 16 bytes dau tien cua **Parameter Table** luu thong tin co ban
 * ve giao tiep va cau hinh va module
 *
 * @note
 * Don vi tra ve trong READ_SYS_PARA la 1 Word = 2 bytes
 * Vi tong 16 bytes tra ve = 8 word
 *
 * @param info Con tro tro den cac tham so tra ve cua he thong module
 * @return
 */
zw111_status_t zw111_read_sysinfo(zw111_sysinfo_t *info);

/**
 * @brief
 * @param multiplier
 * @return
 */
zw111_status_t zw111_set_baudrate(uint16_t multiplier);

/**
 *
 * @param newAddr
 * @return
 */
zw111_status_t zw111_set_new_chip_addr(uint32_t newAddr);

/**
 *
 * @param size
 * @return
 */
zw111_status_t zw111_set_packet_size(zw111_packet_size_t size);

/**
 *
 * @param level
 * @return
 */
zw111_status_t zw111_set_security_level(zw111_match_threshold_t level);

/**
 *
 * @param reg_no Chi so cua thanh ghi can ghi vao trong struct zw111_reg_t
 * @param content Gia tri cau hinh co the thay doi cho thanh ghi
 * @return
 */
zw111_status_t zw111_write_reg_1byte(zw111_reg_t reg_no, uint8_t content);

/* --------------- HELPER FUNCTION --------------- */

/**
 * @brief Ham tra ve chuoi trang thai ACK cho USER
 * @param ack
 * @return
 */
const char *zw111_ack_to_str_debug(zw111_ack_t ack);

/**
 * @brief Ham ho tro anh xa cac tham so trong protocol sang ket qua API cho module
 * @param ack
 * @return
 */
__attribute__((always_inline)) static inline zw111_status_t zw_map_ack_to_status(zw111_ack_t ack){
  switch (ack){

    /* ===== SUCCESS ===== */
    case ZW111_ACK_OK:
      return ZW111_STATUS_OK;

    /* ===== NORMAL FLOW STATES ===== */
    case ZW111_ACK_NO_FINGER:
      return ZW111_STATUS_NO_FINGER;

    case ZW111_ACK_NOT_MATCH:
    case ZW111_ACK_NOT_FOUND:
      return ZW111_STATUS_MATCH_FAIL;

    /* ===== PASSWORD / AUTH ===== */
    case ZW111_ACK_PASSWORD_ERROR:
    case ZW111_ACK_MUST_VERIFY_PASSWORD:
      return ZW111_STATUS_PASSWORD_ERR;

    /* ===== DATABASE ===== */
    case ZW111_ACK_DB_FULL:
      return ZW111_STATUS_DB_FULL;

    /* ===== FLASH / STORAGE ===== */
    case ZW111_ACK_READ_WRITE_FLASH_ERROR:
    case ZW111_ACK_BURNING_FLASH_FAILED:
      return ZW111_STATUS_FLASH_ERR;

    /* ===== PACKET / PROTOCOL ===== */
    case ZW111_ACK_PACKET_ERROR:
    case ZW111_ACK_PACKET_FLAG_ERROR:
    case ZW111_ACK_PACKET_LENGTH_ERROR:
    case ZW111_ACK_SUM_ERROR:
    case ZW111_ACK_CODE_LENGTH_TOO_LONG:
      return ZW111_STATUS_PACKET_ERR;

    /* ===== STREAM ACK (Khong phai loi) ===== */
    case ZW111_ACK_STREAM_DATA_OK:
    case ZW111_ACK_STREAM_CMD_ACCEPTED:
      return ZW111_STATUS_OK;

     default:
       return ZW111_STATUS_ERROR;
   }
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ZW111_LIB_INC_ZW111_H_ */
