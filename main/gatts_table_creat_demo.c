/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* This file is for a gatt server CTF (capture the flag). 
*
****************************************************************************/


 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/event_groups.h"
 #include "esp_system.h"
 #include "esp_log.h"
 #include "nvs_flash.h"
 #include "esp_bt.h"
 #include "driver/gpio.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "gatts_table_creat_demo.h"
#include "esp_gatt_common_api.h"

#define GATTS_TABLE_TAG "ESP_GATTS_DEMO"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "Don't trust the intern."
#define SVC_INST_ID                 0

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 100
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)


static uint8_t adv_config_done       = 0;

//HRS_IDX_NB += 200;
uint16_t blectf_handle_table[HRS_IDX_NB+50];// =51

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;

#define CONFIG_SET_RAW_ADV_DATA
#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power*/
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF, 0x00,
        /* device name (first number is the length) */
        0x0A, 0x09, 'B', 'L', 'E', 'C', 'T','F','F','U','N'

};
static uint8_t raw_scan_rsp_data[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power */
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF,0x00
};

#else
static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 16,
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst blectf_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_TEST                   = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_FLAG			= 0xFF00;    
static const uint16_t GATTS_CHAR_UUID_SCORE                     = 0xFF01;

static const uint16_t GATTS_CHAR_UUID_NAME			= 0xFF02;
static const uint16_t GATTS_CHAR_UUID_HLTH			= 0xFF03;

static const uint16_t GATTS_CHAR_UUID_OLD1			= 0xFF04;
static const uint16_t GATTS_CHAR_UUID_OLD2			= 0xFF05;
static const uint16_t GATTS_CHAR_UUID_OLD3			= 0xFF06;
static const uint16_t GATTS_CHAR_UUID_BART			= 0xFF07;

static const uint16_t GATTS_CHAR_UUID_CROS			= 0xFF08;
static const uint16_t GATTS_CHAR_UUID_SCAB			= 0xFF09;
static const uint16_t GATTS_CHAR_UUID_BLAD			= 0xFF0A;
static const uint16_t GATTS_CHAR_UUID_POMM			= 0xFF0B;
static const uint16_t GATTS_CHAR_UUID_HILT			= 0xFF0C;
static const uint16_t GATTS_CHAR_UUID_BLKS			= 0xFF0D;

static const uint16_t GATTS_CHAR_UUID_TROL			= 0xFF0E;

static const uint16_t GATTS_CHAR_UUID_TERR			= 0xFF0F;



static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                =  ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write_indicate   = ESP_GATT_CHAR_PROP_BIT_WRITE |ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE;
static const uint8_t char_prop_read_write   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_crazy   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_EXT_PROP | ESP_GATT_CHAR_PROP_BIT_BROADCAST |  ESP_GATT_CHAR_PROP_BIT_NOTIFY ;
static const uint8_t heart_measurement_ccc[2]      = {0x00, 0x00};
static const uint8_t char_value[4]                 = {0x11, 0x22, 0x33, 0x44};

// start ctf data vars
static char writeData[100];
static char flag_state[20] = {'F','F','F','F','F','F','F','F','F','F','F','F','F','F','F','F','F','F','F','F'};
static uint8_t score_read_value[11] = {'S', 'c', 'o', 'r', 'e', ':', ' ', '0','/','2','0'};


//static const uint8_t brute_write_flag[33] = {'B','r','u','t','e',' ','f','o','r','c','e',' ','m','y',' ','v','a','l','u','e', ' ', '0','x','0','0',' ','t','o',' ','0','x','f','f'};
static const uint8_t flag_read_value[16] = {'W','r','i','t','e', ' ', 'F', 'l','a','g','s', ' ', 'H','e','r', 'e'};
static const char name_value[] = "NAME HERE";
static const char hlth_value[] = "H H H";
static const char old1_value[] = "Old Gent";
static const char old2_value[] = "Old Gent";
static const char old3_value[] = "Old Gent";
static const char bart_value[] = "Bartender";
static const char pomm_value[] = "Pommel";
static const char hilt_value[] = "Hilt";
static const char cros_value[] = "Crossguard";
static const char blad_value[] = "Blade";
static const char scab_value[] = "Scabbard";
static const char blks_value[] = "Blacksmith";
static const char trol_value[] = "Troll";
static const char terr_value[] = "Terrain";


int score = 0;
static char string_score[10] = "0";
int BLINK_GPIO=2;
int indicate_handle_state = 0;
int send_response=0;
int check_send_response=0;
char quizScore[3]={'F','F','F'};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB+50] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TEST), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},

    /* SCORE Characteristic Declaration */
    [IDX_CHAR_SCORE]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* SCORE Characteristic Value */
    [IDX_CHAR_VAL_SCORE]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_SCORE, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(score_read_value), (uint8_t *)score_read_value}},

    /* FLAG Characteristic Declaration */
    [IDX_CHAR_FLAG]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* FLAG Characteristic Value */
    [IDX_CHAR_VAL_FLAG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_FLAG, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(flag_read_value), (uint8_t *)flag_read_value}},

    //////	STATS	//////
    /* NAME Characteristic Declaration */
    [IDX_CHAR_NAME]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* NAME Characteristic Value */
    [IDX_CHAR_VAL_NAME]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_NAME, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(name_value), (uint8_t *)name_value}},

    /* HLTH Characteristic Declaration */
    [IDX_CHAR_HLTH]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* HLTH Characteristic Value */
    [IDX_CHAR_VAL_HLTH]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_HLTH, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(hlth_value), (uint8_t *)hlth_value}},

    //////	OLD GENTS + BARTENDER	//////
    /* OLD1 Characteristic Declaration */
    [IDX_CHAR_OLD1]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_OLD1] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_OLD1, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(old1_value)-1, (uint8_t *)old1_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_OLD1]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(old1_value)-1, (uint8_t *)old1_value}},

    /* OLD2 Characteristic Declaration */
    [IDX_CHAR_OLD2]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_OLD2] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_OLD2, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(old2_value)-1, (uint8_t *)old2_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_OLD2]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(old2_value)-1, (uint8_t *)old2_value}},

    /* OL3 Characteristic Declaration */
    [IDX_CHAR_OLD3]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_OLD3] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_OLD3, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(old3_value)-1, (uint8_t *)old3_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_OLD3]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(old3_value)-1, (uint8_t *)old3_value}},

    /* BART Characteristic Declaration */
    [IDX_CHAR_BART]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_BART] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_BART, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(bart_value)-1, (uint8_t *)bart_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_BART]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(bart_value)-1, (uint8_t *)bart_value}},

    //////	COLLECTION: SWORD	//////
    /* POMM Characteristic Declaration */
    [IDX_CHAR_POMM]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_POMM] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_POMM, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(pomm_value)-1, (uint8_t *)pomm_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_POMM]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(pomm_value)-1, (uint8_t *)pomm_value}},

    /* HILT Characteristic Declaration */
    [IDX_CHAR_HILT]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_HILT] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_HILT, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(hilt_value)-1, (uint8_t *)hilt_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_HILT]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(hilt_value)-1, (uint8_t *)hilt_value}},

    /* BLAD Characteristic Declaration */
    [IDX_CHAR_BLAD]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_BLAD] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_BLAD, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(blad_value)-1, (uint8_t *)blad_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_BLAD]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(blad_value)-1, (uint8_t *)blad_value}},

    /* SCAB Characteristic Declaration */
    [IDX_CHAR_SCAB]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_SCAB] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_SCAB, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(scab_value)-1, (uint8_t *)scab_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_SCAB]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(scab_value)-1, (uint8_t *)scab_value}},

    /* CROS Characteristic Declaration */
    [IDX_CHAR_CROS]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_CROS] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_CROS, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(cros_value)-1, (uint8_t *)cros_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_CROS]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(cros_value)-1, (uint8_t *)cros_value}},
   
    ////// BLACKSMITH	//////
    /* BLKS Characteristic Declaration */
    [IDX_CHAR_BLKS]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_BLKS] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_BLKS, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(blks_value)-1, (uint8_t *)blks_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_BLKS]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(blks_value)-1, (uint8_t *)blks_value}},

    ////// TROLL	//////
    /* BLKS Characteristic Declaration */
    [IDX_CHAR_TROL]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_TROL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TROL, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(trol_value)-1, (uint8_t *)trol_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_TROL]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(trol_value)-1, (uint8_t *)trol_value}},

    ////// TERRAIN	//////
    /* BLKS Characteristic Declaration */
    [IDX_CHAR_TERR]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_TERR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TERR, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(terr_value)-1, (uint8_t *)terr_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_TERR]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(terr_value)-1, (uint8_t *)terr_value}},





};
static int get_score(char c[20])
{
	int score = 0;
	for (int i = 0 ; i < 20 ; ++i)
  	{
        	if (c[i] == 'T'){
           		score += 1;
        	}
    	}
	return score;

}
static void set_score()
{
    //set scores
    score = get_score(flag_state);
    
    
    itoa(score, string_score, 10);
    for (int i = 0 ; i < strlen(string_score) ; ++i)
    {
        if (strlen(string_score) == 1){
            score_read_value[7] = ' ';}
        score_read_value[6+i] = string_score[i];
    }
    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_SCORE]+1, sizeof score_read_value, score_read_value);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    #ifdef CONFIG_SET_RAW_ADV_DATA
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #else
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #endif
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed");
            }else{
                ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully");
		ESP_LOGI(GATTS_TABLE_TAG, "blade = 756hd");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "update connetion params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(GATTS_TABLE_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(GATTS_TABLE_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(GATTS_TABLE_TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(GATTS_TABLE_TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        esp_log_buffer_hex(GATTS_TABLE_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(GATTS_TABLE_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_REG_EVT");
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
    #ifdef CONFIG_SET_RAW_ADV_DATA
            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if (raw_adv_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
    #else
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
    #endif
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB+50, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");

            //set gpio
            gpio_pad_select_gpio(BLINK_GPIO);
            /* Set the GPIO as a push/pull output */
            gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
            gpio_set_level(BLINK_GPIO, 1);


      

       	    break;
        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_WRITE_EVT");
            
            if (!param->write.is_prep){
                ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                
                // store write data for flag checking
                memset(writeData, 0, sizeof writeData);
                memcpy(writeData, param->write.value, 20); 		

		if(blectf_handle_table[IDX_CHAR_NAME]+1 == param->write.handle)
		{
		  esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_NAME]+1, 20, (uint8_t *)"thisistheflagwelcome");
		
		}


		// old1
                if (blectf_handle_table[IDX_CHAR_OLD1]+1 == param->write.handle)
                {
                    char notify_data[20] = ".3H...bit.ly/2LOGYtf";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_OLD1]+1, sizeof(old1_value)-1, (uint8_t *)old1_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_OLD1], sizeof(notify_data), (uint8_t *)notify_data, false);
                }
		// old2
                if (blectf_handle_table[IDX_CHAR_OLD2]+1 == param->write.handle)
                {
                    char notify_data[20] = ".6E...bit.ly/2v3Qlhq";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_OLD2]+1, sizeof(old2_value)-1, (uint8_t *)old2_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_OLD2], sizeof(notify_data), (uint8_t *)notify_data, false);
                }
		// old3
                if (blectf_handle_table[IDX_CHAR_OLD3]+1 == param->write.handle)
                {
                    char notify_data[20] = "11Y..bit.ly/2NLcVmC";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_OLD3]+1, sizeof(old3_value)-1, (uint8_t *)old3_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_OLD3], sizeof(notify_data), (uint8_t *)notify_data, false);
                }

		// bart
		if (blectf_handle_table[IDX_CHAR_BART]+1 == param->write.handle && strcmp(writeData,"sword?") == 0)
                {
                    char notify_data[20] = "gather5pieces4sword!";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_BART]+1, sizeof(bart_value)-1, (uint8_t *)bart_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_BART], sizeof(notify_data), (uint8_t *)notify_data, false);
                }

		//////	SWORD	//////
		// pommel
		if (blectf_handle_table[IDX_CHAR_POMM]+1 == param->write.handle)
                {
                    char notify_data[4] = "yt87";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_POMM]+1, sizeof(pomm_value)-1, (uint8_t *)pomm_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_POMM], sizeof(notify_data), (uint8_t *)notify_data, false);
                }
		// blade
		if (blectf_handle_table[IDX_CHAR_BLAD]+1 == param->write.handle)
                {
                    char notify_data[] = "bladedoesntliveherecheckLOGI";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_BLAD]+1, sizeof(blad_value)-1, (uint8_t *)blad_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_BLAD], sizeof(notify_data), (uint8_t *)notify_data, false);
                }
		// hilt
		if (blectf_handle_table[IDX_CHAR_HILT]+1 == param->write.handle)
                {
                    char notify_data[5] = "a23bg";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_HILT]+1, sizeof(hilt_value)-1, (uint8_t *)hilt_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_HILT], sizeof(notify_data), (uint8_t *)notify_data, false);
                }
		// crossguard
                if (blectf_handle_table[IDX_CHAR_CROS]+1 == param->write.handle)
                {
                    char notify_data[4] = "56gt";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_CROS]+1, sizeof(cros_value)-1, (uint8_t *)cros_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_CROS], sizeof(notify_data), (uint8_t *)notify_data, false);
                }
		//scabbard
                if (blectf_handle_table[IDX_CHAR_SCAB]+1 == param->write.handle)
                {
                    char notify_data[2] = "66";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_SCAB]+1, sizeof(scab_value)-1, (uint8_t *)scab_value);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_SCAB], sizeof(notify_data), (uint8_t *)notify_data, false);
                }

		// Blacksmith
		if (blectf_handle_table[IDX_CHAR_BLKS]+1 == param->write.handle)
                {
                    char notify_data[20] = "";
		    char notify_data1[20] = "ordertheswordfrompom";
		    char notify_data2[20] = "meltoblade,scabbardi";
		    char notify_data3[5] = "slast";

		    // if correct give flag
		    if(strcmp(writeData,"replace me") == 0){
			esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_BLKS]+1, sizeof(blks_value)-1, (uint8_t *)blks_value);
             	        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_BLKS], sizeof(notify_data), (uint8_t *)notify_data, false);
		    }
		    else{// else tells them to order keys from bottom to top and last is scabbard
			esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_BLKS]+1, sizeof(blks_value)-1, (uint8_t *)blks_value);
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_BLKS], sizeof(notify_data1), (uint8_t *)notify_data1, false);
			esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_BLKS], sizeof(notify_data2), (uint8_t *)notify_data2, false);		
			esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_BLKS], sizeof(notify_data3), (uint8_t *)notify_data3, false);
		    }
                    
                }	
		
            	// Troll
		// interactive order, track questions right
		// give passage flag when done
		char question1[20]="Flags are 20 charact";
		char question1b[20] = "ers to avoid what li";
		char question1c[5] = "mits?";//A: MTU

		char question2[20]="What's length of cus";
		char question2b[17] = "tom uuid in bits?";//A: 128

		char question3[20]="In BLE architecture"; 
		char question3b[20]="what's under HW othe";
		char question3c[12]="r that UART?";//A: BLESS
		char correct[20]="Correct go to next Q";
		
		if (blectf_handle_table[IDX_CHAR_TROL]+1 == param->write.handle)
                {
		    char notify_data[20]="qazwsxedcrfvtgbyhnuj";
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_TROL]+1, sizeof(trol_value)-1, (uint8_t *)trol_value);
		    //ESP_LOGI(GATTS_TABLE_TAG, "quizscore = %s", quizScore);
		    if(get_score(quizScore) == 0){
			if(strcmp(writeData,"MTU") == 0){
				quizScore[0]='T';
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(correct), (uint8_t *)correct, false);			
			}else{
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question1), (uint8_t *)question1, false);
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question1b), (uint8_t *)question1b, false);
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question1c), (uint8_t *)question1c, false);
			}
			//ESP_LOGI(GATTS_TABLE_TAG, "writeQ1 = %s", writeData);
			
		    }
		    else if(get_score(quizScore) == 1){
			if(strcmp(writeData,"128") == 0){
				quizScore[1]='T';
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(correct), (uint8_t *)correct, false);			
			}else{
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question2), (uint8_t *)question2, false);
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question2b), (uint8_t *)question2b, false);
			}
		
		    }
		    else if(get_score(quizScore) == 2){
			if(strcmp(writeData,"BLESS") == 0){
				quizScore[2]='T';
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(correct), (uint8_t *)correct, false);			
			}else{	
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question3), (uint8_t *)question3, false);
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question3b), (uint8_t *)question3b, false);
				esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(question3c), (uint8_t *)question3c, false);
			}
			
		    }

		    else if(get_score(quizScore) == 3){
		    	esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TROL], sizeof(notify_data), (uint8_t *)notify_data, false);
		    }
                }

		// Terrain/Goblins
		// buffer overflow...well kinda...if you dont look at code it seems like one
		if (blectf_handle_table[IDX_CHAR_TERR]+1 == param->write.handle)
                {
		    
                    char full[18] = "Current Health:...";
		    char output[15] = "Current Health:";
		    char notify_data[20] = "overoveroveroverflow";
		    memcpy(full,writeData,18);
		    printf("full: %s",full);
		   
		    if(full[15] == '0' && full[16] == '0' && full[17] == '0'){
			esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_TERR]+1, sizeof(terr_value)-1, (uint8_t *)terr_value);
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TERR], sizeof(notify_data), (uint8_t *)notify_data, false);
		    }
		    else{
                	esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_TERR]+1, sizeof(terr_value)-1, (uint8_t *)terr_value);
                	esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, blectf_handle_table[IDX_CHAR_TERR], sizeof(output), (uint8_t *)output, false);
		    }
                }

		//Easter eggs
		// give yourself more health
		if(blectf_handle_table[IDX_CHAR_HLTH]+1 == param->write.handle){
			esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_HLTH]+1, 11, (uint8_t *)"No cheating");
		}

                //handle flags
                if (blectf_handle_table[IDX_CHAR_FLAG]+1 == param->write.handle)
                {
                    // make sure flag read value stays static
                    esp_ble_gatts_set_attr_value(blectf_handle_table[IDX_CHAR_FLAG]+1, sizeof flag_read_value, flag_read_value);

		       //TODO: fix keys to be something random
                    if (strcmp(writeData,"thisistheflagwelcome") == 0){
             		// name gimme
                        flag_state[0] = 'T';
                    }
		    if (strcmp(writeData,"HHHEEEEEEYYYYYYYYYYY") == 0){
             		// name gimme
                        flag_state[1] = 'T';
                    }	
                    if (strcmp(writeData,"gather5pieces4sword!") == 0){
             		// name gimme
                        flag_state[2] = 'T';
                    }
		    if(strcmp(writeData,"replace me") == 0){
		        flag_state[3] = 'T';
		    }
                    ESP_LOGI(GATTS_TABLE_TAG, "FLAG STATE = %s", flag_state);
                    set_score();
                }
                /* send response when param->write.need_rsp is true*/
                //if (param->write.need_rsp && send_response == 0){
                if (param->write.need_rsp){
                    ESP_LOGI(GATTS_TABLE_TAG, "CATCH ALL SEND RESPONSE TRIGGERED");
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }
            else{
                /* handle prepare write */
                ESP_LOGI(GATTS_TABLE_TAG, "PREPARE WRITE TRIGGERED");
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
         
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d", param->conf.status);
           
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
            

            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
	    quizScore[0]='F';
	    quizScore[1]='F';
	    quizScore[2]='F';
            indicate_handle_state=0;
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != HRS_IDX_NB+50){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB+50);
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(blectf_handle_table, param->add_attr_tab.handles, sizeof(blectf_handle_table));
                esp_ble_gatts_start_service(blectf_handle_table[IDX_SVC]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CLOSE_EVT");
            break;
        case ESP_GATTS_LISTEN_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_LISTEN_EVT");
            break;
        case ESP_GATTS_CONGEST_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONGEST_EVT");
            break;
        case ESP_GATTS_UNREG_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_UNREG_EVT");
            break;
        case ESP_GATTS_DELETE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DELETE_EVT");
            break;
        case ESP_GATTS_RESPONSE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_RESPONSE_EVT");
            
            break;
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            blectf_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == blectf_profile_tab[idx].gatts_if) {
                if (blectf_profile_tab[idx].gatts_cb) {
                    blectf_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void app_main()
{
    esp_err_t ret;

    //uint8_t new_mac[8] = {0xDE,0xAD,0xBE,0xEF,0xBE,0xEF};
    //esp_base_mac_addr_set(new_mac);
    
    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed", __func__);
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}
