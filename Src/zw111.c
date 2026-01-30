/*
 * @file zw111.c
 *
 * @date 27 thg 12, 2025
 * @author: LuongHuuPhuc
 *
 * File dinh nghia Logic giao thuc o Application Layer
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "zw111.h"
#include "string.h"

/* Default enroll state */
static uint16_t s_enroll_page_id_local = 0xFFFF;

/* ----------------------------------------------------------- */

zw111_status_t zw111_uart_init(const zw111_cfg_t *cfg){
  if(!cfg) return ZW111_STATUS_ERROR;

  /* 1. Binding Init UART hardware qua Port layer */
  if(zw111_port_uart_init(cfg->baud, cfg->port_cfg, cfg->port_cfg_size) != true){
      return ZW111_STATUS_ERROR;
  }

  /* 2. Flush UART RX (tranh rac sau Reset) */
  if(zw111_ll_flush_uart() != ZW111_STATUS_OK) return ZW111_STATUS_ERROR;

  /* 3. Verify password neu can */
  if(cfg->password != 0){
      if(zw111_verify_password(cfg->password) != ZW111_STATUS_OK){
          return ZW111_STATUS_ERROR;
      }
  }
  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_uart_deinit(const zw111_cfg_t *cfg){
  /* TODO: Hien tai voi EFR32 chua can dung (Sau nay mo rong cho cac Platform khac) */
  (void)(cfg);

  if(zw111_port_uart_deinit() != true) return ZW111_STATUS_ERROR;
  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_set_password(uint32_t new_pwd){
  uint8_t params[4]; // 4 bytes password dau vao can thay doi
  write_u32_be(params, new_pwd);

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_SET_PWD, params, (uint8_t)sizeof(params), &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_verify_password(uint32_t pwd){
  uint8_t p[4]; // Buffer chua password dau vao can xac thuc (4 bytes)
  write_u32_be(p, pwd);

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_VERIFY_PWD, p, (uint8_t)sizeof(p), &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_get_image(void){
  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_GET_IMAGE, NULL, 0, &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* --------- MATCH FLOW ---------  */

/* ----------------------------------------------------------- */

zw111_status_t zw111_gen_char(zw111_charbuffer_t buf){
  uint8_t p[1] = {(uint8_t)buf}; // Buffer dua tham so cho lenh (TX) la BufferID

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_GEN_CHAR, p, 1, &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_search(zw111_charbuffer_t buf, uint16_t start, uint16_t count, zw111_match_result_t *result){
  if(result == NULL) return ZW111_STATUS_ERROR;

  /* Params cua lenh Search can gui di: BufferID (1 bytes) + Param StartPage (2 bytes) + Param PageNum (2 bytes) = 5 bytes */
  uint8_t p[5];
  p[0] = (uint8_t)buf;
  write_u16_be(&p[1], start);
  write_u16_be(&p[3], count);

  zw111_ack_t ack = 0; // Luu gia tri ACK tra ve
  uint8_t ret_params[16] = {0}; // Luu tham so tra ve tu ACK
  uint16_t ret_len = 0; // Chieu dai params

  zw111_status_t ret = zw111_ll_send_command_packet(ZW111_CMD_SEARCH, p, (uint8_t)sizeof(p));
  if(ret != ZW111_STATUS_OK) return ret;

  ret = zw111_ll_receive_ack_packet_ver2(&ack, ret_params, &ret_len);
  if(ret != ZW111_STATUS_OK) return ret;

  /* Return tu ACK Packet: PageID (2 bytes) + Score (2 bytes) */
  if(ret_len < 4) return ZW111_STATUS_ERROR;

  result->match_score = read_u16_be(&ret_params[0]);
  result->page_id = read_u16_be(&ret_params[2]);

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_load_char(zw111_charbuffer_t buf, uint16_t page_id){
  if(page_id == 0xFFFF) return ZW111_STATUS_ERROR;

  /* Params Cmd Packet: BufferID (1) + PageID (2) = 3 bytes */
  uint8_t p[3];
  p[0] = (uint8_t)buf; // BufferID
  write_u16_be(&p[1], page_id); // PageID

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_LOAD_CHAR, p, (uint8_t)sizeof(p), &ack); // Khong co Return params
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_match(uint16_t *score){
  /* PS_Match: ACK co the tra ve score (2 bytes) - theo datasheet */
  zw111_ack_t ack = 0;
  uint8_t ret_params[8] = {0}; // Buffer chua gia tri tham so tra ve
  uint16_t ret_len = 0;

  // Command MATCH khong co Parameter gui di
  zw111_status_t ret = zw111_ll_send_command_packet(ZW111_CMD_MATCH, NULL, 0);
  if(ret != ZW111_STATUS_OK) return ret;

  // Nhan lai ACK phan hoi sau khi gui Command
  ret = zw111_ll_receive_ack_packet_ver2(&ack, ret_params, &ret_len);
  if(ret != ZW111_STATUS_OK) return ret;

  // Anh xa lai ket qua ACK
  ret = zw_map_ack_to_status(ack);
  if(ret != ZW111_STATUS_OK) return ret;

  // Gia tri Score tra ve (Sau Confirm code)
  if(score){
      if(ret_len < 2) return ZW111_STATUS_ERROR;
      *score = read_u16_be(&ret_params[0]);
  }
  return zw_map_ack_to_status(ack);
}

/* --------- ENROLL FLOW ---------  */

/* ----------------------------------------------------------- */

zw111_status_t zw111_enroll_start(uint16_t page_id){
  s_enroll_page_id_local = page_id;
  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

/**
 * @brief Ham lay pageID de phuc vu cho qua trinh enroll van tay
 * @note De la static de App Layer khong nhin thay, chi dung noi bo
 *
 * @return pageID muon Enroll
 */
static uint16_t zw111_get_enroll_pageid(void){
  return s_enroll_page_id_local;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_enroll_step1(void){
  /* Buoc 1: GetImage + GenChar (CharBuffer1) */
  zw111_status_t ret = zw111_get_image();
  if(ret != ZW111_STATUS_OK) return ret;

  return zw111_gen_char(ZW111_CHARBUFFER_1);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_enroll_step2(void){
  /* Buoc 2: GetImage + GenChar (CharBuffer2) + RegModel (merge) */
  /* Sau khi co duoc feature file qua GenChar trong CharBuffer1 va CharBuffer2 thi thuc hien
   * merge cac feature file voi nhau de tao ra Template file, ket qua lai duoc luu trong CB1 va CB2 */
  zw111_status_t ret = zw111_get_image();
  if(ret != ZW111_STATUS_OK) return ret;

  ret = zw111_gen_char(ZW111_CHARBUFFER_2);
  if(ret != ZW111_STATUS_OK) return ret;

  zw111_ack_t ack = 0;

  // Command RegModel khong co Param gui di
  // ACK phan hoi cung khong co return param
  ret = zw111_ll_cmd_with_ack(ZW111_CMD_REG_MODEL, NULL, 0, &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_enroll_store(void){
  uint16_t page_id = zw111_get_enroll_pageid();

  if(page_id == 0xFFFF) return ZW111_STATUS_ERROR;

  /* StoreChar (Store Templates) luu template file trong CharBuffer1 vao PageID tai FLASH */
  /* Params Command: BufferID (1 byte) + LocationNum (2 bytes) = 3 bytes */
  uint8_t p[3];
  p[0] = (uint8_t)ZW111_CHARBUFFER_1;
  write_u16_be(&p[1], page_id);

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_STORE_CHAR, p, (uint8_t)sizeof(p), &ack); // Khong co Return Param
  if(ret != ZW111_STATUS_OK) return ret;

  /* Reset enroll context sau khi da store xong */
  s_enroll_page_id_local = 0xFFFF;
  return zw_map_ack_to_status(ack);
}


/* --------- DATABASE MANAGEMENT ---------  */

zw111_status_t zw111_delete_template(uint16_t page_id){
  /* PS_DeleteChar Params: PageID (2 bytes) + DeleteNum (2 bytes) = 4 bytes */
  uint8_t p[4];
  write_u16_be(&p[0], page_id);
  write_u16_be(&p[2], 1); /* Xoa 1 temaplate */

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_DELETE_CHAR, p, (uint8_t)sizeof(p), &ack); // Khong co Return Param
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_clear_database(void){
  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_EMPTY, NULL, 0, &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_read_index_table(uint8_t *table, uint8_t len){
  if(table == NULL) return ZW111_STATUS_ERROR;

  /* PS_ReadIndexTable params: IndexPage (1 byte): Page 0/ Page 1
   * ACK tra ve 32 bytes index info
   * Neu muon doc 2 page thi => len >= 64 va goi 2 lan */

  zw111_ack_t ack = 0;
  uint16_t ret_len = 0;

  if(len < 32) return ZW111_STATUS_ERROR;

  {
      /* Page 0 (0 ~ 255) */
      uint8_t p[1] = {0};
      uint8_t ret_param[32] = {0};

      // Gui command
      zw111_status_t ret = zw111_ll_send_command_packet(ZW111_CMD_READ_INDEX_TABLE, p, 1);
      if(ret != ZW111_STATUS_OK) return ret;

      // Nhan ACK phan hoi + Return Param (Index Info) 32 bytes
      ret = zw111_ll_receive_ack_packet_ver2(&ack, ret_param, &ret_len);
      if(ret != ZW111_STATUS_OK) return ret;

      ret = zw_map_ack_to_status(ack);
      if(ret != ZW111_STATUS_OK) return ret;

      if(ret_len < 32) return ZW111_STATUS_ERROR;
      memcpy(&table[0], ret_param, 32); // Copy qua table
  }

  {
    /* Page 1 (Optional) (256 ~ 511) */
    if(len >= 64){
        uint8_t p[1] = {1};
        uint8_t ret_param[32] = {0};

        // Gui command
        zw111_status_t ret = zw111_ll_send_command_packet(ZW111_CMD_READ_INDEX_TABLE, p, 1);
        if(ret != ZW111_STATUS_OK) return ret;

        // Nhan ACK phan hoi + Return Param (Index Info) 32 bytes
        ret = zw111_ll_receive_ack_packet_ver2(&ack, ret_param, &ret_len);
        if(ret != ZW111_STATUS_OK) return ret;

        ret = zw_map_ack_to_status(ack);
        if(ret != ZW111_STATUS_OK) return ret;

        if(ret_len < 32) return ZW111_STATUS_ERROR;
        memcpy(&table[32], ret_param, 32); // Copy qua table
    }
  }

  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_get_valid_template_count(uint16_t *count){
  if(count == NULL) return ZW111_STATUS_ERROR;

  zw111_ack_t ack = 0;
  uint8_t ret_param[4] = {0};
  uint16_t ret_len = 0;

  zw111_status_t ret = zw111_ll_send_command_packet(ZW111_CMD_VALID_TEMPLATE, NULL, 0);
  if(ret != ZW111_STATUS_OK) return ret;

  ret = zw111_ll_receive_ack_packet_ver2(&ack, ret_param, &ret_len);
  if(ret != ZW111_STATUS_OK) return ret;

  if(ret_len < 2) return ZW111_STATUS_ERROR;
  *count = read_u16_be(&ret_param[0]);

  return zw_map_ack_to_status(ack);
}

/* --------- SYSTEM & CONFIG ---------  */

zw111_status_t zw111_read_sysinfo(zw111_sysinfo_t *info){
  if(info == NULL) return ZW111_STATUS_ERROR;

  zw111_ack_t ack = 0;
  uint8_t ret_param[32] = {0};
  uint16_t ret_len = 0;

  zw111_status_t ret = zw111_ll_send_command_packet(ZW111_CMD_READ_SYS_PARA, NULL, 0);
  if(ret != ZW111_STATUS_OK) return ret;

  ret = zw111_ll_receive_ack_packet_ver2(&ack, ret_param, &ret_len);
  if(ret != ZW111_STATUS_OK) return ret;

  ret = zw_map_ack_to_status(ack);
  if(ret != ZW111_STATUS_OK) return ret;

  /* Basic parameter table: 16 bytes theo datasheet */
  if(ret_len < 16) return ZW111_STATUS_ERROR;

  info->system_state = read_u16_be(&ret_param[0]); // 2 bytes (1 word)
  info->sensor_type = read_u16_be(&ret_param[2]); // 2 bytes
  info->database_capacity = read_u16_be(&ret_param[4]); // 2 bytes
  info->security = (zw111_match_threshold_t)read_u16_be(&ret_param[6]); // 2 bytes
  info->device_address = read_u32_be(&ret_param[8]); // 4 bytes (2 word)
  info->packet_size = (zw111_packet_size_t)read_u16_be(&ret_param[10]); // 2 bytes
  info->baudrate_multipler = read_u16_be(&ret_param[12]); // 2 bytes

  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_set_new_chip_addr(uint32_t newAddr){
  /* Params Command: ChipAddress (4 bytes) */
  uint8_t p[4];
  write_u32_be(&p[0], newAddr);

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_SET_CHIP_ADR, p, sizeof(p), &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  // Set lai dia chi moi vao bien static dang dung
  zw111_ll_set_chip_address(newAddr);

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_write_reg_1byte(zw111_reg_t reg_no, uint8_t content){
  uint8_t p[2];
  p[0] = (uint8_t)reg_no;
  p[1] = content;

  zw111_ack_t ack = 0;
  zw111_status_t ret = zw111_ll_cmd_with_ack(ZW111_CMD_WRITE_REG, p, 2, &ack);
  if(ret != ZW111_STATUS_OK) return ret;

  return zw_map_ack_to_status(ack);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_set_baudrate(uint16_t multiplier){
  /* Datasheet: baud = 9600 * N, N la 1 byte */
  if(multiplier == 0 || multiplier > 255) return ZW111_STATUS_ERROR;
  return zw111_write_reg_1byte(ZW111_REG_BAUDRATE, (uint8_t)multiplier);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_set_packet_size(zw111_packet_size_t size){
  if(size > ZW111_PKT_SIZE_256) return ZW111_STATUS_ERROR;
  return zw111_write_reg_1byte(ZW111_REG_PKT_SIZE, (uint8_t)size);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_set_security_level(zw111_match_threshold_t level){
  if(level < ZW111_MATCH_LEVEL_1 || level > ZW111_MATCH_LEVEL_5) return ZW111_STATUS_ERROR;
  return zw111_write_reg_1byte(ZW111_REG_MATCH_THRESHOLD, (uint8_t)level);
}

/* ----------------------------------------------------------- */

const char *zw111_ack_to_str_debug(zw111_ack_t ack){
  switch(ack){
    case ZW111_ACK_OK: return "OK";
    case ZW111_ACK_NO_FINGER: return "NO_FINGER";
    case ZW111_ACK_NOT_MATCH: return "NOT_MATCH";
    case ZW111_ACK_NOT_FOUND: return "NOT_FOUND";
    case ZW111_ACK_PACKET_ERROR: return "PACKET_ERROR";
    case ZW111_ACK_PASSWORD_ERROR: return "PASSWORD_ERROR";
    case ZW111_ACK_DB_FULL: return "DB_FULL";
    default: return "UNKNOWN_ACK";
  }
}

#ifdef __cplusplus
}
#endif // __cplusplus
