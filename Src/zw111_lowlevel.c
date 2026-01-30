/*
 * @file zw111_lowlevel.c
 *
 * @date 27 thg 12, 2025
 * @author LuongHuuPhuc
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "zw111_lowlevel.h"

static uint32_t s_chip_default_addr = ZW111_DEFAULT_ADDRESS;

// =============== STATIC INLINE HELPER FUNCTION DEFINITION ===============

/* ----------------------------------------------------------- */

/**
 * @brief Ham tinh toan checksum cua Packet nhan duoc o dang Big-Endian
 * Gia tri Checksum = Packet Flag -> Last bytes of Payload (Khong tinh field Checksum)
 *
 * @param pid Gia tri cua truong Packet Flag
 * @param len_field Chieu dai (so byte) cua truong Packet Length
 * @param payload Con tro tro den mang chua data Payload
 * @param payload_len Chieu dai cua Payload
 * @return Gia tri 2 bytes checksum
 */
static inline uint16_t calc_checksum_rx(uint8_t pid, uint16_t len_field, const uint8_t *payload, uint16_t payload_len){
  if(payload_len < ZW111_CHECKSUM_SIZE_BYTES) return 0; // Chieu dai toi thieu phai lon hon 2 bytes cua Checksum
  uint32_t sum = 0;

  /* Packet Flag */
  sum += pid;

  /* Packet Length */
  sum += (uint8_t)(len_field >> 8);
  sum += (uint8_t)(len_field & 0x00FF);

  /* Payload (khong tinh Checksum bytes) */
  uint16_t n = payload_len - ZW111_CHECKSUM_SIZE_BYTES;
  for(uint16_t i = 0; i < n; i++){
      sum += payload[i];
  }
  return (uint16_t)(sum & 0xFFFF);
}

/* ----------------------------------------------------------- */

/**
 * @brief Ham doc 2 bytes checksum o duoi moi packet o dang Big-Endian
 * @param payload
 * @param payload_len
 * @return
 */
static inline uint16_t read_checksum_tail_be(const uint8_t *payload, uint16_t payload_len){
  return read_u16_be(&payload[payload_len - 2]);
}

/* ----------------------------------------------------------- */

/**
 * @brief Tra ve dia chi hien tai dang dung cua module cam bien
 * @return Dia chi hien tai (32-bit) dang su dung (mac dinh hoac sau khi gan cai moi)
 */
static inline uint32_t zw111_ll_get_chip_address(void){
  return s_chip_default_addr;
}

// =============== PROTOTYPE FUNCTION DEFINITION ===============

/* ==================== PACKET TRANSMIT ==================== */

zw111_status_t zw111_ll_send_command_packet(zw111_cmd_t cmd, const uint8_t *params, uint8_t param_len){
  if(!cmd) return ZW111_STATUS_ERROR;

  uint8_t tx_buf[64]; // Buffer chua Command Packet can gui (theo byte)
  uint16_t idx = 0;

  /* Header */
  write_u16_be(&tx_buf[idx], ZW111_PKT_HEADER);
  idx += 2; // Cong 2 byte

  /* Chip Address */
  write_u32_be(&tx_buf[idx], zw111_ll_get_chip_address());
  idx += 4;     // Cong 4 byte

  /* Packet Flag (PID) */
  tx_buf[idx++] = ZW111_PID_COMMAND; // 1 byte

  /* Gia tri Packet Length = Instruction (1) + Parameters + Checksum (2) (khong tinh byte cua Packet Length) */
  uint16_t payload_len = ZW111_INSTRUCTION_BYTES + ZW111_CHECKSUM_SIZE_BYTES + param_len; // Gia tri bieu dien
  write_u16_be(&tx_buf[idx], payload_len);
  idx += 2; // Cong 2 bytes

  /* Instruction */
  tx_buf[idx++] = (uint8_t)cmd; // 1 byte cho INSTRUCTION ID

  /* Params */
  if((params != NULL) && (param_len > 0)){
      for(uint8_t i = 0; i < param_len; i++){
          tx_buf[idx++] = params[i];
      }
  }

  /* Gia tri Checksum = Tu Packet Flag -> last payload byte (Packet Flag (1) + Packet Length (2) + Instruction (1) + Params) (khong tinh Checksum) */
  // tx_buf[6] vi day la con tro toi byte dau tien can tinh checksum
  // Vi Checksum no tinh ca Packet Flag nen index phai di tu 6 de no tinh len chieu dai buffer
  uint16_t checksum_len = zw111_ll_calc_checksum(&tx_buf[6], payload_len + ZW111_PACKET_FLAG_BYTES);
  write_u16_be(&tx_buf[idx], checksum_len);
  idx += 2; // Cong 2 bytes checksum vao cuoi Packet

  /* Gui Command Packet vao UART */
  if(!zw111_port_uart_tx(tx_buf, idx)) return ZW111_STATUS_ERROR;

  /* Cho TX xong (timeout ngan) */
  return wait_tx_done(200);
}

/* ----------------------------------------------------------- */

__attribute__((unused)) zw111_status_t zw111_ll_send_data_packet(const uint8_t *data, uint16_t data_len, uint8_t is_last){
  uint8_t tx_buf[300];  // Buffer chua Data Packet can gui(thep Bytes)
  uint16_t idx = 0;

  /* Header */
  write_u16_be(&tx_buf[idx], ZW111_PKT_HEADER);
  idx += 2; // Cong 2 byte

  /* Chip Address */
  write_u32_be(&tx_buf[idx], zw111_ll_get_chip_address());
  idx += 4;     // Cong 4 byte

  /* Packet Flag (PID) */
  tx_buf[idx++] = is_last ? ZW111_PID_END : ZW111_PID_DATA; // Cong vao 1 byte

  /* Gia tri Packet Length = Data + Checksum (2) (khong tinh byte cua Packet Length) */
  uint16_t payload_len = data_len + ZW111_CHECKSUM_SIZE_BYTES; // Gia tri bieu dien
  write_u16_be(&tx_buf[idx], payload_len);
  idx += 2; // Cong vao 2 bytes

  /* Data */
  if((data != NULL) && data_len > 0){
      for(uint16_t i = 0; i < data_len; i++){
          tx_buf[idx++] = data[i];
      }
  }

  /* Gia tri Checksum = Tu Packet Flag -> last payload byte (Packet Flag (1) + Packet Length (2) + Data) (khong tinh Checksum) */
  uint16_t checksum_len = zw111_ll_calc_checksum(&tx_buf[6], payload_len + ZW111_PACKET_FLAG_BYTES);
  write_u16_be(&tx_buf[idx], checksum_len);
  idx += 2; // Cong vao 2 byte

  /* Gui Command Packet vao UART */
  if(!zw111_port_uart_tx(tx_buf, idx)) return ZW111_STATUS_ERROR;

  /* Cho TX xong (timeout ngan) */
  return wait_tx_done(200);
}

/* ----------------------------------------------------------- */

/* ==================== PACKET RECEIVE ==================== */

// FIXME: Ham nay hien tai khong dung den do loi gap giua 2 lan transaction khien khong nhan duoc bytes
__attribute__((unused)) zw111_status_t zw111_ll_receive_ack_packet_ver1(zw111_ack_t *ack, uint8_t *ret_params, uint16_t *ret_param_len){
  if(ack == NULL) return ZW111_STATUS_ERROR;

  uint8_t hdr[ZW111_HDR_LEN]; // Tong byte header + addr + packet flag + packet length

  /* TRANSACTION 1: Receive Header co dinh tu ACK Packet */
  if(!zw111_port_uart_rx(hdr, 9, ZW111_RX_TIMEOUT_MS)) return ZW111_STATUS_ERROR;

  /* Poll rx header　done */
  zw111_status_t ret = zw111_port_uart_wait_rx_reach(ZW111_HDR_LEN, ZW111_RX_TIMEOUT_MS);
  if(ret != ZW111_STATUS_OK){
      DEBUG_LOG(1, "[LOWLEVEL] Poll for RX header in ack packet failed...\r\n");
      return ret;
  }

  if(read_u16_be(&hdr[0]) != ZW111_PKT_HEADER) return ZW111_STATUS_ERROR;
  if(hdr[6] != ZW111_PID_ACK) return ZW111_STATUS_ERROR;

  /* Phan tach du lieu */
  /* Gia tri Packet Length ACK = Confirm + Return Params + Checksum */
  uint16_t payload_len_receive = read_u16_be(&hdr[7]); // Boc tach 2 bytes gia tri tai truong Packet Length

  /* Gia tri Packet Length toi thieu cua ACK Packet la 3 bytes, khong tinh bytes cua Packet Length */
  if(payload_len_receive < ZW111_ACK_PAYLOAD_LENGTH_MIN) return ZW111_STATUS_ERROR;

  uint8_t payload[256]; // Buffer luu payload nhan duoc
  if(payload_len_receive > sizeof(payload)) return ZW111_STATUS_ERROR; // Dieu kien bao ve (optional)

  /* TRANSACTION 2: Receive Payload tu ACK Packet voi tham so dau vao bang do dai payload_len_receive */
  if(!zw111_port_uart_rx(payload, payload_len_receive, ZW111_RX_TIMEOUT_MS)) return ZW111_STATUS_ERROR;

  /* Poll RX payload done */
  ret = zw111_port_uart_wait_rx_reach(payload_len_receive, ZW111_RX_TIMEOUT_MS);
  if(ret != ZW111_STATUS_OK){
      DEBUG_LOG(1, "[LOWLEVEL] Poll for RX payload in ack packet failed...payload_len_receive=%u\r\n", payload_len_receive);
      DEBUG_LOG(1, "[LOWLEVEL][HDR] %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
             hdr[0],hdr[1],hdr[2],hdr[3],hdr[4],hdr[5],hdr[6],hdr[7],hdr[8]);
      return ret;
  }

  // Verify gia tri tra ve cua Checksum
  uint16_t expect = calc_checksum_rx(ZW111_PID_ACK, payload_len_receive, payload, payload_len_receive); // Ky vong
  uint16_t got = read_checksum_tail_be(payload, payload_len_receive); // Doc duoc thuc te
  if(expect != got) return ZW111_STATUS_ERROR;

  /* Gia tri ACK tra ve (Confirm code) doc tu gia tri da duoc luu trong payload tai vi tri dau tien  */
  *ack = (zw111_ack_t)(payload[0]);

  // Kiem tra xem co Return Param khong (doc tu vi tri thu 2 tro di (index 1) - sau confirm code)
  if(ret_params && ret_param_len){
      *ret_param_len = payload_len_receive - ZW111_CONFIRM_CODE_BYTES - ZW111_CHECKSUM_SIZE_BYTES; // Gia tri chieu dai cua Return Parameters
      for(uint16_t i = 0; i < *ret_param_len; i++){
          ret_params[i] = payload[1 + i];
      }
  }
  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_ll_receive_ack_packet_ver2(zw111_ack_t *ack, uint8_t *ret_params, uint16_t *ret_param_len){
  if(ack == NULL) return ZW111_STATUS_ERROR;

  zw111_status_t ret = ZW111_STATUS_ERROR;

  // Thuc hien luon 1 transaction cho header + chip addr + packet flag + packet length (9 bytes) + payload (256)
  uint8_t frame[ZW111_HDR_LEN + 256];
  const uint16_t max_rx_len = (uint16_t)sizeof(frame);

  /* Kick 1 lan RX transaction dai (khong bi gap giua header va payload) */
  if(!zw111_port_uart_rx(frame, max_rx_len, ZW111_RX_TIMEOUT_MS)) return ZW111_STATUS_ERROR;

  /* Doc du 9 bytes header */
  ret = zw111_port_uart_wait_rx_reach(ZW111_HDR_LEN, ZW111_RX_TIMEOUT_MS);
  if(ret != ZW111_STATUS_OK){
      DEBUG_LOG(1, "[LOWLEVEL] Waiting for RX header reach enough bytes in ack packet failed...\r\n");
      goto cleanup_abort;
  }

  uint8_t *hdr = &frame[0]; /* Tao 1 con tro hdr de tro den cac gia tri trong frame (bat dau tu frame tai index = 0) */

  /* Bat dau check cac vi tri cua packet */
  if(read_u16_be(&hdr[0]) != ZW111_PKT_HEADER){
      ret =  ZW111_STATUS_PACKET_ERR;
      goto cleanup_abort;
  }

  if(hdr[6] != ZW111_PID_ACK){
      ret =  ZW111_STATUS_PACKET_ERR;
      goto cleanup_abort;
  }

  /* Gia tri Packet Length ACK = Confirm + Return Params + Checksum */
  uint16_t payload_len_receive = read_u16_be(&hdr[7]); // Boc tach 2 bytes gia tri tai truong Packet Length

  /* Gia tri Packet Length toi thieu cua ACK Packet la 3 bytes, khong tinh bytes cua Packet Length */
  if(payload_len_receive < ZW111_ACK_PAYLOAD_LENGTH_MIN){
      ret =  ZW111_STATUS_PACKET_ERR;
      goto cleanup_abort;
  }
  if(payload_len_receive > 256){
      ret =  ZW111_STATUS_PACKET_ERR;
      goto cleanup_abort;
  }

  /* Doc du toan bo frame can thiet: 9 bytes hdr dau + payload_len */
  uint16_t need_total = (uint16_t)(ZW111_HDR_LEN + payload_len_receive);

  ret = zw111_port_uart_wait_rx_reach(need_total, ZW111_RX_TIMEOUT_MS);
  if(ret != ZW111_STATUS_OK){
      DEBUG_LOG(1, "[LOWLEVEL] Waiting for RX payload reach enough bytes in ack packet failed...need_total=%u\r\n", need_total);
      DEBUG_LOG(1, "[LOWLEVEL][HDR] %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
             hdr[0],hdr[1],hdr[2],hdr[3],hdr[4],hdr[5],hdr[6],hdr[7],hdr[8]);
      goto cleanup_abort;
  }

  uint8_t *payload = &frame[ZW111_HDR_LEN]; /* Con tro de doc lan luot cac gia tri cua payload (bat dau tu frame index = 9) */

  // Verify gia tri tra ve cua Checksum
  uint16_t expect = calc_checksum_rx(ZW111_PID_ACK, payload_len_receive, payload, payload_len_receive); // Ky vong
  uint16_t got = read_checksum_tail_be(payload, payload_len_receive); // Doc duoc thuc te
  if(expect != got){
      ret =  ZW111_STATUS_PACKET_ERR;
      goto cleanup_abort;
  }

  /* Gia tri ACK tra ve (Confirm code) doc tu gia tri da duoc luu trong payload tai vi tri dau tien  */
  *ack = (zw111_ack_t)(payload[0]);

  // Kiem tra xem co Return Param khong (doc tu vi tri thu 2 tro di (index 1) - sau confirm code)
  if(ret_params && ret_param_len){
      *ret_param_len = payload_len_receive - ZW111_CONFIRM_CODE_BYTES - ZW111_CHECKSUM_SIZE_BYTES; // Gia tri chieu dai cua Return Parameters
      for(uint16_t i = 0; i < *ret_param_len; i++){
          ret_params[i] = payload[1 + i];
      }
  }

  ret = ZW111_STATUS_OK; // Den day coi nhu la OK

  cleanup_abort:
    /**
     * @note Transaction RX van dang BUSY. Abort chu dich de tranh tu no TIMEOUT ve sau
     * va de driver quay ve trang thai sach truoc lan transacton tiep theo
     */
    (void)zw111_port_uart_abort_rx_ok(ZW111_RX_TIMEOUT_MS);

  return ret;
}

/* ----------------------------------------------------------- */

__attribute__((unused)) zw111_status_t zw111_ll_receive_data_packet(uint8_t *buf, uint16_t buf_len, uint16_t *recv_len){
  uint8_t hdr[ZW111_HDR_LEN]; // Tong byte header + addr + packet flag + packet length

  /* Transaction 1: Receive Header co dinh tu ACK Packet */
  if(!zw111_port_uart_rx(hdr, 9, ZW111_RX_TIMEOUT_MS)) return ZW111_STATUS_ERROR;

  /* Poll rx header done */
  zw111_status_t ret = zw111_port_uart_wait_rx_reach(ZW111_HDR_LEN, ZW111_RX_TIMEOUT_MS);
  if(ret != ZW111_STATUS_OK){
      DEBUG_LOG(1, "[LOWLEVEL] Poll for RX header in data packet failed...\r\n");
      return ret;
  }

  if(read_u16_be(&hdr[0]) !=  ZW111_PKT_HEADER) return ZW111_STATUS_ERROR;

  uint8_t pid = hdr[6];
  if(pid != ZW111_PID_DATA && pid != ZW111_PID_END) return ZW111_STATUS_ERROR;

  // Boc tach 2 bytes gia tri nhan duoc tai truong Packet Length
  uint16_t payload_len_receive = read_u16_be(&hdr[7]);

  // Neu gia tri nhan duoc be hon chieu dai 2 bytes (chieu dai cua truong Packet Length)
  if(payload_len_receive < ZW111_DATA_PAYLOAD_LENGTH_MIN) return ZW111_STATUS_ERROR;

  // Neu gia tri chieu dai nhan duoc lon hon ca chieu dai buffer truyen vao
  if(payload_len_receive - ZW111_DATA_PAYLOAD_LENGTH_MIN > buf_len) return ZW111_STATUS_ERROR;

  uint8_t payload[300];
  if(payload_len_receive > sizeof(payload)) return ZW111_STATUS_ERROR;

  /* Transaction 2: Receive Payload tu ACK Packet voi tham so dau vao bang do dai payload_len_receive */
  if(!zw111_port_uart_rx(payload, payload_len_receive, ZW111_RX_TIMEOUT_MS)) return ZW111_STATUS_ERROR;

  /* Poll RX payload done */
  ret = zw111_port_uart_wait_rx_reach(payload_len_receive, ZW111_RX_TIMEOUT_MS);
  if(ret != ZW111_STATUS_OK){
      DEBUG_LOG(1, "[LOWLEVEL] Poll for RX payload in data packet failed...\r\n");
      return ret;
  }

  // Verify gia tri chieu dai tra ve cua Checksum
  uint16_t expect = calc_checksum_rx(pid, payload_len_receive, payload, payload_len_receive); // Ky vong
  uint16_t got = read_checksum_tail_be(payload, payload_len_receive); // Doc duoc thuc te
  if(expect != got) return ZW111_STATUS_ERROR;

  /* Copy Payload nhan duoc qua buffer truyen vao */
  if((buf != NULL) && buf_len > 0){
      *recv_len = payload_len_receive - ZW111_CHECKSUM_SIZE_BYTES;
      for(uint8_t i = 0; i < *recv_len; i++){
          buf[i] = payload[i];
      }
  }
  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

uint16_t zw111_ll_calc_checksum(const uint8_t *buf, uint16_t len){
  uint32_t sum = 0;
  for(uint16_t i = 0; i < len; i++){
      sum += buf[i];
  }
  return (uint16_t)(sum & 0xFFFF);
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_ll_wait_ack(zw111_ack_t *ack, uint32_t timeout){
  uint32_t start = zw111_ll_get_ticks();

  while(elapsed_ms(start, zw111_ll_get_ticks()) < timeout){
      zw111_ack_t local_ack = 0;
      zw111_status_t ret = zw111_ll_receive_ack_packet_ver2(&local_ack, NULL, NULL);

      if(ret == ZW111_STATUS_OK){
          if(ack) *ack = local_ack;
          return ZW111_STATUS_OK;
      }
      if(ret != ZW111_STATUS_TIMEOUT) return ret;
  }
  return ZW111_STATUS_TIMEOUT;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_ll_cmd_with_ack(zw111_cmd_t cmd, const uint8_t *params, uint8_t param_len, zw111_ack_t *ack){
  if(zw111_ll_send_command_packet(cmd, params, param_len) == ZW111_STATUS_OK){
      return zw111_ll_receive_ack_packet_ver2(ack, NULL, NULL);
  }
  return ZW111_STATUS_ERROR;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_ll_flush_uart(void){
  if(zw111_port_uart_flush() != true){
      return ZW111_STATUS_ERROR;
  }
  return ZW111_STATUS_OK;
}

/* ----------------------------------------------------------- */

void zw111_ll_set_chip_address(uint32_t addr){
  s_chip_default_addr = addr;
}

/* ----------------------------------------------------------- */

uint32_t zw111_ll_get_ticks(void){
  return zw111_port_get_ticks();
}

/* ----------------------------------------------------------- */

#ifdef __cplusplus
}
#endif // __cplusplus
