/*
 * @file zw111_port_efr23.c
 *
 * @date 27 thg 12, 2025
 * @author LuongHuuPhuc
 *
 * File dinh nghia thao tac Transmit va Receive co ban cua USART cho EFR32
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(EFR32_PLATFORM)
#include "../../Inc/Port/zw111_port_efr32.h"

/* Bien dam nhiem handle cac thong so du lieu cua gia thuc UART sau khi da khoi tao */
#ifdef USER_PORT_UART_INIT
static UARTDRV_HandleData_t s_uart_handle_data;
static UARTDRV_Handle_t s_uart_handle = &s_uart_handle_data;
#endif // USER_PORT_UART_INIT

/* Khoi tao UART_Handle_t truoc roi gan bien nay cho no */
static UARTDRV_Handle_t uart_efr32_handle = NULL;

#if !defined(UART_NON_BLOCKING_MODE) && !defined(UART_BLOCKING_MODE)
#define UART_NON_BLOCKING_MODE
#endif

/* ----------------------------------------------------------- */

/* Co trang thai de biet TX/RX da xong hay chua */
static volatile zw111_port_uart_state_t s_tx_state = UART_IDLE;
static volatile zw111_port_uart_state_t s_rx_state = UART_IDLE;

/* Luu trang thai tra ve tu Callback */
static volatile Ecode_t s_tx_status = ECODE_OK;
static volatile Ecode_t s_rx_status = ECODE_OK;

#ifdef UART_NON_BLOCKING_MODE

/* Phuc vu cho viec Poll timeout done hay chua (danh dau thoi diem bat dau transaction) */
static volatile uint32_t s_rx_start_kick = 0;
static volatile uint32_t s_tx_start_kick = 0;

/* Bien bao neu rx nhan du N bytes thi DONE som (0 = disable early-done) */
static volatile uint16_t s_rx_need_bytes = 0;

/* ----------------------------------------------------------- */

/**
 * @note
 * Callback se bao lai tinh trang transaction cua giao thuc moi khi API goi TX/RX chay thanh cong (truyen/nhan du byte yeu cau)
 * hoac khong thanh cong (khong du byte) (khac voi API tu return trang thai cua chinh no)
 * Hai ham callback nay se duoc he thong goi lien tuc (neu khong log theo debug level thi log se lien tuc spam luon)
 */
static void uart_efr32_tx_callback(UARTDRV_HandleData_t *handle,
                                   Ecode_t transferStatus,
                                   uint8_t *data,
                                   UARTDRV_Count_t transferCount){
  (void)(handle);
  (void)(data);
  s_tx_status = transferStatus;

  /* Neu truyen di du len byte yeu cau tu UARTDRV_Transmit() */
  if(transferStatus == ECODE_OK || transferStatus == ECODE_EMDRV_DMADRV_OK){
      s_tx_state = UART_DONE;  // Gan ngay state la DONE
      DEBUG_LOG(3, "[PORT][TX_CB] DONE cnt=%lu st=0x%lx\r\n", (unsigned long)transferCount, (unsigned long)transferStatus);

  }else{ /* Neu chua du len byte tu UARTDRV_Transmit() */

      if(s_tx_state == UART_TIMEOUT){ // Neu bi set la TIMEOUT
          DEBUG_LOG(2, "[PORT][TX_CB] POST-TIMEOUT cnt=%lu st=0x%lx\r\n", (unsigned long)transferCount, (unsigned long)transferStatus);
          return;
      }

      s_tx_state = UART_ERROR; // Gan ngay state la ERROR
      DEBUG_LOG(1, "[PORT][TX_CB] ERROR cnt=%lu st=0x%lx\r\n", (unsigned long)transferCount, (unsigned long)transferStatus);
  }
}

/* ----------------------------------------------------------- */

static void uart_efr32_rx_callback(UARTDRV_HandleData_t *handle,
                                   Ecode_t transferStatus,
                                   uint8_t *data,
                                   UARTDRV_Count_t transferCount){
  (void)(handle);
  (void)(data);
  s_rx_status = transferStatus;

  /* NEW: Abort 1 transaction co chu dich (sau khi nhan du byte yeu cau) -> coi nhu DONE toan bo */
  if(s_rx_state == UART_ABORT_OK){ // Neu duoc set la ABORT_OK
      s_rx_state = UART_DONE;
      DEBUG_LOG(2, "[PORT][RX_CB] ABORT-OK cnt=%lu\r\n", (unsigned long)transferCount);
      return;
  }

  /* Neu nhan du len byte yeu cau tu UARTDRV_Receive() */
  if(transferStatus == ECODE_OK || transferStatus == ECODE_EMDRV_DMADRV_OK){
      s_rx_state = UART_DONE; // Gan ngay state la DONE
      DEBUG_LOG(3, "[PORT][RX_CB] DONE cnt=%lu st=0x%lx\r\n", (unsigned long)transferCount, (unsigned long)transferStatus);

  }else{ /* Neu chua du len byte yeu cau tu UARTDRV_Receive() */

      if(s_rx_state == UART_TIMEOUT){ // Neu bi set la TIMEOUT
          DEBUG_LOG(2, "[PORT][RX_CB] POST-TIMEOUT cnt=%lu st=0x%lx\r\n", (unsigned long)transferCount, (unsigned long)transferStatus);
          return;
      }

      s_rx_state = UART_ERROR; // Gan ngay state la ERROR
      DEBUG_LOG(1, "[PORT][RX_CB] ERROR cnt=%lu st=0x%lx\r\n", (unsigned long)transferCount, (unsigned long)transferStatus);
  }
}

#endif // UART_NON_BLOCKING_MODE

/* ----------------------------------------------------------- */

/**
 * @details
 * Voi EFR32 thuong dung UARTDRV instance duoc tao san trong project
 * Voi moi thong so va tinh nang da cau hinh trong GUI thi he thong da sinh het code roi
 * khi chuong trinh chay, UARTDRV se duoc init tai sl_system_init() trong main.c
 * Code nay chi lay handle cau hinh hien tai cua he thong va sao chep no sang
 *
 * Ham nay chi:
 *  - Kiem tra handle hop le
 *  - (Tuy chon) set baudrate seu SDK ho tro
 *  - Flush RX de sach buffer truoc khi giao tiep
 */
bool zw111_port_uart_init(uint32_t baudrate, const void *port_cfg, uint32_t port_cfg_size){

/* Neu thich tu cau hinh */
#ifdef USER_PORT_UART_INIT
  if(port_cfg == NULL || port_cfg_size != sizeof(zw111_port_efr32_cfg_t)) return false;

  // Copy cau hinh cua Port EFR32
  zw111_port_efr32_cfg_t *cfg = (zw111_port_efr32_cfg_t*)port_cfg;

  if(cfg->handle == NULL) return false;

  // Copy cau hinh UART da thiet lap tu USER
  UARTDRV_InitUart_t init_local = cfg->init;

  if(baudrate != 0){
      if(baudrate < SL_UARTDRV_USART_USART_UART2_BAUDRATE) init_local.baudRate = SL_UARTDRV_USART_USART_UART2_BAUDRATE;
      else init_local.baudRate = baudrate;
  }

  if(cfg->isUART_init == false){

    /* Mac dinh trong `UARTDRV_InitUart() da thiet lap san cac thong so phan cung mac dinh roi (default) */
    Ecode_t ret = UARTDRV_InitUart(cfg->handle, &init_local);

    if(ret != ECODE_EMDRV_UARTDRV_OK && ret != ECODE_OK) return false;
  }

  cfg->isUART_init = true;

  // Copy Handle da duoc khoi tao tu USER
  zw111_port_efr32_set_handle(cfg->handle);

/* Dung luon API da san cua SliconLabs */
#elif defined(SL_PORT_UART_INIT)

  (void)(baudrate);
  (void)(port_cfg);
  (void)(port_cfg_size);

  /* Dam bao USARTDRV da duoc Init boi he thong */
  UARTDRV_Handle_t sl_handle = sl_uartdrv_get_default();
  if(sl_handle == NULL){
      return false;
  }
  /* NOTE: Ham khoi tao Driver da duoc lam trong ham sl_system_init() cua main.c */
  if(zw111_port_efr32_set_handle(sl_handle) != true) return false;

#endif
  return true;
}

/* ----------------------------------------------------------- */

bool zw111_port_uart_deinit(void){
  if(uart_efr32_handle == NULL) return false;

  if(UARTDRV_DeInit(uart_efr32_handle) == ECODE_EMDRV_UARTDRV_OK){
      uart_efr32_handle = NULL;
      return true;
  }
  return false;
}

/* ----------------------------------------------------------- */

EFR32_PLATFORM_TAG bool zw111_port_efr32_set_handle(UARTDRV_Handle_t uart_handle){
  if(uart_handle != NULL){
      uart_efr32_handle = uart_handle;
      return true;
  }
  return false;
}

/* ----------------------------------------------------------- */

#ifdef USER_PORT_UART_INIT
EFR32_PLATFORM_TAG void zw111_port_efr32_default_cfg(zw111_port_efr32_cfg_t *cfg){
  if(cfg == NULL) return;
  memset(cfg, 0, sizeof(*cfg));

  static UARTDRV_Buffer_FifoQueue_t rxQueue;
  static UARTDRV_Buffer_FifoQueue_t txQueue;

  cfg->handle = &s_uart_handle;

  cfg->init.baudRate = SL_UARTDRV_USART_USART_UART2_BAUDRATE;
  cfg->init.port = USART2;
  cfg->init.stopBits = SL_UARTDRV_USART_USART_UART2_STOP_BITS;
  cfg->init.parity = SL_UARTDRV_USART_USART_UART2_PARITY;
  cfg->init.oversampling = SL_UARTDRV_USART_USART_UART2_OVERSAMPLING;
  cfg->init.fcType = SL_UARTDRV_USART_USART_UART2_FLOW_CONTROL_TYPE;
  cfg->init.rxQueue = &rxQueue;
  cfg->init.txQueue = &txQueue;

  cfg->isUART_init = false;
}
#endif // USER_PORT_UART_INIT

/* ----------------------------------------------------------- */

bool zw111_port_uart_tx(const uint8_t *buf, uint16_t len){
  if(uart_efr32_handle == NULL || buf == NULL || len == 0) return false;
  if(s_tx_state == UART_BUSY) return false;

  s_tx_state = UART_BUSY; // state == BUSY khi bat dau 1 transaction
  s_tx_status = ECODE_OK;

#if defined(UART_BLOCKING_MODE)

  Ecode_t ret = UARTDRV_TransmitB(uart_efr32_handle, (uint8_t*)buf, (UARTDRV_Count_t)len);
  if(ret != ECODE_OK && ret != ECODE_EMDRV_UARTDRV_OK){
      s_tx_state = UART_ERROR;
      s_tx_status = ret;
      return false;
  }
  s_tx_state = UART_DONE; // Chi blocking mode moi set duoc
  return true;

#elif defined(UART_NON_BLOCKING_MODE)

  s_tx_start_kick = zw111_port_get_ticks();
  Ecode_t ret = UARTDRV_Transmit(uart_efr32_handle, (uint8_t*)buf, (UARTDRV_Count_t)len, uart_efr32_tx_callback);

  if(ret != ECODE_OK && ret != ECODE_EMDRV_UARTDRV_OK){
      s_tx_state = UART_ERROR;
      s_tx_status = ret;
      return false;
  }
  return true;
#endif // ...
}

/* ----------------------------------------------------------- */

bool zw111_port_uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms){
  if(uart_efr32_handle == NULL || buf == NULL || len == 0) return false;
  if(s_rx_state == UART_BUSY) return false;
  (void)timeout_ms; // Ham nay chi kick RX, khong xu ly timeout

  s_rx_state = UART_BUSY; // state == BUSY khi bat dau 1 transaction
  s_rx_status = ECODE_OK; // Default ban dau la OK

#if defined(UART_BLOCKING_MODE)

  Ecode_t ret = UARTDRV_ReceiveB(uart_efr32_handle, buf, (UARTDRV_Count_t)len);
  if(ret != ECODE_OK && ret != ECODE_EMDRV_UARTDRV_OK){
      s_rx_state = UART_ERROR;
      s_rx_status = ret;
      return false;
  }
  s_rx_state = UART_DONE; // Chi blocking mode moi set duoc (vi khong co ham callback)
  return true;

#elif defined(UART_NON_BLOCKING_MODE)

  s_rx_start_kick = zw111_port_get_ticks();
  s_rx_need_bytes = 0;
  Ecode_t ret = UARTDRV_Receive(uart_efr32_handle, buf, (uint32_t)len, uart_efr32_rx_callback);

  if(ret != ECODE_OK && ret != ECODE_EMDRV_UARTDRV_OK){
      s_rx_state = UART_ERROR;
      s_rx_status = ret;
      return false;
  }
  return true;
#endif // ...
}

/* ----------------------------------------------------------- */

#if defined(UART_NON_BLOCKING_MODE)

zw111_port_uart_state_t zw111_port_uart_tx_poll(uint32_t timeout_ms){
  if(s_tx_state != UART_BUSY) return s_tx_state;

  if(timeout_ms > 0){
      uint32_t now = zw111_port_get_ticks(); // Ticks
      uint32_t dt = elapsed_ticks(s_tx_start_kick, now);

      /* Chuyen tu ms -> ticks */
      uint32_t timeout_ticks = sl_sleeptimer_ms_to_tick(timeout_ms);
      if(timeout_ticks == 0) timeout_ticks = 1; // clamp

      /* Neu thoi gian nhan qua lau -> loi timeout */
      if(dt > timeout_ticks){

          /* DEBUG START */
          UARTDRV_Count_t txCount = 0, txRemaining = 0;
          uint8_t *p = NULL;
          UARTDRV_Status_t ret = UARTDRV_GetTransmitStatus(uart_efr32_handle, &p, &txCount, &txRemaining);
          DEBUG_LOG(2, "[PORT][TX_POLL] GetTransmitStatus status=0x%lx p=%p count=%lu rem=%lu dt_ticks=%lu timeout_ticks=%lu\r\n",
                                                         (unsigned long)ret,
                                                         (void*)p,
                                                         (unsigned long)txCount,
                                                         (unsigned long)txRemaining,
                                                         (unsigned long)dt,
                                                         (unsigned long)timeout_ticks);
          /* DEBUG END */
          s_tx_state = UART_TIMEOUT; // TX Callback tra ve timeout
          UARTDRV_Abort(uart_efr32_handle, uartdrvAbortTransmit); // End transaction
      }
  }
  return s_tx_state;
}

/* ----------------------------------------------------------- */

zw111_port_uart_state_t zw111_port_uart_rx_poll(uint32_t timeout_ms){
  if(s_rx_state != UART_BUSY) return s_rx_state;

  // NEW: Neu co yeu cau "du N bytes" thi kiem tra rxCount
  if(s_rx_need_bytes > 0){
      UARTDRV_Count_t rxCount = 0, rxRemaining = 0;
      uint8_t *p = NULL;
      UARTDRV_GetReceiveStatus(uart_efr32_handle, &p, &rxCount, &rxRemaining);

      // DONE logic khi du N bytes (khong Abort transaction, khong doi state)
      if(rxCount >= s_rx_need_bytes){
          return UART_DONE; // DONE ao coi nhu da doc xong N bytes de thoat wait_rx_done()
      }
  }

  if(timeout_ms > 0){
      uint32_t now = zw111_port_get_ticks(); // Ticks
      uint32_t dt = elapsed_ticks(s_rx_start_kick, now);

      /* Chuyen tu ms -> ticks */
      uint32_t timeout_ticks = sl_sleeptimer_ms_to_tick(timeout_ms);
      if(timeout_ticks == 0) timeout_ticks = 1; // clamp

      /* Neu thoi gian nhan qua lau -> loi timeout */
      if(dt > timeout_ticks){

          /* DEBUG START */
          /**
           * @note (debug khi timeout)
           * Khi phat hien timeout, ham se snapshot trang thai tien trinh RX tu UARTDRV de phuc vu debug:
           * - Dung `UARTDRV_GetReceiveStatus(handle, &p, &rxCount, &rxRemaining)`, trong do:
           *   + p: Con tro buffer ma driver dang fill (thuong phai trung voi buffer da kick)
           *   + rxCount: So byte da nhan vao buffer
           *   + rxRemaining: So byte con thieu de du len -> DONE
           *
           * Log debug giup xac dinh:
           *  - Co nhan duoc byte nao khong (rxCount > 0)
           *  - Dang thieu byte nao (rxRemaining)
           *  - Driver co dang fill dung buffer (p == buf kick) hay bi lech/race
           */
          UARTDRV_Count_t rxCount = 0, rxRemaining = 0;
          uint8_t *p = NULL;
          UARTDRV_Status_t ret = UARTDRV_GetReceiveStatus(uart_efr32_handle, &p, &rxCount, &rxRemaining);
          DEBUG_LOG(2, "[PORT][RX_POLL] GetReceiveStatus status=0x%lx p=%p count=%lu rem=%lu dt_ticks=%lu timeout_ticks=%lu\r\n",
                                                         (unsigned long)ret,
                                                         (void*)p,
                                                         (unsigned long)rxCount,
                                                         (unsigned long)rxRemaining,
                                                         (unsigned long)dt,
                                                         (unsigned long)timeout_ticks);
          /* DEBUG END */
          s_rx_state = UART_TIMEOUT; // RX Callback tra ve timeout
          UARTDRV_Abort(uart_efr32_handle, uartdrvAbortReceive); // End transaction
      }
  }
  return s_rx_state; // Khi nay van la UART_BUSY
}
#endif // UART_NON_BLOCKING_MODE

/* ----------------------------------------------------------- */

/**
 * @brief Chi xu ly trang thai Poll RX tra ve
 *
 * @note
 * Thoi gian timeout duoc tinh tu luc kick RX (goi ham `zw111_port_uart_rx())
 * cho den luc ham Poll nay duoc goi
 * Ham nay la wrapper cho rx_poll() vi de tach bach trang thai tra ve cua transaction
 * voi trang thai tra ve cua ham
 *
 * @param timeout_ms Thoi gian (dvi: ms) (void)
 * @return
 * - ZW111_STATUS_OK - Neu RX hoan tat (UART_DONE)
 * - ZW111_STATUS_ERROR - Neu RX loi (UART_ERROR)
 * - ZW111_STATUS_TIMEOUT - Neu RX timeout (UART_TIMEOUT)
 */
static inline zw111_status_t wait_rx_done(uint32_t timeout_ms){
  while(1){
      zw111_port_uart_state_t ret = zw111_port_uart_rx_poll(timeout_ms);

      if(ret == UART_DONE) return ZW111_STATUS_OK;
      if(ret == UART_ERROR){
          DEBUG_LOG(1, "[PORT] RX done Error...\r\n"); // Debug
          return ZW111_STATUS_ERROR;
      }

      if(ret == UART_TIMEOUT){
          DEBUG_LOG(1, "[PORT] RX done Timeout...\r\n"); // Debug
          return ZW111_STATUS_TIMEOUT;
      }
  }
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_port_uart_wait_rx_reach(uint16_t need_bytes, uint32_t timeout_ms){
  s_rx_need_bytes = need_bytes;
  zw111_status_t ret = wait_rx_done(timeout_ms);
  s_rx_need_bytes = 0; // Clear ngay sau khi return
  return ret;
}

/* ----------------------------------------------------------- */

zw111_status_t zw111_port_uart_abort_rx_ok(uint32_t timeout_ms){
  if(s_rx_state != UART_BUSY) return ZW111_STATUS_ERROR;

  s_rx_state = UART_ABORT_OK; // Chuyen tu BUSY -> ABORT_OK de hoan thanh transaction
  UARTDRV_Abort(uart_efr32_handle, uartdrvAbortReceive);

  // cho callback doi sang UART_DONE (callback da co xu ly UART_ABORT_OK)
  uint32_t start = zw111_port_get_ticks();
  uint32_t timeout_ticks = sl_sleeptimer_ms_to_tick(timeout_ms);
  if(timeout_ticks == 0) timeout_ticks = 1;

  while(1){
      if(s_rx_state == UART_DONE) return ZW111_STATUS_OK; // Neu RX callback tra ve ngay DONE sau khi set ABORT_OK
      if(s_rx_state == UART_ERROR) return ZW111_STATUS_ERROR;

      uint32_t now = zw111_port_get_ticks();
      if(elapsed_ticks(start, now) > timeout_ticks) return ZW111_STATUS_TIMEOUT;
  }
}

/* ----------------------------------------------------------- */

bool zw111_port_uart_flush(void){
  if(uart_efr32_handle == NULL) return false;

  /* Hoi trang thai RX truoc de tranh Abort mu */
  UARTDRV_Count_t rxCount = 0, rxRemaining = 0;
  uint8_t *p = NULL;
  UARTDRV_GetReceiveStatus(uart_efr32_handle, &p, &rxCount, &rxRemaining);

  // Neu co byte ton tai
  if(rxRemaining > 0){

      /* Abort la viec dung 1 Transaction dang chay, neu khong co transaction (UART dang idle), Abort fail khong phai loi */
      if(UARTDRV_Abort(uart_efr32_handle, uartdrvAbortReceive) == ECODE_EMDRV_UARTDRV_OK) return true;
  }
  return true;
}

/* ----------------------------------------------------------- */

bool zw111_port_uart_ready(void){
  return (uart_efr32_handle != NULL);
}

/* ----------------------------------------------------------- */

void zw111_port_delay_ms(uint32_t ms){
  sl_sleeptimer_delay_millisecond(ms);
}

/* ----------------------------------------------------------- */

uint32_t zw111_port_get_ticks(void){
  return sl_sleeptimer_get_tick_count();
}

/* ----------------------------------------------------------- */

#endif // EFR32_PLATFORM

#ifdef __cplusplus
}
#endif // __cplusplus
