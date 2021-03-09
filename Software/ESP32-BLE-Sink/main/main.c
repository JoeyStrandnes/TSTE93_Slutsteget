/* Play music from Bluetooth device

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "esp_peripherals.h"
#include "periph_touch.h"
#include "periph_adc_button.h"
#include "periph_button.h"
#include "filter_resample.h"
#include "audio_mem.h"
#include "bluetooth_service.h"


static const char *TAG = "BLE Sink";


void app_main(void){

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);


    audio_pipeline_handle_t pipeline;
    audio_element_handle_t bt_stream_reader, i2s_stream_writer;

    nvs_flash_init();

// ******************************************************************
    ESP_LOGI(TAG, "Create Bluetooth service");

    bluetooth_service_cfg_t bt_cfg = {
        .device_name = "OwO BLE Spwekewr",
        .mode = BLUETOOTH_A2DP_SINK,
    };
    bluetooth_service_start(&bt_cfg);


// ******************************************************************
    
    ESP_LOGI(TAG, "Create audio pipeline for playback");

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);    


// ******************************************************************

    ESP_LOGI(TAG, "Create i2s stream to write data to codec chip");
    
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);


    ESP_LOGI(TAG, "Get Bluetooth stream");
    bt_stream_reader = bluetooth_service_create_stream();

    ESP_LOGI(TAG, "Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, bt_stream_reader, "bt");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");


    ESP_LOGI(TAG, "Link it together [Bluetooth]-->bt_stream_reader-->i2s_stream_writer-->[codec_chip]");
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 44100;
    rsp_cfg.src_ch = 2;
    rsp_cfg.dest_rate = 48000;
    rsp_cfg.dest_ch = 2;
    audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);
    audio_pipeline_register(pipeline, filter, "filter");
    i2s_stream_set_clk(i2s_stream_writer, 48000, 16, 2);
    const char *link_tag[3] = {"bt", "filter", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);


    ESP_LOGI(TAG, "Initialize peripherals");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    //ESP_LOGI(TAG, "Initialize Touch peripheral");
    //audio_board_key_init(set);

    ESP_LOGI(TAG, "Create Bluetooth peripheral");
    esp_periph_handle_t bt_periph = bluetooth_service_create_periph();

    ESP_LOGI(TAG, "Start all peripherals");
    esp_periph_start(set, bt_periph);

    ESP_LOGI(TAG, "Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGI(TAG, "Start audio_pipeline");
    audio_pipeline_run(pipeline);
// ******************************************************************

    while(1){

        audio_event_iface_msg_t msg;
        audio_event_iface_listen(evt, &msg, portMAX_DELAY);

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) bt_stream_reader && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            
            audio_element_info_t music_info = {0};
            audio_element_getinfo(bt_stream_reader, &music_info);

            audio_element_setinfo(i2s_stream_writer, &music_info);

            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }


               // Stop when the Bluetooth is disconnected or suspended
        if (msg.source_type == PERIPH_ID_BLUETOOTH
            && msg.source == (void *)bt_periph) {
            if (msg.cmd == PERIPH_BLUETOOTH_DISCONNECTED) {
                ESP_LOGW(TAG, "[ * ] Bluetooth disconnected");
                break;
            }
        }
        // Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }

    }



    ESP_LOGI(TAG, "Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, bt_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all peripherals before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    // Release all resources

    audio_pipeline_unregister(pipeline, filter);
    audio_element_deinit(filter);


}
