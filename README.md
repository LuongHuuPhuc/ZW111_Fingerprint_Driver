# I. TỔNG QUAN VỀ CẢM BIẾN VÂN TAY ZW111   

![Sensor image](/Images/ZW111sensor.png)v <br>
**ZW111 datasheet**: ![Datasheet ZW111](https://www.elemon.com.ar/images/productos/hojas-de-datos/sensores-huella-dactilar/zw111.pdf) <br>
**Communication Protocol**: ![Datasheet Fingerprint Sensor Module User Manual](https://cdn.sparkfun.com/assets/5/a/3/f/c/Module_User_Manual-17151-Capacitive_Fingerprint_Scanner_-_AS-108M.pdf)

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

### 3.3. ROM/FLASH/RAM 
- ROM:
   - Chứa firmware gốc 
   - Chứa thuật toán nhận dạng
   - Người dùng không sửa được 

- FLASH (Template Database/Fingerprint Database):
   - Lưu vân tay đã enroll
   - NotePad (User Data)
   - Mỗi vân tay = 1 template
   - Có Page ID

- RAM: 
   - ImageBuffer (Bộ đệm hình ảnh): Lưu tạm thời, rất ngắn 
   - Buffer Charfile1/Charfile2 (Bộ đệm đặc trưng)

## 4. Vì sao lại có Buffer1/Buffer2
- ZW111 không xử lý ảnh trực tiếp trên FLASH
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

### 4.1 Nếu cảm biến không lưu ảnh thì ImageBuffer là gì ? 
- ZW111 KHÔNG lưu ảnh vân tay để người dùng truy xuất nhưng bên trong nó VẪN có ImageBuffer để xử lý tạm thời 
- Nó chính là buffer trung gian nội bộ, dùng cho pipeline xử lý 
```css
Cảm biến →
  ImageBuffer (RAM nội)
    ↓
  Xử lý ảnh (lọc, tăng cường)
    ↓
  Trích đặc trưng (feature)
    ↓
  So khớp / tạo template
```

- Nó tồn tại rất ngắn, chỉ trong quá trình *eroll*, *capture*, *verify*, *identify*
- Tự động thu thập và xử lý hình ảnh bằng ImageBuffer nội bộ 
- Tuy nhiên, hình ảnh vân tay không được hiển thị cho host MCU, MCU chỉ giao tiếp thông qua các lệnh cấp cao và nhận kết quả khớp hoặc mã trạng thái.

## 5. Instruction Form Specification (Giải thích giao thức lệnh)
- ZW111 là module nhận diện vân tay hoạt động ở chế độ **Slave**, giao tiếp với MCU host qua UART
- MCU host: 
   - Gửi instruction (Command/Data packet)
   - Nhận ACK packet hoặc Data packet
- Module chỉ phản hổi, không chủ động gửi data

### 5.1. Cơ chế phân loại gói tin (Packet Types)
- ZW111 sử dụng 3 packet chính, tất cả đều bắt đầu bằng **packet header** cố định `0xEF01`

| Packet flag | Loại packet            | Ý nghĩa                |
| ----------- | ---------------------- | ---------------------- |
| `0x01`      | Command packet         | Gói lệnh điều khiển    |
| `0x02`      | Data packet (continue) | Gói dữ liệu trung gian |
| `0x08`      | End data packet        | Gói dữ liệu cuối       |
- Data luôn được chia thành nhiều packet

### 5.2. Cấu trúc chung của Packet

```text
| Header | Chip Addr | Packet Flag | Length | Payload | Checksum |
```
- Chi tiết từng trường:

| Trường        | Kích thước | Mô tả                                  |
| ------------- | ---------- | -------------------------------------- |
| Packet Header | 2 bytes    | Luôn là `0xEF01`                       |
| Chip Address  | 4 bytes    | Địa chỉ module (mặc định `0xFFFFFFFF`) |
| Packet Flag   | 1 byte     | Loại packet                            |
| Packet Length | 2 bytes    | Tổng byte của Payload (Không tính header, address, flag)|
| Payload       | N bytes    | Instruction / Data                     |
| Checksum      | 2 bytes    | Tổng checksum (Theo quy tắc **Big-Endian**)|

#### 5.2.1. Command Packet Format (Flag = `0x01`)
- Cấu trúc: 

![Command Packet Format](/Images/CommandPacketFormat.png)

- Mã lệnh (instruction): 1 byte
- Parameter: Tham số cho lệnh 
- Cách tính **Packet Length** = tổng số byte tính từ trường **Packet Length** cho đến trường **Checksum(Sum)** (bao gồm cả Instruction/Parameter/Data và **Checksum** nhưng không bao gồm chính byte của trường **Packet Length**). Đây cũng chính là độ dài của **Payload**
- Cách tính **Checksum** = ổng số byte tính từ **Packet Flag** cho đến **Checksum**. Nếu kết quả cộng vượt quá 2 byte thì bỏ qua phần Carry (chỉ giữ 16-bit thấp)

#### 5.2.2. Data Packet Format (Flag = `0x02`)
- Cấu trúc: 

![Data Packet Format](/Images/DataPacketFormat.png)

- Dùng khi: 
   - Truyền template 
   - Truyền dữ liêu FLASH 
   - Truyền dữ liệu nhiều gói 
- Mỗi gói phải được ACK trước khi gửi tiếp 

#### 5.2.3. End Packet Format (Flag = `0x08`)
- Cấu trúc: 

![End Packet Format](/Images/EndPacketFormat.png)

- Gói **cuối cùng** của chuỗi data packet

#### 5.2.4 ACK Packet Format (Flag = `0x07`)
- Đây là Packet chỉ truyền 1 chiều khi Module phản hồi lại MCU host (phản hồi trạng thái), không truyền 2 chiều như **Data Packet**
- Định dạng packet giống hệt 3 **Data Packet** trên nhưng nó không nằm trong nhóm Data Packet do Payload của nó khác (chỉ gửi Confirm code + return param).

![ACK Packet Format](/Images/ACKPacketFormat.png)

- ACK Packet có thể đi kèm Data Packet (*ACK packet contains parameter and can be with continue data packet* - theo datasheet)
- Nghĩa là ACK packet vừa có thể: 
   - Chỉ báo trạng thái
   - Hoặc vừa ACK vừa báo còn data tiếp theo 
- Ví dụ: 
   - ACK = `0xF1` → cho phép bắt đầu stream
   - ACK = `0xF0` → cho phép gửi packet data tiếp
- Quan hệ giữa ACK và Data packet 

```scss
Host → Command packet
Module → ACK (0x07, ConfirmCode = 0xF1)

Host → Data packet (0x02)
Module → ACK (0x07, ConfirmCode = 0xF0)

Host → End data packet (0x08)
Module → ACK (0x07, ConfirmCode = 0x00)
```
- ACK điều khiển flow, Data chỉ mang Payload

-  **Thứ tự truyền bắt buộc (QUAN TRỌNG)**

```scss
Command Packet
  ↓ ACK (0xF1)
Data Packet (0x02)
  ↓ ACK (0xF0)
Data Packet (0x02)
  ↓ ACK (0xF0)
End Data Packet (0x08)
  ↓ ACK (0x00)
```
- Không được gửi Packet mới khi chưa nhận được ACK phản hồi 

### 5.3. Vì sao driver phải dùng Command/Instruction Set + ACK 
- Vì ZW111 là thiết bị chủ 

| MCU ngoài             | ZW111          |
| --------------------- | -------------- |
| Gửi lệnh              | Thực thi       |
| Chờ ACK               | Trả kết quả    |
| Không biết thuật toán | Không cần biết |

- Đây là kiểu giao tiếp **Command-Response Protocol**, không phải **Register-based** như các cảm biến ngoại vi khác (Ví dụ I2C,...)


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
|3| `DataBaseSize`| 1 | Theo FLASH |  Dung lượng Fingerprint Database |

#### PART 2 - Giao tiếp & bảo mật 
|Number | Name | Length(Word) | Default/Content | Mô tả | 
|-------|------|--------------|-----------------|-------|
|4| `SecurityLevel`|  1 | 3 | Mức bảo mật (5 mức) |
|5| `DeviceAddress`| 2 | 0xFFFFFFFF | Địa chỉ chip (32-bit) |
|6| `CFG_PktSize`|  1 | 1 | Dung lượng packet data |
|7| `CFG_Baudrate`| 1 | 6 |  Hệ số baudrate |
|8| `CFG_VID`| 1 |  | USB descriptor|
|9| `CFG_PID`| 1 |  | USB descriptor|
|10 - 13 | `Reversed`| 1 |  |  | 
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

### 1.2 System Parameter Memory Structure 
- ZW111 có SPM (System Parameter Memory) chia làm 8 page, mỗi page 512 bytes 

| Page | Nội dung                   |
|------|----------------------------|
| 0    | Reserved                   |
| 1    | Parameter Table            |
| 2    | User Notepad               |
| 3–6  | Reserved                   |
| 7    | Index Fingerprint Database |

- Quan trọng: 
   - Index table cho phép quản lý tối đa 2048 template
   - Mỗi bit đại diện cho 1 template slot
- Driver : 
   - Đọc index để biết slot trống, không ghi template "mù"

### 1.3 NotePad (Vùng nhớ người dùng)
- ZW111 dành 512 bytes FLASH cho USER NotePad (chính là 1 page trong SPM)
-  Cấu trúc: 
   - Chia logic thành 16 page
   - Mỗi page: 32 bytes 
- Truy cập: 
   - `PS_WriteNotePad(page, data[32])`
   - `PS_ReadNotePad(page)`
- Lưu ý quan trọng: 
   - Ghi là ghi đè cả page 
   - Không có write từng bytes
   - Phù hợp để lưu ID người dùng, Metadata, Version ứng dụng

### 1.4 Buffer & Fingerprint Database 
- Các Buffer nội bộ:

| Buffer      | Dung lượng | Chức năng              |
| ----------- | ---------- | ---------------------- |
| ImageBuffer | ~72 KB     | Lưu ảnh vân tay        |
| CharBuffer1 | ~7 Bytes   | Lưu feature / template |
| CharBuffer2 | ~7 Bytes   | So khớp / enroll       |

- Buffer này nằm bên trong module, không phải RAM host.
- USER có thể đọc/ghi bất kỳ bộ đệm nào ở trên bằng các Command/Instruction ID
- Để giảm thời gian giao tiếp khi tải lên hoặc tải xuống hình ảnh thông qua giao diện UART, chỉ 4-bit trên cùng của byte pixel được áp dụng, tức là kết hợp 2 byte pixel thành 1 byte trong quá trình truyền 
   - Qua UART: 1 byte = 2 pixel (4-bit/pixel) → giảm thời gian
- Còn đối với USB thì vẫn phải truyền đủ 8 bytes/pixel

### 1.5 Features & Templates 
- ZW111 không lưu ảnh mà lưu các đặc trưng vân tay (feature)
- Feature file: 
   - Kích thước: 425 bytes 
   - Chứa: thông tin chung, Minutiae (điểm đặc trưng)
- Template file: 
   - Kích thước: 2129 bytes 
   - Gồm 5 feature file của cùng 1 ngón tay 
   - Dùng cho so khớp chính xác cao 
- Quy trình chuẩn: 
   - `GetImage` -> `GenChar` (n lần) -> `RegModel` (Tạo Template) -> `Storechar` (Ghi FLASH)

### 1.6 Feature File Structure (Cấu trúc file đặc trưng)
- Số lượng **Minutiae (điểm đặc trưng)** của 1 file đặc trưng thì không nhiều hơn **99**
- Trong tổng 2129 bytes (kích thước của 1 file đặc trưng là 425 bytes), 56 bytes đầu là file header sử dụng cho thông tin chung. 
- 369 bytes sau được dùng để lưu trữ thông tin chi tiết, 4 byte cho mỗi **Minutiae**
- **Format file Header (56 bytes đầu)**:

| Trường      | Ý nghĩa                 |
|-------------|-------------------------|
| Flag        | Hợp lệ / không          |
| Type        | Loại feature            |
| Quality     | Chất lượng (0–100)      |
| Number      | Số minutiae             |
| SN          | Thông tin hỗ trợ search |
| Singularity | Tâm vân tay             |
- MCU host không cần hiểu thứ này, cái này do nội bộ module xử lý

### 1.7 ROM (Hệ thống bên trong Module)
- ROM của ZW111 chứa toàn bộ hệ thống hoàn chỉnh: 
   - Giao thức của UART/USB
   - Bộ giải mã lệnh
   - Thuật toán nhận dạng vân tay
   - Quản lý FLASH
   - Driver cảm biến vân tay (HF105)
- Điều này giải thích: 
   - ZW111 không phải sensor thuần 
   - Mà là **SoC** vân tay hoàn chỉnh
- Host MCU không cần xử lý ảnh, không xử lý thuật toán, chỉ cần: 
   - Gửi lệnh, nhận ACK, nhận kết quả

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

![Config](/Images/USARTDRV_config.png)

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
