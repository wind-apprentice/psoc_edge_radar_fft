# psoc_edge_radar_fft

## 1. 專案摘要 Project Summary

本專案運行於 PSOC Edge E84 AI Kit，從板載 XENSIV BGT60TR13C 雷達感測器讀取 IF signal，並在 MCU 端進行 FFT 與 spectrum 分析。

目前採用的處理流程如下：

- radar signal frame acquisition
- per-chirp DC removal
- Hamming windowing
- range FFT
- magnitude spectrum integration across chirps
- UART spectrum output

此分支刻意對齊 Infineon 原始 sample flow，方便後續由原廠協助除錯與支援。

## 2. 參考來源 Reference Repositories

本專案基於以下公開資源整合：

- https://github.com/Infineon/mtb-example-ce241721-xensiv-60ghz-static-distance
- https://github.com/Infineon/mtb-example-psoc-edge-hello-world
- https://github.com/Infineon/sensor-xensiv-bgt60trxx
- mtb://sensor-dsp（ModusToolbox dependency）

說明：

- sensor-xensiv-bgt60trxx 已以官方原始碼形式納入本專案的 third_party 目錄
- sensor-dsp 仍透過 ModusToolbox library dependency 取得

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

- UART 輸出的 integrated spectrum
- 每個 frame 一行 spectrum CSV
- frame index 與有效 bin 範圍資訊

備註 Note：

- get_static_distance() 的 distance 回傳值在本分支不是主要輸出。
- 主要對外交付資訊為 spectrum，本分支不再額外輸出自訂 peak/noise/SNR 診斷欄位。

### 4.1 客戶實測簡單流程（建議）

1. 將 KIT_PSE84_AI 固定在桌面，雷達正面朝向測試區域。
2. 開啟 UART terminal serial port（115200-8-N-1），確認有連續 frame 輸出。
3. Baseline：前方 0.5 m 內不放明顯反射物，觀察 50 到 100 筆輸出。
4. 放入目標物：在約 20 到 30 cm 放置金屬板，觀察 50 到 100 筆輸出。
5. 調整距離：將目標移到約 50 到 70 cm，確認 spectrum 主峰位置有可追蹤的位移趨勢。

### 4.2 輸入/輸出資料範例

輸入資料來自 sensor FIFO 的 raw radar frame。這些原始 sample 會先經過官方流程做 de-bias、window 與 FFT，再輸出 integrated spectrum。

輸出資料（Mode 3）範例如下：

Example Output A:
Frame=220 | WaitMs=31 | SpectrumBins=3-24 | Spectrum=0.812,1.044,1.482,1.126,0.901,0.744,0.618,0.551,0.498,0.463,0.429,0.401,0.372,0.341,0.329,0.318,0.305,0.291,0.276,0.263,0.251,0.241

Example Output B:
Frame=221 | WaitMs=30 | SpectrumBins=3-24 | Spectrum=0.774,0.992,1.215,1.903,1.312,0.889,0.701,0.612,0.558,0.503,0.471,0.436,0.398,0.377,0.351,0.333,0.320,0.304,0.289,0.272,0.260,0.248

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
2. 在 Project Explorer 右鍵點選最上層 application（psoc_edge_radar_fft），選擇 ModusToolbox -> Library Manager。
3. 確認 proj_cm33_ns/deps 內存在 sensor-dsp，按 Apply 或 Update 讓 IDE 抓取官方 DSP library。
4. Library Manager 完成後，右鍵 application 選擇 Clean Application。
5. 再次右鍵 application，選擇 Build Application。
6. 等待 Console 顯示 build 完成且無 error。
7. 於上方工具列選擇 Run -> Run Configurations。
8. 選擇 .mtbLaunchConfigs 中的 Program Application（或 Debug MultiCore）。
9. 按 Run 或 Debug 進行燒錄。
10. 燒錄完成後，開啟 Terminal Tool（例如 Tera Term）連到 KitProg3 COM Port，設定 115200-8-N-1。

### 7.1 若出現 ifx_sensor_dsp.h: No such file or directory

1. 回到 Library Manager，確認 sensor-dsp 已成功安裝且版本已 lock。
2. 確認 proj_cm33_ns/deps/sensor-dsp.mtb 存在。
3. 執行 Clean Application 後重新 Build Application。
4. 若仍失敗，關閉 IDE 後重開 workspace 再重試 Build。

### 7.2 若出現 xensiv_bgt60trxx_mtb.h: No such file or directory

1. 確認本專案內存在 third_party/sensor-xensiv-bgt60trxx。
2. 確認該目錄內含 xensiv_bgt60trxx_mtb.h、xensiv_bgt60trxx.c、xensiv_bgt60trxx_edge.c。
3. 執行 Clean Application 後重新 Build Application。


## 8. UART 輸出判讀 UART Verification

UART 每個 frame 會輸出類似欄位：

- Frame
- WaitMs
- SpectrumBins
- Spectrum

建議先確認 spectrum 行持續刷新，再進行目標物距離與角度場景測試。

## 9. 授權與交付注意事項 License / Delivery Notes

本專案包含來自 Infineon 公開範例與公開 library 的衍生整合內容。

交付客戶前請確認：

- 保留原始 source file 的 license header 與 attribution
- 不移除上游版權與授權聲明
- 若公司有 OSS/compliance 流程，依內部規範再走一次法遵審查