#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__

#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_log.h>
#include <esp_timer.h>
#if CONFIG_IDF_TARGET_ESP32S3
#include <driver/usb_serial_jtag.h>
#endif

class PowerManager {
private:
    // 分压系数：2个100k电阻 → 1:2 分压，电池电压 = 测量电压 × 2
    static constexpr float VOLTAGE_DIVIDER_RATIO = 2.0f;

    // 多点电池曲线（锂电池真实放电特性，单位：毫伏）
    // 4.2V=100%, 4.0V≈80%, 3.85V≈60%, 3.7V≈40%, 3.5V≈15%, 3.3V≈0%
    static constexpr struct {
        uint32_t millivolts;
        uint8_t level;
    } BATTERY_LEVELS[] = {
        {3300, 0},
        {3500, 15},
        {3700, 40},
        {3850, 60},
        {4000, 80},
        {4200, 100}
    };
    static constexpr size_t BATTERY_LEVELS_COUNT = 6;
    static constexpr size_t ADC_VALUES_COUNT = 10;

    // 充电检测阈值（毫伏）
    static constexpr uint32_t CHARGING_VOLTAGE_HIGH_THRESHOLD = 4280;  // 电池电压超过4.28V → 充电中
    static constexpr uint32_t CHARGING_VOLTAGE_DELTA = 40;             // 当前电压比基准高40mV → 充电中
    static constexpr uint32_t DISCHARGING_VOLTAGE_DELTA = 30;          // 当前电压比基准低30mV → 放电中
    static constexpr int CHARGING_CHECKPOINT_INTERVAL = 30;
    static constexpr int CHARGING_HOLD_TICKS = 30;

    esp_timer_handle_t timer_handle_ = nullptr;
    gpio_num_t charging_pin_;
    adc_unit_t adc_unit_;
    adc_channel_t adc_channel_;
    uint32_t adc_values_[ADC_VALUES_COUNT];  // 改为存毫伏值
    size_t adc_values_index_ = 0;
    size_t adc_values_count_ = 0;
    uint8_t battery_level_ = 100;
    bool is_charging_ = false;
    inline static bool battery_update_paused_ = false;

    // 充电趋势检测变量（毫伏）
    uint32_t current_avg_mv_ = 0;
    uint32_t checkpoint_mv_ = 0;
    int checkpoint_tick_ = 0;
    bool adc_charging_state_ = false;
    int charging_hold_ticks_ = 0;

    adc_oneshot_unit_handle_t adc_handle_;
    adc_cali_handle_t adc_cali_handle_ = nullptr;  // ADC 校准句柄

    // 原始ADC → 校准后毫伏 → 电池电压毫伏
    uint32_t RawAdcToBatteryMillivolts(int raw_adc) {
        int calibrated_mv = 0;
        if (adc_cali_handle_) {
            adc_cali_raw_to_voltage(adc_cali_handle_, raw_adc, &calibrated_mv);
        } else {
            calibrated_mv = static_cast<int>(static_cast<float>(raw_adc) * 3300.0f / 4095.0f);
        }
        return static_cast<uint32_t>(calibrated_mv * VOLTAGE_DIVIDER_RATIO);
    }

    void CheckBatteryStatus() {
      if (battery_update_paused_) {
        return;
      }

      ReadBatteryAdcData();

      if (charging_pin_ == GPIO_NUM_NC) {
#if CONFIG_IDF_TARGET_ESP32S3
        if (usb_serial_jtag_is_connected()) {
          is_charging_ = true;
        } else {
          is_charging_ = DetectChargingByVoltage();
        }
#else
        is_charging_ = DetectChargingByVoltage();
#endif
      } else {
        is_charging_ = gpio_get_level(charging_pin_) == 0;
      }
    }

    bool DetectChargingByVoltage() {
      if (current_avg_mv_ >= CHARGING_VOLTAGE_HIGH_THRESHOLD) {
        adc_charging_state_ = true;
        charging_hold_ticks_ = CHARGING_HOLD_TICKS;
        return true;
      }

      if (checkpoint_mv_ > 0 &&
          current_avg_mv_ >= checkpoint_mv_ + CHARGING_VOLTAGE_DELTA) {
        adc_charging_state_ = true;
        charging_hold_ticks_ = CHARGING_HOLD_TICKS;
        return true;
      }

      if (checkpoint_mv_ > 0 &&
          current_avg_mv_ <= checkpoint_mv_ - DISCHARGING_VOLTAGE_DELTA) {
        adc_charging_state_ = false;
        charging_hold_ticks_ = 0;
        return false;
      }

      if (charging_hold_ticks_ > 0) {
        charging_hold_ticks_--;
        return adc_charging_state_;
      }

      adc_charging_state_ = false;
      return false;
    }

    void UpdateChargingCheckpoint() {
      checkpoint_tick_++;
      if (checkpoint_tick_ >= CHARGING_CHECKPOINT_INTERVAL) {
        checkpoint_tick_ = 0;
        if (!adc_charging_state_ && current_avg_mv_ > 0) {
          checkpoint_mv_ = current_avg_mv_;
          ESP_LOGI("PowerManager", "充电检测基准更新: %lu mV", checkpoint_mv_);
        }
      }
    }

    void ReadBatteryAdcData() {
        int raw_adc;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_, adc_channel_, &raw_adc));

        // 原始ADC → 校准后电池电压毫伏
        uint32_t battery_mv = RawAdcToBatteryMillivolts(raw_adc);

        adc_values_[adc_values_index_] = battery_mv;
        adc_values_index_ = (adc_values_index_ + 1) % ADC_VALUES_COUNT;
        if (adc_values_count_ < ADC_VALUES_COUNT) {
            adc_values_count_++;
        }

        uint32_t average_mv = 0;
        for (size_t i = 0; i < adc_values_count_; i++) {
            average_mv += adc_values_[i];
        }
        average_mv /= adc_values_count_;

        current_avg_mv_ = average_mv;
        CalculateBatteryLevel(average_mv);

        if (checkpoint_mv_ == 0 && average_mv > 0 && adc_values_count_ >= 3) {
          checkpoint_mv_ = average_mv;
          ESP_LOGI("PowerManager", "充电检测初始基准: %lu mV", checkpoint_mv_);
        }

        if (charging_pin_ == GPIO_NUM_NC) {
          UpdateChargingCheckpoint();
        }
    }

    // 多点曲线电量计算
    void CalculateBatteryLevel(uint32_t millivolts) {
        if (millivolts <= BATTERY_LEVELS[0].millivolts) {
            battery_level_ = 0;
        } else if (millivolts >= BATTERY_LEVELS[BATTERY_LEVELS_COUNT - 1].millivolts) {
            battery_level_ = 100;
        } else {
            // 在相邻两点之间线性插值
            for (size_t i = 0; i < BATTERY_LEVELS_COUNT - 1; i++) {
                if (millivolts >= BATTERY_LEVELS[i].millivolts &&
                    millivolts < BATTERY_LEVELS[i + 1].millivolts) {
                    float ratio = static_cast<float>(millivolts - BATTERY_LEVELS[i].millivolts) /
                                  (BATTERY_LEVELS[i + 1].millivolts - BATTERY_LEVELS[i].millivolts);
                    battery_level_ = BATTERY_LEVELS[i].level +
                                     ratio * (BATTERY_LEVELS[i + 1].level - BATTERY_LEVELS[i].level);
                    break;
                }
            }
        }
    }

public:
    PowerManager(gpio_num_t charging_pin, adc_unit_t adc_unit = ADC_UNIT_2,
                 adc_channel_t adc_channel = ADC_CHANNEL_3)
        : charging_pin_(charging_pin), adc_unit_(adc_unit), adc_channel_(adc_channel) {

      if (charging_pin_ != GPIO_NUM_NC) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << charging_pin_);
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io_conf);
        ESP_LOGI("PowerManager", "充电检测引脚配置完成: GPIO%d", charging_pin_);
      } else {
        ESP_LOGI("PowerManager", "充电检测引脚未配置，将使用USB+ADC电压检测充电状态");
      }

        esp_timer_create_args_t timer_args = {
            .callback =
                [](void* arg) {
                    PowerManager* self = static_cast<PowerManager*>(arg);
                    self->CheckBatteryStatus();
                },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "battery_check_timer",
            .skip_unhandled_events = true,
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle_));
        ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle_, 1000000));

        InitializeAdc();
    }

    void InitializeAdc() {
      adc_oneshot_unit_init_cfg_t init_config = {
          .unit_id = adc_unit_,
          .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
          .ulp_mode = ADC_ULP_MODE_DISABLE,
      };
      ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle_));

      adc_oneshot_chan_cfg_t chan_config = {
          .atten = ADC_ATTEN_DB_12,
          .bitwidth = ADC_BITWIDTH_12,
      };
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, adc_channel_, &chan_config));

      // ADC 校准初始化（ESP32-S3 使用 curve_fitting 方案）
      adc_cali_curve_fitting_config_t cali_config = {
          .unit_id = adc_unit_,
          .atten = ADC_ATTEN_DB_12,
          .bitwidth = ADC_BITWIDTH_12,
      };
      esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle_);
      if (ret == ESP_OK) {
          ESP_LOGI("PowerManager", "ADC校准初始化成功（使用eFuse曲线拟合校准数据）");
      } else {
          ESP_LOGW("PowerManager", "ADC校准初始化失败，将使用粗略估算");
          adc_cali_handle_ = nullptr;
      }
    }

    ~PowerManager() {
        if (timer_handle_) {
            esp_timer_stop(timer_handle_);
            esp_timer_delete(timer_handle_);
        }
        if (adc_cali_handle_) {
            adc_cali_delete_scheme_curve_fitting(adc_cali_handle_);
        }
        if (adc_handle_) {
            adc_oneshot_del_unit(adc_handle_);
        }
    }

    bool IsCharging() { return is_charging_; }

    uint8_t GetBatteryLevel() { return battery_level_; }

    static void PauseBatteryUpdate() { battery_update_paused_ = true; }
    static void ResumeBatteryUpdate() { battery_update_paused_ = false; }
};
#endif  // __POWER_MANAGER_H__
