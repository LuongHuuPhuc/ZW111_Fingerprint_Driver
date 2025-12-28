# I. TỔNG QUAN VỀ CẢM BIẾN VÂN TAY ZW111   

![Sensor image](../Images/ZW111sensor.png)
![Datasheet ZW111](https://www.elemon.com.ar/images/productos/hojas-de-datos/sensores-huella-dactilar/zw111.pdf)
![Datasheet Fingerprint Sensor Module User Manual](https://cdn.sparkfun.com/assets/5/a/3/f/c/Module_User_Manual-17151-Capacitive_Fingerprint_Scanner_-_AS-108M.pdf)

## 1. ZW111 không phải là cảm biến 

- **ZW111** là một module cảm biến vân tay hoàn chỉnh, không chỉ là một sensor quét ảnh mà còn là hệ thống nhúng mini bên trong 
- Bên trong cảm biến ZW111 có 4 khối lớn: 

```css
┌─────────────────────────────┐
│  Optical Fingerprint Sensor │  ← phần quang học
├─────────────────────────────┤
│  MCU / DSP nội bộ            │  ← xử lý ảnh & thuật toán
├─────────────────────────────┤
│  Flash / ROM / RAM           │  ← lưu template + firmware
├─────────────────────────────┤
│  UART Communication Engine   │  ← giao tiếp với MCU ngoài
└─────────────────────────────┘

```
## 2. Vì sao ZW111 khác cảm biến thường ? 

| Cảm biến thường    | ZW111                          |
| ------------------ | ------------------------------ |
| Trả về raw data    | Trả về **kết quả logic**       |
| MCU ngoài xử lý    | **MCU trong module xử lý**     |
| Không nhớ dữ liệu  | **Có Flash lưu template**      |
| Giao tiếp đơn giản | **Protocol phức tạp (packet)** |

=> Không xử lý ảnh vân, chỉ gửi lệnh, module tự xử lý

## 3. Cấu trúc bên trong (theo datasheet)
### 3.1. Optical Sensor (phần quang học)
- Loại: Optical fingerprint sensor 
- Nguyên lý: 
   - LED chiếu sáng ngón tay
   - Camera CMOS chụp ảnh vân tay
   - Dựa vào sự khác biệt phản 

### 3.2. MCU/DSP nội bộ 
- Đây là quan trọng nhất
- MCU nội bộ chịu trách nhiệm: 
   - Capture Image
   - Extract feature 
   - Generate Character file 
   - Match/Search
   - Manage 
- Toàn bộ thuật toán đóng kín

### 3.3. ROM/Flash/RAM 
- ROM:
   - Chứa firmware gốc 
   - Chứa thuật toán nhận dạng
   - Người dùng không sửa được 

- Flash (Template Database)
   - Lưu vân tay đã enroll
   - NotePad (User Data)
   - Mỗi vân tay = 1 template
   - Có Page ID

- RAM: 
   - Buffer ảnh 
   - Buffer Charfile1/Charfile2

## 4. Vì sao lại có Buffer1/Buffer2
- ZW111 không xử lý ảnh trực tiếp trên flash
- Flow chuẩn: 

```sql
GetImage
   ↓
GenChar → CharBuffer1
   ↓
GenChar → CharBuffer2
   ↓
Match / RegModel

```
- Buffer là RAM nội bộ, không phải MCU ngoài

## 5. Vì sao driver phải dùng Command/Instruction Set + ACK 
- Vì ZW111 là thiết bị chủ 

| MCU ngoài             | ZW111          |
| --------------------- | -------------- |
| Gửi lệnh              | Thực thi       |
| Chờ ACK               | Trả kết quả    |
| Không biết thuật toán | Không cần biết |

- Đây là kiểu giao tiếp **Command-Response Protocol**, không phải **Register-based** như các ngoại vi khác (Ví dụ I2C,...)

### 5.1. Cấu trúc Packet chuẩn 

```text
| Header | Addr | PID | Length | Command/Data | Checksum |

```
- Ví dụ (hex):

```nginx
EF 01 | FF FF FF FF | 01 | 00 03 | 01 | 00 05

```
- Giải thích:
   - `EF 01`: Header 
   - `FF FF FF FF`: Address (cảm biến)
   - `01`: Packet ID (Command)
   - `00 03`: Length 
   - `01`: Command Length
   - `00 05`: Check sum

## 6. Vì sao nhìn cảm biến ZW111 "dẹt" như thế ?
- Thế hệ cũ, LED, Camera thường nằm lộ ra, module to và dày. Nhưng thế hệ mới như ZW111 đã được cải tiến nhỏ gọn hơn
- "Dẹt" không có nghĩa là không có phần quang học:
   - LED nắm dưới lớp kính (phần mặt phẳng để user chạm tay vào)
   - Sensor CMOS nằm song song mặt kính
   - Ánh sáng đi vào - phản xạ - thu lại trong module
- Quang học kiểu **internal reflection** (TIR - Total internal Reflection)

### 6.1. Cấu trúc bên trong ZW111 (thực tế)
```css
Ngón tay
──────────────
Kính / acrylic (sensor window)
──────────────
LED hồng ngoại (bên cạnh / dưới)
──────────────
Prism / optical layer
──────────────
CMOS image sensor (nằm PHẲNG)
──────────────
MCU + DSP

```

# II. CHI TIẾT VỀ CẢM BIẾN VÂN TAY ZW111
- *Thông tin dưới đây được lấy từ datasheet*

## 1. Software Developing Guide - ZW111
### 1.1 Paramter Table (Bảng tham số hệ thống)
- Parameter Table là bảng cấu hình lõi quyết định cách giao tiếp, thuật toán và bộ nhớ của Module ZW111
- Đặc điểm quan trong: 
   - Độ dài: 64 **Words** = 128 bytes
   - Lưu trong FLASH nội bộ 
   - Sau khi cấp nguồn: 
      - Được copy từ **FLASH** → **RAM** (0x200–0x23F)
      - DSP sử dụng bảng này để khởi tạo toàn bộ hệ thống 
- Vì vậy: 
   - Đây không phải thanh ghi MCU 
   - Mà là bảng cấu hình logic của Module 
- **Các nhóm thông số chính:**

#### PART 1 - Trạng thái & cảm biến 
|Number | Name | Length(Word) | Default/Content | Mô tả | 
|-------|------|--------------|-----------------|-------|
|1| `SSR`|  1 | 0 | State Register |
|2| `SensorType`| 1 |   0 - 15 | Loại cảm biến |
|3| `DataBaseSize`| 1 | Theo FLASH |  Dung lượng database vân tay |

#### PART 2 - Giao tiếp & bảo mật 
|Number | Name | Length(Word) | Default/Content | Mô tả | 
|-------|------|--------------|-----------------|-------|
|4| `SecurityLevel`|  1 | 3 | Mức bảo mật (5 mức) |
|5| `DeviceAddress`| 2 | 0xFFFFFFFF | Địa chỉ chip (32-bit) |
|6| `CFG_PktSize`|  1 | 1 | Dung lượng packet data |
|7| `CFG_Baudrate`| 1 | 6 |  Hệ số baudrate |
|8| `CFG_VID`| 1 |  | USB descriptor|
|9| `CFG_PID`| 1 |  | USB descriptor|
|10| `Reversed`| 1 |  |  | 
....
|13| `Reversed`| 1 |  |  | 
|14| `ProductSN`| 4 | ProductSN, ASCII Code  |  Mô tả thiết bị từ nhà sản xuất |
|15| `SoftwareVersion`| 4 | ASCII Code|  Mô tả thiết bị từ nhà sản xuất |
|16| `Manufacturer `| 4 | ASCII Code  |  Mô tả thiết bị từ nhà sản xuất |
|17| `SensorName`| 4 | ASCII Code |  Mô tả thiết bị từ nhà sản xuất |
|18| `Password`|2 | 00000000H  | Mặc định 00000000H |
|19| `JtagLockFlag`| 2 | 00000000H | |
|20| `SensorInitEntry`|  | Entry Address | Quy trình đầu vào khởi tạo cảm biến|
|21| `SensorGetImageEntry`| 1 | Entry Address | Chương trình lấy hình ảnh đầu vào |
|22| `Reversed`| 27 |  |  |

#### PART 3 - Kiểm tra hợp lệ 
|Number | Name | Length(Word) | Default/Content | Mô tả | 
|-------|------|--------------|-----------------|-------|
|23| `ParaTableFlag`| 1 | 0x1234 | Cờ xác nhận tham số hợp lệ |

- Ý nghĩa Driver: 
   - Driver không truy cập trực tiếp bảng 
   - Mà đọc/ghi thông tin qua lệnh `PS_ReadSysPara`, `PS_WriteReg`



## 2. Kiến trúc hệ thống phần mềm trong Project
```css
[ Fingerprint ZW111 ]
          │ UART
          ▼
[ Zigbee End Device ]  <-- DRIVER Ở ĐÂY
          │
          │ Zigbee Cluster
          ▼
[ Coordinator / Gateway ]
          │
          ▼
[ App / Server ]
```

## 3. Luồng hoạt động chuẩn (cửa thông minh)

```sql
Finger placed
   ↓
ZW111 driver match()
   ↓
Result = MATCH / FAIL
   ↓
Zigbee send status
   ↓
Coordinator quyết định
   ↓
Send OPEN / CLOSE
   ↓
Relay / Motor
```
- Zigbee chỉ gửi kết quả, không gửi ảnh/raw data
- Driver ZW111 phải nằm trong Zigbee **End Device** vì đó là nơi duy nhất có cảm biến và điều khiển khóa 

## 4. Cấu hình giao thức cho ZW111 trong project
- Giao thức sử dụng **USART1**
- Bước cấu hình Driver USART:

### 4.1. Thêm **UARTDRV**
- Vào: 
```nginx
Software Components
→ Plarform
→ Driver
→ UART
→ UARTDRV USART
```
- Add/ Enable nếu chưa có

### 4.2. Chọn đúng USART instance (**Quan trọng**)
- Trong config UARTDRV USART, khi chọn vào đó, sẽ có cửa sổ bắt bạn chọn tên (instance) cho giao thức 
- Hãy đặt là `USART_UAST1` nếu bạn chọn UART1 hoặc tùy theo tên bạn muốn đặt.

![Config](../Images/USARTDRV_config.png)

### 4.3. Cấu hình **USART UART** 
- Sau khi Enable xong rồi thì bạn cần phải cấu hình luôn thông số và Pin cho giao thức
- Cấu hình như sau: 

| Tham số      | Giá trị                               |
| ------------ | ------------------------------------- |
| Mode         | **Asynchronous**                      |
| Baudrate     | **57600** (hoặc theo datasheet ZW111) |
| Data bits    | **8**                                 |
| Parity       | **None**                              |
| Stop bits    | **1**                                 |
| Flow control | **None**                              |
 
### 4.4. Pin routing (quan trọng)
- Cấu hình xong thông số rồi thì tiếp theo là đến bước chọn cấu hình GPIO cho USART

| Signal    | Pin               |
| --------- | ----------------- |
| USARTx_TX | GPIO nối TX ZW111 |
| USARTx_RX | GPIO nối RX ZW111 |

### 4.5. Sơ đồ tư duy đúng 

```sql
ZW111
  │ UART
  ▼
UARTDRV USART  (End Device)
  │
ZW111 Driver
  │
Door Lock App
  │
Zigbee Cluster
```

## 5. Cấu trúc thư mục của thư viện ZW111

```yaml
 
ZW111_lib/
├─ Inc/
│  ├─ Port/
│  │   ├─ zw111_port_efr32.h
│  │   ├─ zw111_port_stm32.h
│  │   └─ zw111_port_esp32.h
│  ├─ zw111.h              ← API cho app
│  ├─ zw111_types.h        ← struct / enum / status
│  ├─ zw111_lowlevel.h     ← API lệnh thấp
│  ├─ zw111_port.h         ← interface phần cứng
│  └─ zw111_port_select.h  ← chọn port (EFR32/STM32/ESP32)
│
├─ Src/
│  ├─ zw111.c              ← logic cao (enroll/match)
│  ├─ zw111_lowlevel.c     ← packet, checksum, parse
│  ├─ Port/
│  │   ├─ zw111_port_efr32.c
│  │   ├─ zw111_port_stm32.c
│  │   └─ zw111_port_esp32.c
│
└─ README.md

```
### 5.1 FLOW các file của thư viện 

```yaml
App / Zigbee
     |
     v
 zw111.h          ← API cho app
     |
     v
 zw111.c          ← logic cao
     |
     v
 zw111_lowlevel.h ← giao thức ZW111
     |
     v
 zw111_port.h     ← cổng phần cứng
     |
     v
 zw111_port_efr32.c (UARTDRV)
```