/*
 * @file zw111_types.h
 *
 * @date 27 thg 12, 2025
 * @author: LuongHuuPhuc
 *
 * File chua dinh nghia cac struct/type cho zw111 tu datasheet
 * Chi chua khai niem co ban, khong lien quan gi den giao thuc
 * => Thu vien de bao tri hon
 */


#ifndef ZW111_LIB_INC_ZW111_TYPES_H_
#define ZW111_LIB_INC_ZW111_TYPES_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "stdio.h"
#include "stdint.h"

/* Status API driver */
typedef enum ZW111_STATUS{
  ZW111_STATUS_OK           = 0x00,
  ZW111_STATUS_ERROR        = 0x01,

  ZW111_STATUS_NO_FINGER    = 0x02,
  ZW111_STATUS_MATCH_FAIL   = 0x03,

  ZW111_STATUS_TIMEOUT      = 0x04,
  ZW111_STATUS_PACKET_ERR   = 0x05,
  ZW111_STATUS_PASSWORD_ERR = 0x06,
  ZW111_STATUS_DB_FULL      = 0x07,
  ZW111_STATUS_FLASH_ERR    = 0x08,
  ZW111_STATUS_PROTOCOL_ERR = 0x09
} zw111_status_t;

/* Instruction Set/Command ID  (trang 10-12 datasheet) */
typedef enum ZW111_INSTRUCTION_SET {
  ZW111_CMD_GET_IMAGE           = 0x01,  /* Doc Images tu cam bien va luu no vao Image Buffer */
  ZW111_CMD_GEN_CHAR            = 0x02,  /* Tao dac trung fingerprint theo nhu image goc va luu chung vao CharBuffer1 */
  ZW111_CMD_MATCH               = 0x03,  /* So khop mau (Pattern-matching) feature file trong CharBuffer1 va CharBuffer2 */
  ZW111_CMD_SEARCH              = 0x04,  /* Su dung feature files trong CharBuffer1 de tim kiem toan bo hoac tung phan trong FLASH Template database */
  ZW111_CMD_REG_MODEL           = 0x05,  /* Hop nhat feature files trong CharBuffer1 va khoi tao chung thanh template de luu vao CharBuffer1*/
  ZW111_CMD_STORE_CHAR          = 0x06,  /* Luu files trong feature buffer (RAM) vao FLASH (template database) */
  ZW111_CMD_LOAD_CHAR           = 0x07,  /* Doc 1 template tu FLASH (template database) den feature buffer (RAM) */
  ZW111_CMD_UP_CHAR             = 0x08,  /* Upload files trong feature buffer (RAM) vao HOST */
  ZW111_CMD_DOWN_CHAR           = 0x09,  /* Download 1 feature file tu HOST den feature buffer */
  ZW111_CMD_UP_IMAGE            = 0x0A,  /* Uploading anh goc (original image) */
  ZW111_CMD_DOWN_IMAGE          = 0x0B,  /* Download anh goc (original image) */
  ZW111_CMD_DELETE_CHAR         = 0x0C,  /* Xoa 1 feature file trong FLASH template database */
  ZW111_CMD_EMPTY               = 0x0D,  /* Clear FLASH template database */
  ZW111_CMD_WRITE_REG           = 0x0E,  /* Ghi vao he thong thanh ghi SOC (SOC system register) */
  ZW111_CMD_READ_SYS_PARA       = 0x0F,  /* Doc thong so co ban cua he thong */
  ZW111_CMD_SET_PWD             = 0x12,  /* Thiet lap password handshake cho device */
  ZW111_CMD_VERIFY_PWD          = 0x13,  /* Verify password handshake cua device */
  ZW111_CMD_GET_RANDOM_CODE     = 0x14,  /* Lay mau ma ngau nhien */
  ZW111_CMD_SET_CHIP_ADR        = 0x15,  /* Thiet lap dia chi chip */
  ZW111_CMD_READ_INFO_PAGE      = 0x16,  /* Doc noi dung cua FLASH infomation page */
  ZW111_CMD_WRITE_NOTE_PAD      = 0x18,  /* Ghi vao NotePad (FLASH) */
  ZW111_CMD_READ_NOTE_PAD       = 0x19,  /* Doc NotePad (FLASH) */
  ZW111_CMD_VALID_TEMPLATE      = 0x1D,  /* Doc so template con hieu luc (valid) */
  ZW111_CMD_READ_INDEX_TABLE    = 0x1F,  /* Doc bang index cua template */
  ZW111_CMD_CANCEL              = 0x30   /* Huy lenh (command) */
} zw111_cmd_t;

/* ACK/Confirm Code (trang 14-16 datasheet) */
typedef enum ZW111_INSTRUCTION_ACK {
  ZW111_ACK_OK                     = 0x00,  /* Bieu thi thuc thi ket thuc hoac OK */
  ZW111_ACK_PACKET_ERROR           = 0x01,  /* Bieu thi loi khi nhan packet */
  ZW111_ACK_NO_FINGER              = 0x02,  /* Khong nhan thay ngon tay tren cam bien */
  ZW111_ACK_IMAGE_FAIL             = 0x03,  /* Loi khong nhan dien duoc fingerprint image */
  ZW111_ACK_IMAGE_TOO_DRY          = 0x04,  /* Bieu thi fingerprint image qua nhat mau hoac qua sang de tao dac trung */
  ZW111_ACK_IMAGE_TOO_WET          = 0x05,  /* Bieu thi fingerprint image qua mo hoac nhoe - khong ro rang de tao dac trung */
  ZW111_ACK_IMAGE_TOO_BLURRY       = 0x06,  /* Bieu thi fingerprint image khong the xac dinh duoc */
  ZW111_ACK_FEW_FEATURE            = 0x07,  /* Anh qua it dac trung de khoi tao */
  ZW111_ACK_NOT_MATCH              = 0x08,  /* Bieu thi van tay khong khop */
  ZW111_ACK_NOT_FOUND              = 0x09,  /* Khong tim thay van tay */
  ZW111_ACK_MERGE_FAIL             = 0x0A,  /* Khong the hop nhat dac trung van tay */
  ZW111_ACK_PAGE_OUT_OF_RANGE      = 0x0B,  /* Bieu thi dia chi Serial Number (SN) vuot khoi pham vi DB */
  ZW111_ACK_READ_TEMPLATE_FAIL     = 0x0C,  /* Loi khi doc template hoac invalid template tu fingerprint DB */
  ZW111_ACK_UPLOAD_FAIL            = 0x0D,  /* Loi khi tai len dac trung van tay */
  ZW111_ACK_CANNOT_RECEIVE         = 0x0E,  /* Loi module khong the nhan data packet ke tiep */
  ZW111_ACK_IMAGE_UPLOAD_FAIL      = 0x0F,  /* Loi kh tai len anh van tay */
  ZW111_ACK_DELETE_FAIL            = 0x10,  /* Bieu thi loi module khi xoa anh */
  ZW111_ACK_CLEAR_DB_FAIL          = 0x11,  /* Loi khi clear fingerprint DB */
  ZW111_ACK_CANNOT_IN_LOW_PWR      = 0x12,  /* Khong the vao trang thai tieu thu nang luong thap */
  ZW111_ACK_PASSWORD_ERROR         = 0x13,  /* Mat khau sai */
  ZW111_ACK_SYSTEM_RESET_FAILED    = 0x14,  /* Reset he thong loi */
  ZW111_ACK_INVALID_ORINAL_IMG     = 0x15,  /* Khong co anh goc hieu luc trong buffer de tao anh */
  ZW111_ACK_ON_LINE_UPGR_FAILED    = 0x16,  /* */
  ZW111_ACK_INCPLT_FINGERPRINT     = 0x17,  /* Bieu thi co van tay chua hoan thanh hoac ngon tay giu yen qua lau giua 2 lan chup anh */
  ZW111_ACK_READ_WRITE_FLASH_ERROR = 0x18,  /* Doc - ghi FLASH bi loi */
  ZW111_ACK_STREAM_DATA_OK         = 0xF0,  /* ACK cho moi packet du lieu tiep theo */
  ZW111_ACK_STREAM_CMD_ACCEPTED    = 0xF1,  /* ACK cho command bat dau truyen nhieu packet */
  ZW111_ACK_SUM_ERROR              = 0xF2,  /* Sum check loi khi burning internal FLASH */
  ZW111_ACK_PACKET_FLAG_ERROR      = 0xF3,  /* Packet flag loi khi burning internal FLASH */
  ZW111_ACK_PACKET_LENGTH_ERROR    = 0xF4,  /* Chieu dai packet loi khi burning internal FLASH  */
  ZW111_ACK_CODE_LENGTH_TOO_LONG   = 0xF5,  /* Chieu dai code qua dai khi burning internal FLASH */
  ZW111_ACK_BURNING_FLASH_FAILED   = 0xF6,  /* Burning FLASH that bai khi burning internal FLASH */
  ZW111_ACK_NON_DEFINED_ERROR      = 0x19,  /* Loi khong xac dinh */
  ZW111_ACK_INVALID_REG_NUMBER     = 0x1A,  /* So thanh ghi khong ton tai/khong hop le */
  ZW111_ACK_REG_DISTR_WRONG_NUMBER = 0x1B,  /* Thanh ghi phan phoi noi dung sai so luong */
  ZW111_ACK_NOTEPAD_APPOINT_ERROR  = 0x1C,  /* Loi so trang notepad */
  ZW111_ACK_PORT_OPERATION_FAILED  = 0x1D,  /* Loi port thuc thi */
  ZW111_ACK_AUTO_ENROLL_FAILED     = 0x1E,  /* Dang ky dau van tay tu dong that bai */
  ZW111_ACK_DB_FULL                = 0x1F,  /* Fingerprint DB day */
  ZW111_ACK_REVERSED               = 0x20,  /* */
  ZW111_ACK_MUST_VERIFY_PASSWORD   = 0x21   /* Phai xac nhan mat khau truoc */
} zw111_ack_t;

/* Buffer ID (CharBuffer1 & CharBuffer2) */
typedef enum ZW111_CHARBUFFER_ID {
  ZW111_CHARBUFFER_1 = 0x01,
  ZW111_CHARBUFFER_2 = 0x02
} zw111_charbuffer_t;

/* Packet Size Resgister Parameters */
typedef enum ZW111_PACKET_SIZE_REGISTER {
  ZW111_PKT_SIZE_32   = 0,
  ZW111_PKT_SIZE_64   = 1,
  ZW111_PKT_SIZE_128  = 2,
  ZW111_PKT_SIZE_256  = 3
} zw111_packet_size_t;

/* Muc do bao mat/nguong khop cua van tay (level 1 -> level 5) */
typedef enum ZW111_SECURITY_RANK {
  ZW111_MATCH_LEVEL_1 = 1,
  ZW111_MATCH_LEVEL_2 = 2,
  ZW111_MATCH_LEVEL_3 = 3,
  ZW111_MATCH_LEVEL_4 = 4,
  ZW111_MATCH_LEVEL_5 = 5,
} zw111_match_threshold_t;

/* System Registers */
typedef enum ZW111_SYSTEM_REGITER {
  ZW111_REG_BAUDRATE        = 0x04, /* Thanh ghi dieu khien Baud Rate (9600 * N) */
  ZW111_REG_MATCH_THRESHOLD = 0x05, /* Thanh ghi nguong khop (Match Threshold)/Security Rank (1-5) - Cang cao cang kho match */
  ZW111_REG_PKT_SIZE        = 0x06  /* Thanh ghi dieu khien kich thuoc goi tin */
} zw111_reg_t ;

typedef struct {
  uint16_t page_id;      /* ID template */
  uint16_t match_score;  /* Do tuong dong van tay */
} zw111_match_result_t;

/* Thong tin image cho ImageBuffer*/
typedef struct {
  uint32_t size_bytes;
  uint16_t width;
  uint16_t height;
} zw111_image_info_t;

/*  Struct tong hop cua he thong */
typedef struct ZW111_SYSTEM_INFO {
  uint32_t device_address;           /* Dia chi cua cam bien ZW111 (32-bit) */
  uint16_t system_state;             /* Hardware State Register */
  uint16_t sensor_type;              /* Sensor Type Code */
  uint16_t database_capacity;        /* So luong template toi da */
  uint16_t baudrate_multipler;       /* N -> Baudrate = 9600 * N */
  zw111_match_threshold_t security;  /* Security Rank tuong ung voi 5 feature file (1 - 5 level) */
  zw111_packet_size_t packet_size;   /* Kich thuoc Packet (32/64/128/256) */
} zw111_sysinfo_t;

#ifdef __cplusplus
}
#endif // __cplusplus


#endif /* ZW111_LIB_INC_ZW111_TYPES_H_ */
