/*
 * @file zw111_app.c
 *
 * @date 27 thg 12, 2025
 * @author: LuongHuuPhuc
 *
 * Dinh nghia API Application Layer trong Zigbee AF cho USER
 * Goi cac API cap thap (`zw111_port, zw111_lowlevel, zw111` de xu ly logic)
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "zw111_app.h"
#include "zw111_port_select.h"

#ifdef EFR32_PLATFORM
#include "app/framework/include/af.h"

/* Bien luu trang thai */
static zw111_app_state_t s_state = ZW111_APP_IDLE;

/* Bien luu timing cho lenh phan hoi (lau qua se timeout) */
static uint32_t s_state_enter_tick = 0;

/* Bien luu so lan thu so khop */
static uint8_t s_match_try = 0;

/* Bien luu so lan thu lai khi enroll van tay moi */
static uint8_t s_enroll_try = 0;

/* ----------------------------------------------------------- */

/**
 * @brief Ham thuc hien phep gan trang thai hoat logic dong cua
 * chuong trinh vao bien toan cuc `s_state` de chuong trinh biet
 * can lam gi tiep theo
 * Dong thoi gan thoi gian moi lan vao ham nay nham muc dich so sanh
 * tre cho cac logic yeu cau timeout
 *
 * @param st Trang thai dau vao
 */
static inline void zw111_app_enter_state(zw111_app_state_t st){
  s_state = st;
  s_state_enter_tick = zw111_ll_get_ticks();
}

/* ----------------------------------------------------------- */

zw111_app_state_t zw111_app_uart_init(uint32_t baudrate, uint32_t timeout_ms, uint32_t password){

#ifdef USER_PORT_UART_INIT

  /* Thiet lap thong so phan cung UART qua struct rieng cua EFR32 */
  zw111_port_efr32_cfg_t port_cfg;
  zw111_port_efr32_default_cfg(&port_cfg);

  /* Struct luu cau hinh phan cung cho giao thuc cua ZW111 sau khi da thiet lap ban dau theo dung Platform dang dung */
  zw111_cfg_t cfg = {
      .baud = baudrate,
      .timeout_ms = timeout_ms,
      .password = password,
      .port_cfg = &port_cfg,
      .port_cfg_size = sizeof(port_cfg)
  };

#endif // USER_PORT_UART_INIT

  zw111_cfg_t cfg = {
        .baud = baudrate,
        .timeout_ms = timeout_ms,
        .password = password,
        .port_cfg = NULL, // EFR32 da duoc khoi tao boi he thong nen khong can USER cau hinh
        .port_cfg_size = 0
   };

  if(zw111_uart_init(&cfg) != ZW111_STATUS_OK){
      emberAfCorePrintln("[ZW111] UART initialized for ZW111 failed...");
      return ZW111_APP_ERROR;
      EFM_ASSERT(false);
  }
  emberAfCorePrintln("[ZW111] UART initialized for ZW111 OK !");
  return ZW111_APP_PROBE;
}

/* ----------------------------------------------------------- */

zw111_app_state_t zw111_app_sensor_probe(void){
  zw111_sysinfo_t info;
  emberAfCorePrintln("[ZW111] Probing sensor...");

  if(zw111_read_sysinfo(&info) != ZW111_STATUS_OK){
      emberAfCorePrintln("[ZW111] Probe FAILED");
      return ZW111_APP_ERROR;
  }

  emberAfCorePrintln("[ZW111] Probe OK | Addr: 0x%08X | Capacity: %d", info.device_address, info.database_capacity);
  emberAfCorePrintln("[ZW111] Sensor is waiting finger...");

  return ZW111_APP_WAIT_FINGER;
}

/* ----------------------------------------------------------- */

zw111_app_state_t zw111_app_process(void){
  zw111_status_t ret; /* Bien luu ket qua tra ve API Application Layer cua cam bien */
  zw111_app_state_t ret_app; /* Bien luu ket qua tra ve API tai Zigbee AF */
  uint32_t now = zw111_ll_get_ticks();

  switch(s_state){
    case ZW111_APP_IDLE:
      /* Khong tu init o day de tranh vong lap
       * Nen goi o ham khoi tao tai emberAfMainInitCallback trong app.c */
    break;

    case ZW111_APP_PROBE:
      ret_app = zw111_app_sensor_probe();
      if(ret_app != ZW111_APP_WAIT_FINGER){
          zw111_app_enter_state(ZW111_APP_ERROR);
          break;
      }
      zw111_app_enter_state(ret_app);
    break;

    /* ===================== MATCH FLOW (XAC THUC VAN TAY) ===================== */

    /* ------- 1. WAIT FINGER: Cho USER dat ngon tay vao cam bien */
    case ZW111_APP_WAIT_FINGER:
      /* Timeout cho finger */
      if(elapsed_ms(s_state_enter_tick, now) > ZW111_APP_TIMEOUT_GET_IMAGE_MS){
          emberAfCorePrintln("[ZW111] WAIT FINGER timeout");
          zw111_app_enter_state(ZW111_APP_WAIT_FINGER); /* Reset timer */
          break;
      }

      /* Thuc hien lay Image tu ngon tay USER */
      ret = zw111_get_image();
      if(ret == ZW111_STATUS_OK){
          emberAfCorePrintln("[ZW111] >>> GET IMAGE done, about to GEN CHAR... ");
          zw111_app_enter_state(ZW111_APP_GEN_CHAR);
      }
      else if(ret == ZW111_STATUS_NO_FINGER) { /* Cho tiep */ }
      else{
          emberAfCorePrintln("[ZW111] GET_IMAGE error=0x%02X", ret);
          zw111_app_enter_state(ZW111_APP_ERROR);
      }
     break;

    /* ------- 2. GEN CHAR: Thuc hien trich xuat dac trung van tay */
    case ZW111_APP_GEN_CHAR:
      ret = zw111_gen_char(ZW111_CHARBUFFER_1);
      if(ret == ZW111_STATUS_OK){
          emberAfCorePrintln("[ZW111] >>> GEN_CHAR OK");
          zw111_app_enter_state(ZW111_APP_MATCH);
      }else{
          emberAfCorePrintln("[ZW111] GEN_CHAR error=0x%02X", ret);
          zw111_app_enter_state(ZW111_APP_ERROR);
      }
    break;

    /* ------- 3. MATCH: Thuc hien so khop van tay voi du lieu co trong database */
    case ZW111_APP_MATCH:{
      uint16_t score = 0;
      ret = zw111_match(&score);

      if(ret == ZW111_STATUS_OK){
          emberAfCorePrintln("[ZW111] >>> MATCH OK score=%d", score);

          if(score >= ZW111_APP_MATCH_SCORE_MIN){
              emberAfCorePrintln("[ZW111] >>> ACCEPT ");
              zw111_app_enter_state(ZW111_APP_DONE);
              /* TODO: Them logic dong mo cua va gui lenh vao mang Zigbee */
          }
      }else if(ret == ZW111_STATUS_MATCH_FAIL){
          emberAfCorePrintln("[ZW111] MATCH FAIL");
          s_match_try++;

          if(s_match_try >= 3){
              s_match_try = 0;
              emberAfCorePrintln("[ZW111] MATCH FAIL over 3 times, back to WAIT FINGER");
              zw111_app_enter_state(ZW111_APP_WAIT_FINGER); /* Reset timer */
          }
      }else{
          emberAfCorePrintln("[ZW111] MATCH error=0x%02X", ret);
          zw111_app_enter_state(ZW111_APP_ERROR);
      }
    }
    break;

    /* ===================== ENROLL FLOW (DANG KY VAN TAY MOI) ===================== */

    /* ------- 1. Enroll step 1: GetImage + GenChar(CharBuffer1) */
    case ZW111_APP_ENROLL_STEP1:
      ret = zw111_enroll_step1();
      if(ret == ZW111_STATUS_OK){
          s_enroll_try = 0;
          emberAfCorePrintln("[ZW111] ENROLL STEP1 OK");
          zw111_app_enter_state(ZW111_APP_ENROLL_STEP2);

      }else if(ret == ZW111_STATUS_NO_FINGER) { /* Cho tiep */ }

      else{
          s_enroll_try++;
          if(s_enroll_try >= ZW111_APP_ENROLL_MAX_TRIES){
              emberAfCorePrintln("[ZW111] ENROLL STEP1 failed (max tries), return last error=0x%02X", ret);
              zw111_app_enter_state(ZW111_APP_ERROR);
          }
      }
    break;

    /* ------- 2. Enroll step 2: GetImage + GenChar(CharBuffer2) + RegModel */
    case ZW111_APP_ENROLL_STEP2:
      ret = zw111_enroll_step2();
      if(ret == ZW111_STATUS_OK){
          s_enroll_try = 0;
          emberAfCorePrintln("[ZW111] ENROLL STEP2 OK");
          zw111_app_enter_state(ZW111_APP_ENROLL_STORE);
      }
      else{
          s_enroll_try++;
          if(s_enroll_try >= ZW111_APP_ENROLL_MAX_TRIES){
              emberAfCorePrintln("[ZW111] ENROLL STEP2 failed (max tries), return last error=0x%02X", ret);
              zw111_app_enter_state(ZW111_APP_ERROR);
          }
      }
    break;

    /* ------- 3. Store Enrolled Fingerprint: StoreChar(page_id) */
    case ZW111_APP_ENROLL_STORE:
      ret = zw111_enroll_store();
      if(ret == ZW111_STATUS_OK){
          emberAfCorePrintln("[ZW111] ENROLL STORE OK");
          zw111_app_enter_state(ZW111_APP_DONE);
      }else{
          emberAfCorePrintln("[ZW111] ENROLL STORE error=0x%02X", ret);
          zw111_app_enter_state(ZW111_APP_ERROR);
      }
    break;

    /* ===================== DONE ===================== */
    case ZW111_APP_DONE:
      emberAfCorePrintln("[ZW111] APP DONE state...");
      /* Xu ly tac vu khac tai app.c (gui Zibgee, log, LED) */
    break;

    /* ===================== ERROR ===================== */
    case ZW111_APP_ERROR:
    default:
      emberAfCorePrintln("[ZW111] APP ERROR state...");
      /* Dung tai day, khong back ve IDLE, quyet dinh se duoc thuc hien tai app.c */
    break;
  }
  return s_state;
}

/* ----------------------------------------------------------- */

void zw111_app_start(void){
  if(s_state == ZW111_APP_IDLE){
      zw111_app_enter_state(ZW111_APP_PROBE);
  }
}

/* ----------------------------------------------------------- */

void zw111_app_start_enroll(uint16_t page_id){
  s_match_try = 0;
  s_enroll_try = 0;
  zw111_enroll_start(page_id);
  zw111_app_enter_state(ZW111_APP_ENROLL_STEP1);
}

/* ----------------------------------------------------------- */

zw111_app_state_t zw111_app_get_state(void){
  return s_state;
}

/* ----------------------------------------------------------- */

#endif // EFR32_PLATFORM

#ifdef STM32_PLATFORM
/* */
#endif // STM32_PLATFORM

#ifdef ESP32_PLATFORM
/* */
#endif // ESP32_PLATFORM

#ifdef __cplusplus
}
#endif // __cplusplus
