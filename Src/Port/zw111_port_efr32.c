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
#include "../../../config/sl_uartdrv_usart_USART_UART1_config.h"
#include "../../../autogen/sl_uartdrv_instances.h"
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

#ifdef UART_NON_BLOCKING_MODE

/* Co trang thai de biet TX/RX da xong hay chua */
static volatile bool s_tx_done = false;
static volatile bool s_rx_done = false;

/* Luu trang thai tra ve tu Callback */
static volatile Ecode_t s_tx_status = ECODE_OK;
static volatile Ecode_t s_rx_status = ECODE_OK;

static void uart_efr32_tx_callback(UARTDRV_HandleData_t *handle,
                              Ecode_t transferStatus,
                              uint8_t *data,
                              UARTDRV_Count_t transferCount){
  (void)(handle);
  (void)(data);
  (void)(transferCount);
  s_tx_status = transferStatus;
  s_tx_done = true;
}

/* ----------------------------------------------------------- */

static void uart_efr32_rx_callback(UARTDRV_HandleData_t *handle,
                              Ecode_t transferStatus,
                              uint8_t *data,
                              UARTDRV_Count_t transferCount){
  (void)(handle);
  (void)(data);
  (void)(transferCount);
  s_rx_status = transferStatus;
  s_rx_done = true;
}

#endif // UART_NON_BLOCKING_MODE

/* ----------------------------------------------------------- */

/**
 * @details
 * Voi EFR32 thuong dung UARTDRV instance duoc tao san trong project
 * Voi moi thong so va tinh nang da cau hinh trong GUI thi he thong da sinh het code roi
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
  const zw111_port_efr32_cfg_t *cfg = (const zw111_port_efr32_cfg_t*)port_cfg;

  if(cfg->handle == NULL) return false;

  // Copy cau hinh UART da thiet lap tu USER
  UARTDRV_InitUart_t init_local = cfg->init;

  if(baudrate != 0){
      if(baudrate < SL_UARTDRV_USART_USART_UART1_BAUDRATE) init_local.baudrate = SL_UARTDRV_USART_USART_UART1_BAUDRATE;
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
  UARTDRV_Handle_t sl_handle = sl_uartdrv_usart_USART_UART1_handle;
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

bool zw111_port_efr32_set_handle(UARTDRV_Handle_t uart_handle){
  if(uart_handle != NULL){
      uart_efr32_handle = uart_handle;
      return true;
  }
  return false;
}

/* ----------------------------------------------------------- */

#ifdef USER_PORT_UART_INIT
void zw111_port_efr32_default_cfg(zw111_port_efr32_cfg_t *cfg){
  if(cfg == NULL) return;
  memset(cfg, 0, sizeof(*cfg));

  static UARTDRV_Buffer_FifoQueue_t rxQueue;
  static UARTDRV_Buffer_FifoQueue_t txQueue;

  cfg->handle = &s_uart_handle;

  cfg->init.baudRate = SL_UARTDRV_USART_USART_UART1_BAUDRATE;
  cfg->init.port = USART1;
  cfg->init.stopBits = SL_UARTDRV_USART_USART_UART1_STOP_BITS;
  cfg->init.parity = SL_UARTDRV_USART_USART_UART1_PARITY;
  cfg->init.oversampling = SL_UARTDRV_USART_USART_UART1_OVERSAMPLING;
  cfg->init.fcType = SL_UARTDRV_USART_USART_UART1_FLOW_CONTROL_TYPE;
  cfg->init.rxQueue = &rxQueue;
  cfg->init.txQueue = &txQueue;

  cfg->isUART_init = false;
}
#endif // USER_PORT_UART_INIT

/* ----------------------------------------------------------- */

void zw111_port_uart_tx(const uint8_t *buf, uint16_t len){
  if(uart_efr32_handle == NULL || buf == NULL || len == 0) return;

#if defined(UART_BLOCKING_MODE)
  UARTDRV_TransmitB(uart_efr32_handle, (uint8_t*)buf, (UARTDRV_Count_t)len);
#elif defined(UART_NON_BLOCKING_MODE)
  s_tx_done = false;
  s_tx_status = ECODE_OK;
  (void)UARTDRV_Transmit(uart_efr32_handle, (uint8_t*)buf, (UARTDRV_Count_t)len, uart_efr32_tx_callback);

  /* Cho TX xong (thuong khong can timeout o TX, timeout hay dung cho RX */
  while (!s_tx_done) {
    /* Neu co RTOS, co the yield/sleep o day */
  }
  (void)s_tx_status;
#endif
}

/* ----------------------------------------------------------- */

uint32_t zw111_port_uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms){
  if(uart_efr32_handle == NULL || buf == NULL || len == 0) return 0;

#if defined(UART_BLOCKING_MODE)
  (void)timeout_ms;
  Ecode_t ret = UARTDRV_ReceiveB(uart_efr32_handle, buf, (UARTDRV_Count_t)len);
  return (ret == ECODE_OK || ret == ECODE_EMDRV_UARTDRV_OK) ? (uint32_t)len : 0;

#elif defined(UART_NON_BLOCKING_MODE)
  s_rx_done = false;
  s_rx_status = ECODE_OK;
  Ecode_t ret = UARTDRV_Receive(uart_efr32_handle, buf, (uint32_t)len, uart_efr32_rx_callback);

  if(ret != ECODE_OK && ret != ECODE_EMDRV_UARTDRV_OK) return 0;

  uint32_t start = zw111_port_get_ticks();
  while(!s_rx_done){
      if(timeout_ms >0){
          uint32_t now =  zw111_port_get_ticks();
          if(elapsed_ms(start, now) >= timeout_ms){
              /* Timeout: Hoi xem da nhan duoc bao nhieu byte */
              UARTDRV_Count_t got = 0, remaining = 0;
              UARTDRV_GetReceiveStatus(uart_efr32_handle, buf, &got, &remaining);

              /* Abort receive de dung transaction */
              UARTDRV_Abort(uart_efr32_handle, uartdrvAbortReceive);
              return (uint32_t)got;
          }
      }
  }
  return (uint32_t)len;
#endif
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

bool zw111_port_uart_flush(void){
  if(uart_efr32_handle == NULL) return false;

  Ecode_t ret = UARTDRV_Abort(uart_efr32_handle, uartdrvAbortReceive);
  if(ret != ECODE_EMDRV_UARTDRV_OK && ret != ECODE_OK) return false;
  return true;
}

/* ----------------------------------------------------------- */

bool zw111_port_uart_ready(void){
  return (uart_efr32_handle != NULL);
}

#endif // EFR32_PLATFORM

#ifdef __cplusplus
}
#endif // __cplusplus
