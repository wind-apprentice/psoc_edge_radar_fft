# psoc_edge_radar_fft

## 1. 專案摘要 Project Summary

本專案運行於 PSOC Edge E84 AI Kit，從板載 XENSIV BGT60TR13C 雷達感測器讀取 IF signal，並在 MCU 端進行 FFT 與 spectrum 分析。

目前採用的處理流程如下：

- radar signal frame acquisition
- per-chirp DC removal
- Hamming windowing
- range FFT
- magnitude spectrum integration across chirps
- UART diagnostics output

此分支刻意對齊 Infineon 原始 sample flow，方便後續由原廠協助除錯與支援。

## 2. 參考來源 Reference Repositories

本專案基於以下公開資源整合：

- https://github.com/Infineon/mtb-example-ce241721-xensiv-60ghz-static-distance
- https://github.com/Infineon/mtb-example-psoc-edge-hello-world
- https://github.com/Infineon/sensor-xensiv-bgt60trxx
- mtb://sensor-dsp（ModusToolbox dependency）

## 3. 使用硬體 Hardware

- 開發板 Board：KIT_PSE84_AI (PSOC Edge E84 AI Kit)
- 雷達感測器 Radar Sensor：onboard XENSIV BGT60TR13C
- 資料介面：SPI + IRQ
- 記錄輸出：KitProg3 Virtual COM，115200-8-N-1

## 4. 輸入與輸出 Input / Output

輸入 Input：

- 從 sensor FIFO 讀取的 raw radar frame samples
- frame layout 由 XENSIV_BGT60TRXX_CONF 相關設定定義

輸出 Output：

- UART frame-level diagnostics
- spectrum 摘要 bins（例如 B2/B6/B10/B14）
- peak bin 與 SNR-like 指標（bring-up 驗證用途）

備註 Note：

- get_static_distance() 的 distance 回傳值在本分支不是主要輸出。
- 主要對外交付資訊為 spectrum 與其診斷指標。

### 4.1 客戶實測簡單流程（建議）

1. 將 KIT_PSE84_AI 固定在桌面，雷達正面朝向測試區域。
2. 開啟 UART terminal（115200-8-N-1），確認有連續 frame 輸出。
3. Baseline：前方 0.5 m 內不放明顯反射物，觀察 50 到 100 筆輸出。
4. 放入目標物：在約 20 到 30 cm 放置金屬板，觀察 50 到 100 筆輸出。
5. 調整距離：將目標移到約 50 到 70 cm，確認 SearchPeakBin 與 Peak 有趨勢變化。

### 4.2 輸入/輸出資料範例

輸入資料（sensor FIFO raw sample 摘要）可觀察前幾個 sample 值是否持續更新：

Example Input Snapshot:
S0=1118 | S1=2875 | S2=1719 | S3=2398

輸出資料（Mode 3）範例如下：

Example Output A:
Frame=220 | WaitMs=31 | RawPeakBin=3 | SearchPeakBin=7 | Peak=1.482 | Range=11.41 cm | Noise=0.436 | SNRx=3.40 | B2=6.88 B6=1.36 B10=0.92 B14=0.64

Example Output B:
Frame=221 | WaitMs=30 | RawPeakBin=3 | SearchPeakBin=11 | Peak=1.903 | Range=17.93 cm | Noise=0.472 | SNRx=4.03 | B2=6.54 B6=1.12 B10=1.58 B14=0.79

## 5. 專案結構 Project Structure

- proj_cm33_ns：non-secure application，負責 radar acquisition、processing、UART output
- proj_cm33_s：CM33 secure side project
- proj_cm55：CM55 side project

## 6. 使用 ModusToolbox Eclipse IDE 匯入專案（GUI）

客戶端建議流程是「先 clone 到本地，再用 ModusToolbox 匯入」。

### 6.1 先把 GitHub repo clone 到本地端（Terminal）

1. 開啟終端機（Windows PowerShell、CMD 或 Git Bash 皆可）。
2. 切換到你要放專案的資料夾。
3. 執行以下指令：

```bash
git clone https://github.com/wind-apprentice/psoc_edge_radar_fft.git
```

4. clone 完成後，會得到本地資料夾 psoc_edge_radar_fft。

### 6.2 在 ModusToolbox Eclipse IDE 匯入 clone 後的專案（GUI）

1. 開啟 ModusToolbox Eclipse IDE。
2. 點選 File -> Import。
3. 選擇 ModusToolbox 類別中的 Existing Application In-Place。
4. 在 Application Path 選到剛 clone 下來的本地資料夾 psoc_edge_radar_fft。
5. 按 Finish，等待 Workspace indexing 與 project discovery 完成。
6. 若 IDE 提示缺少 library，點選 Yes 讓 IDE 自動抓取 dependencies。
7. 確認 Project Explorer 中出現以下三個子專案：
8. psoc_edge_radar_fft.proj_cm33_ns
9. psoc_edge_radar_fft.proj_cm33_s
10. psoc_edge_radar_fft.proj_cm55

### 6.3 匯入常見檢查點

1. 若看不到三個子專案，先右鍵最上層 application 做 Refresh。
2. 若 launch config 沒出現，重開一次 IDE 並等待索引完成。
3. 若 library 抓取失敗，檢查公司網路是否可連到 GitHub。

## 7. 使用 ModusToolbox Eclipse IDE 建置與燒錄（GUI）

1. 將 KIT_PSE84_AI 以 USB 連接至電腦（KitProg3 介面）。
2. 在 Project Explorer 右鍵點選最上層 Application，選擇 Build Application。
3. 等待 Console 顯示 build 完成且無 error。
4. 於上方工具列選擇 Run -> Run Configurations。
5. 選擇 .mtbLaunchConfigs 中的 Program Application（或 Debug MultiCore）。
6. 按 Run 或 Debug 進行燒錄。
7. 燒錄完成後，開啟 Terminal Tool（例如 Tera Term）連到 KitProg3 COM Port，設定 115200-8-N-1。

## 8. 執行模式 Runtime Mode

請在 proj_cm33_ns/main.c 內設定 PHASE1_APP_MODE：

- 3U：real radar signal -> FFT/spectrum（本分支僅支援 3U）

## 9. UART 輸出判讀 UART Verification

在 Mode 3 下，UART 每個 frame 會輸出類似欄位：

- Frame
- RawPeakBin
- SearchPeakBin
- Peak
- Noise
- SNRx
- B2/B6/B10/B14

建議先確認欄位穩定刷新，再進行目標物距離與角度場景測試。

## 10. 授權與交付注意事項 License / Delivery Notes

本專案包含來自 Infineon 公開範例與公開 library 的衍生整合內容。

交付客戶前請確認：

- 保留原始 source file 的 license header 與 attribution
- 不移除上游版權與授權聲明
- 若公司有 OSS/compliance 流程，依內部規範再走一次法遵審查