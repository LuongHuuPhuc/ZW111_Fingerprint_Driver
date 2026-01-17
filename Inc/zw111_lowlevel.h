/*
 * @file zw111_lowlevel.h
 *
 * @date 27 thg 12, 2025
 * @author: LuongHuuPhuc
 *
 * File chua API cap thap dieu khien cam bien ZW111
 * Mapping datasheet → code - chua API dieu khien giao thuc/protocol USART (Packet Format)
 * Xu ly viec parse (phan tach) packet/checksum/verify/transaction
 *
 * @note
 * Khong duoc phep viet API khoi tao phan cung o day (phai viet o file `zw111_port.h`)
 *
 * @remark
 * Duoc xay len tu API primitive cua `zw111_port.h` thong qua cac Platform API khac nhau (tuy loai MCU)
 * API tai file nay khong can biet Platform nao va no phai la cac API duoc su dung chung cho nhieu Port
 */

#ifndef ZW111_LIB_INC_ZW111_LOWLEVEL_H_
#define ZW111_LIB_INC_ZW111_LOWLEVEL_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "stdio.h"
#include "zw111_types.h"
#include "zw111_port.h"
#include "zw111_port_select.h"

#define ZW111_PKT_HEADER            0xEF01     /* Packet Header */
#define ZW111_DEFAULT_ADDRESS       0xFFFFFFFF /* 4 bytes (32-bit) - 2 Word */
#define ZW111_DEFAULT_PASSWORD      0x00000000 /* 4 bytes (32-bit) - 2 Word */
#define ZW111_PID_COMMAND           0x01       /* Command Packet Flag */
#define ZW111_PID_DATA              0x02       /* Data Packet Flag */
#define ZW111_PID_END               0x08       /* End Packet Flag */
#define ZW111_PID_ACK               0x07       /* ACK Packet Flag */

#define ZW111_CHECKSUM_SIZE_BYTES   2     /* Kich thuoc toi da cua checksum */
#define ZW111_INSTRUCTION_BYTES     1     /* Kich thuoc toi da cua Instruction */
#define ZW111_CONFIRM_CODE_BYTES    1     /* Kich thuoc toi da cua Confirm Code ACK */

#define ZW111_ACK_PAYLOAD_LENGTH_MIN    (ZW111_CHECKSUM_SIZE_BYTES) + (ZW111_CONFIRM_CODE_BYTES) // Packet Length toi thieu cua ACK Packet (3 bytes)
#define ZW111_CMD_PAYLOAD_LENGTH_MIN    (ZW111_CHECKSUM_SIZE_BYTES) + (ZW111_INSTRUCTION_BYTES)  // Packet Length toi thieu cua Command Packet (3 bytes)
#define ZW111_DATA_PAYLOAD_LENGTH_MIN   ZW111_CHECKSUM_SIZE_BYTES // Packet Length toi thieu cua Data Packet (2 bytes) - khong tinh Data
#define ZW111_END_PAYLOAD_LENGTH_MIN    ZW111_CHECKSUM_SIZE_BYTES // Packet Length toi thieu cua End Packet (2 bytes) - Khong tinh Data

#define THIS_IS_LAST_DATA_PACKET        1
#define THIS_IS_NOT_LAST_DATA_PACKET    0

#define ZW111_FLUSH_TOTAL_MS        50
#define ZW111_FLUSH_BYTE_TO         1

// =============== PROTOTYPE FUNCTION ===============

/**
 * @brief API de gui goi lenh (Command packet) den device
 *
 * @remark Packet Format:
 *   [Header][Address][PID][Length][Instruction][Params...][Checksum]
 *
 * @param cmd Command (Instruction ID) can gui den device
 * @param params Con tro den buffer Paramter (1...n) (co the NULL)
 * @param param_len Chieu dai cua buffer Paramter (0 neu khong co tham so)
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_ll_send_command_packet(zw111_cmd_t cmd, const uint8_t *params, uint8_t param_len);

/**
 * @brief API nhan ACK packet tu device cam bien
 *
 * @remark Packet Format:
 *   [Header][Address][PID][Length][Confirm Code][Return Params...][Checksum]
 *
 * @note Ham nay se phan tach ACK packet thanh `Confirm code` va `Optional Return Code`
 * Sau moi lan gui Data Packet hay Command Packet, phai can ACK packet phan hoi lai
 *
 * @param ack Con tro luu Confirm Code cua ACK Packet
 * @param ret_params Buffer luu tham so tra ve (Return parameters) (Can truyen buffer vao de gia tri co the tra ve)
 * @param ret_param_len Con tro luu chieu dai cua tham so tra ve (Return Parameters)
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_ll_receive_ack_packet(zw111_ack_t *ack, uint8_t *ret_params, uint16_t *ret_param_len);

/**
 * @brief API gui Data packet den device cam bien
 *
 * @remark Packet Format:
 *   [Header][Address][PID][Length][Data][Checksum]
 *
 * @param data Buffer tro den data can gui
 * @param data_len Chieu dai buffer data can gui
 * @param is_last De la 1 neu la Data Packet cuoi cung
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_ll_send_data_packet(const uint8_t *data, uint16_t data_len, uint8_t is_last);

/**
 * @brief API de nhan Data packet tu device cam bien
 *
 * @note Dung khi can download image hay feature data
 *
 * @param buf Buffer luu data nhan duoc (Can truyen Buffer vao de gia tri co the tra ve)
 * @param buf_len Chieu dai cho buffer luu nhan duoc
 * @param recv_len Con tro tro den chieu dai data nhan duoc
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 */
zw111_status_t zw111_ll_receive_data_packet(uint8_t *buf, uint16_t buf_len, uint16_t *recv_len);

/**
 * @brief API de tinh checksum cua moi Packet
 *
 * @note Checksum duoc tinh bang tong byte tu Packet Flag (PID) den Checksum bytes
 * Neu vuot qua 2 byte thi no chi giu 16 bits thap nhat
 *
 * @param buf Buffer tro den noi chua tong bytes
 * @param len Chieu dai so bytes can tinh
 *
 * @return Gia tri checksum 16-bit
 */
uint16_t zw111_ll_calc_checksum(const uint8_t *buf, uint16_t len);

/**
 * @brief Ham cho ACK packet phan hoi tu module cam bien sau khi gui Command/Data Packet
 *
 * @details
 * Ham nay duoc su dung de **doi ACK Packet** tu thiet bi trong mot khoang thoi gian
 * toi da do nguoi dung chi dinh
 *
 * Thong thuong, sau khi gui:
 *  - Command Packet
 *  - Data Packet
 *  - End Packet
 * Thiet bi ZW111 se phan hoi bang 1 ACK Packet (PID = 0x07)
 * Ham nay se lien tuc lang nghe UART cho den khi nhan duoc:
 *  - ACK hop le, tra ve OK
 *  - Het thoi gian timeout -> tra ve TIMEOUT
 *
 * @param[out] ack Con tro luu Payload cua ACK Packet nhan duoc
 * @param[in] timeout Thoi gian cho toi da (ms)
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 *
 * @note Ham nay chi cho ACK, khong gui them bat ky Packet nao
 * Viec gui Command/Data phai duoc thuc hien truoc do
 */
zw111_status_t zw111_ll_wait_ack(zw111_ack_t *ack, uint32_t timeout);

/**
 * @brief Gui Command Packet va cho ACK phan hoi tu module cam bien
 *
 * @details
 * Day la ham **low-level helper**, ket hop hai buoc:
 *  1. Gui Command Packet toi thiet bi
 *  2. Cho va nhan ACK Packet phan hoi
 * Ham nay thuong duoc dung boi Applicaton-Layer de don gian hoa luong
 * Ham nay chi dung khi ACK Packet phan hoi lai khong co Return parameter
 *
 * @code
 * zw111_ll_cmd_with_ack(ZW111_CMD_GET_IMAGE, NULL, 0, &ack);
 * @endcode
 *
 * @param cmd Command/Instruction ID) can gui toi ZW111
 * @param params Con tro den buffer chua cac tham so Command (NULL neu khong co tham so)
 * @param param_len Do dai (byte) cua buffer tham so (0 neu khong co tham so)
 * @param ack Con tro de luu Confirm Code cua ACK Packet phan hoi
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 *
 * @note
 * Ham nay **khong xu ly Data Packet**.
 * Voi cac lenh yeu cau upload/download du lieu (image, feature)
 * Can su dung them cac API gui/nhan Data Packet
 */
zw111_status_t zw111_ll_cmd_with_ack(zw111_cmd_t cmd, const uint8_t *params, uint8_t param_len, zw111_ack_t *ack);

/**
 * @brief Xoa (Flush) toan bo du lieu con ton dong trong UART RX Buffer
 *
 * @details
 * Ham nay duoc dung de **lam sach Buffer UART** truoc khi bat dau
 * mot chuoi giao tiep voi module ZW111
 * Thuong su dung trong cac truong hop:
 *  - Sau khi Reset Module
 *  - Sau khi Timeout hoac loi giao thuc
 *  - Truoc khi gui 1 command quan trong (Enroll/Search)
 * Ham se lien tuc doc va bo qua du lieu cho den khi UART khong con du lieu
 *
 * @return zw111_status_t
 *  - ZW111_STATUS_OK on success
 *  - ZW111_STATUS_ERROR on failure
 *
 * @note Ham nay chi thao tac tren UART RX buffer tu phia MCU
 */
zw111_status_t zw111_ll_flush_uart(void);

/**
 * @brief API cho phep set dia chi moi cho chip cam bien
 * bang cach gan gia tri truyen vao cho bien toan cuc cua file
 * Bien toan cuc ay chi duoc dung noi bo trong file `zw111_lowlevel.c`
 *
 * @param addr Dia chi (32-bit) dau vao
 */
void zw111_ll_set_chip_address(uint32_t addr);

/**
 * @brief Ham wrapper cho API get ticks da co san cua Port
 * Muc dich de de quan ly va thong nhat giua cac layer voi nhau
 * @return Ticks hien tai cua he thong
 */
uint32_t zw111_ll_get_ticks(void);

/* --------------- HELPER FUNCTION --------------- */

/**
 * @brief Viet gia tri 16-bit theo thu tu Big-Endian
 * Big-Endian la kieu sap xep ma byte co trong so cao nhat (bit cao - MSB)
 * nam o vi tri dia chi thap nhat (thanh ghi thap hoac byte dau tien) (duoc gui di truoc)
 *
 * @param buf Buffer luu data da chuyen doi truoc khi dua vao thanh ghi
 * @param val Gia tri dau vao (Data, Command, Length,...)
 */
__attribute__((always_inline)) static inline void write_u16_be(uint8_t *buf, uint16_t val){
  buf[0] = (uint8_t)(val >> 8); // Dich 8 bit cao nhat ve phia ben phai (gui dau tien)
  buf[1] = (uint8_t)(val & 0x00FF); // Ghi byte thap (LSB) con lai vao
}

/* ----------------------------------------------------------- */

/**
 * @brief Viet gia tri 32-bit theo thu tu Big-Endian
 * Big-Endian la kieu sap xep ma byte co trong so cao nhat (bit cao - MSB)
 * nam o vi tri dia chi thap nhat (thanh ghi thap hoac byte dau tien) (duoc gui di truoc)
 *
 * @param buf Buffer luu data da chuyen doi truoc khi dua vao thanh ghi
 * @param val Gia tri dau vao
 */
__attribute__((always_inline)) static inline void write_u32_be(uint8_t *buf, uint32_t val){
  buf[0] = (uint8_t)(val >> 24);
  buf[1] = (uint8_t)(val >> 16);
  buf[2] = (uint8_t)(val >> 8);
  buf[3] = (uint8_t)(val & 0x000000FF);
}

/* ----------------------------------------------------------- */

/**
 * @brief Doc gia tri nhi phan 16-bit da duoc thiet bi/protocol gui o
 * dang Big-Endian roi chuyen doi no ve dang so nguyen decimal theo chuan CPU
 * Vi Big-Endian se truyen Byte co trong so cao (MSB) truoc nen de doc dung du lieu
 * can chuyen doi nguoc lai
 *
 * @param buf Buffer luu data sau khi doc va chuyen doi tu data tho
 */
__attribute__((always_inline)) static inline uint16_t read_u16_be(const uint8_t *buf){
  return (uint16_t)((buf[0] << 8) | buf[1]);
}

/* ----------------------------------------------------------- */

/**
 * @brief Doc gia tri nhi phan 32-bit da duoc thiet bi/protocol gui o
 * dang Big-Endian roi chuyen doi no ve dang so nguyen decimal theo chuan CPU
 * Vi Big-Endian se truyen Byte co trong so cao (MSB) truoc nen de doc dung du lieu
 * can chuyen doi nguoc lai
 *
 * @param buf Buffer luu data sau khi doc va chuyen doi tu data tho
 */
__attribute__((always_inline, unused)) static inline uint32_t read_u32_be(const uint8_t *buf){
  return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3]);
}

/* ----------------------------------------------------------- */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* ZW111_LIB_INC_ZW111_LOWLEVEL_H_ */
