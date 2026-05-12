#include "adc_sensors.h"
#include "config.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "adc";

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static bool do_calibration;

int adc_sensors_init(void)
{
    /* ADC1 oneshot init */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));

    /* Configure all three ADC channels: 11dB attenuation (~2.5V full-scale) */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &chan_cfg));  /* MAX9814 */
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &chan_cfg));  /* MQ-7 */
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &chan_cfg));  /* MQ-135 */

    /* Attempt calibration (ESP32-C3 may support eFuse calibration values) */
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    do_calibration = (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK);

    ESP_LOGI(TAG, "ADC1 initialized (CH0=MAX9814, CH1=MQ-7, CH3=MQ-135), calibration=%s",
             do_calibration ? "yes" : "no");
    return 0;
}

void adc_sensors_deinit(void)
{
    if (cali_handle) {
        adc_cali_delete_scheme_curve_fitting(cali_handle);
        cali_handle = NULL;
        do_calibration = false;
    }
    if (adc1_handle) {
        adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
    }
    ESP_LOGI(TAG, "ADC1 oneshot deinitialized");
}

static int read_channel(adc_channel_t chan)
{
    if (!adc1_handle) return -1;

    int raw;
    esp_err_t ret = adc_oneshot_read(adc1_handle, chan, &raw);
    if (ret != ESP_OK) return -1;

    if (do_calibration) {
        int mv;
        if (adc_cali_raw_to_voltage(cali_handle, raw, &mv) == ESP_OK) {
            return mv;  /* return calibrated mV */
        }
    }
    return raw;
}

int adc_mq7_read_raw(void)
{
    return read_channel(ADC_CHANNEL_1);
}

int adc_mq135_read_raw(void)
{
    return read_channel(ADC_CHANNEL_3);
}

int adc_max9814_read_raw(void)
{
    return read_channel(ADC_CHANNEL_0);
}
