# 🐟 Smart Aquarium Controller with Auto Feeding, Heater Control, and pH Monitoring

Proyek ini adalah sistem **akuarium otomatis berbasis Arduino** yang dapat:
- Memberikan pakan otomatis pada pagi dan sore hari sesuai jadwal.
- Mengontrol pemanas air berdasarkan suhu air.
- Menampilkan waktu, suhu air, dan pH air secara real-time di LCD.
- Dapat dikonfigurasi melalui tombol menu untuk set jam RTC, jadwal pakan, dan batas suhu.

---

## 🔧 Fitur Utama

### ✅ Auto Feeder
- Jadwal pakan pagi dan sore dapat diatur melalui menu LCD.
- Menggunakan **servo motor** untuk membuka dan menutup pakan sebanyak 4 kali per sesi.

### ✅ Kontrol Pemanas Air (Heater)
- Mengontrol **relay heater** otomatis berdasarkan suhu air.
- Pengguna dapat mengatur batas suhu minimal yang diperbolehkan.

### ✅ Monitoring Real-time
- Menampilkan **jam digital (RTC)**.
- Monitoring **suhu air (°C)** dan **pH air** secara langsung di LCD 16x2 I2C.

### ✅ Menu Navigasi
- Menggunakan 4 tombol (`MENU`, `UP`, `DOWN`, `OK`) untuk mengatur:
  - Jam RTC
  - Jadwal pakan (pagi & sore)
  - Batas suhu minimum
- Pengaturan disimpan ke **EEPROM** agar tidak hilang saat mati listrik.

---

## 🧰 Komponen yang Digunakan

| Komponen                 | Jumlah | Keterangan                                 |
|--------------------------|--------|--------------------------------------------|
| Arduino Uno / Nano       | 1      | Mikrokontroler utama                       |
| LCD 16x2 I2C             | 1      | Tampilan antarmuka pengguna                |
| RTC DS3231               | 1      | Real-Time Clock (penjaga waktu)            |
| Sensor Suhu DS18B20      | 1      | Sensor suhu air digital                    |
| Sensor pH analog         | 1      | Sensor pengukur keasaman air               |
| Relay Module             | 1      | Untuk mengontrol pemanas/heater            |
| Servo Motor (SG90/MG90S) | 1      | Untuk membuka tutup pakan                  |
| Push Button              | 4      | Navigasi menu (MENU, UP, DOWN, OK)         |
| EEPROM (internal)        | -      | Digunakan untuk menyimpan pengaturan       |
| Kabel jumper & breadboard| -      | Untuk sambungan rangkaian                  |
| Power supply             | 1      | 5V atau USB                                |

---

## ⚙️ Alur Kerja Sistem

### 🔁 Loop Utama
1. Menampilkan suhu dan pH di layar LCD setiap detik.
2. Mengecek waktu sekarang:
   - Jika sesuai jadwal, servo akan berputar untuk membuka pakan.
3. Jika suhu < batas minimum, heater akan menyala melalui relay.

### 📟 Menu Navigasi
Menu diakses dengan menekan tombol `MENU`. Navigasi dan pengaturan dilakukan dengan tombol `UP`, `DOWN`, dan `OK`.

**Menu yang tersedia:**
1. **Set Jam** — untuk atur jam RTC internal.
2. **Set Jadwal** — mengatur jadwal pakan pagi & sore.
3. **Batas Suhu** — mengatur suhu minimal untuk mengaktifkan heater.
4. **Keluar** — kembali ke tampilan utama.

---

## 📐 Struktur Pin Arduino

| Nama          | Pin Arduino |
|---------------|-------------|
| BTN_MENU      | D5          |
| BTN_UP        | D4          |
| BTN_DOWN      | D3          |
| BTN_OK        | D2          |
| Servo Motor   | D6          |
| Relay Heater  | D7          |
| pH Sensor     | A0          |
| DS18B20       | D12         |
| RTC DS3231    | SDA/SCL     |
| LCD I2C       | SDA/SCL     |

---

## 📊 Kalibrasi Sensor pH

Sensor pH diatur menggunakan nilai referensi tegangan:

```cpp
float PH4 = 3.1; // Tegangan saat larutan buffer pH 4
float PH7 = 2.6; // Tegangan saat larutan buffer pH 7
