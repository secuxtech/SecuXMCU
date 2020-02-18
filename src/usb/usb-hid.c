#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_hid_generic.h"
#include "app_usbd_hid_mouse.h"
#include "app_usbd_hid_kbd.h"
#include "app_error.h"
#include "bsp.h"
#include "aes.h"
//#include "bsp_cli.h"
//#include "nrf_cli.h"
//#include "nrf_cli_uart.h"
//#include "nrf_cli_rtt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "preloader_cmd_lst.h"
#include "hmac_sha384.h"
#include "nrf_delay.h"





bool g_key0_f=false;
bool g_key1_f=false;
bool g_key2_f=false;
bool g_key3_f=false;

uint8_t g_select_method = 0;

enum {
	kMenuMin = 0,
	kMenuGetModeState = kMenuMin,
	kMenuGetFWVersion,
	kMenuGetModularError,
	kMenuInitDeviceInfo,
	kMenuBindingRegister,
	kMenuBindingLogin,
	kMenuBindingLogout,
	kMenuBackToInitState,
	kMenuInitWallet,
	kMenuQueryWalletInfo,
	kMenuCreateAccount,
	kMenuQueryAccountInfo,
	kMenuQueryAccountKeyInfo,
	kMenuMnemonicToSeed,
	kMenuGenerateNextAddress,
	kMenuPrepareTransactionSign,
	kMenuTransactionSignBegin,
	kMenuTransactionSign,
	kMenuTransactionSignFinish,
	kMenuBackToIKVLoader,
	kMenuJumpToCos,
	kMenuPRELOADER_KV_LOADSESSAUTH,
	kMenuPRELOADER_KV_MAC_VERIFY_AND_BURN,
	kMenuMax = kMenuPRELOADER_KV_MAC_VERIFY_AND_BURN
} kMenu;

int8_t get_mode_state(uint8_t*);
int8_t get_fw_version(uint8_t *response);
int8_t get_modular_error(uint8_t *response);
int8_t init_device_info(void);
int binding_register(void);
int binding_login(void);
int binding_logout(void);
int back_to_init_state(void);
int init_wallet(void);
int hdw_query_wallet_info(void);
int hdw_create_account(void);
int hdw_query_account_info(void);
int hdw_query_account_key_info(void);
int mnemonic_to_seed(void);
int hdw_generate_next_trx_addr(void);
int prepare_transaction_sign(void);
int transaction_sign_begin(void);
int transaction_sign(void);
int transaction_sign_finish(void);
int BackToIKVldr(void);
//int loader_cmdhdlr_LoadsessAuth	(void);
//int preloader_kv_mac_verify_and_burn(void);


#define HID_REPORT_LENGTH 			64
#define APDU_COMMAND_QUEUE_SIZE 128

extern uint8_t g_apdu_command_received;
//uint8_t g_apdu_command_processed = 0;
//uint8_t g_apdu_command_buffer[APDU_COMMAND_QUEUE_SIZE][HID_REPORT_LENGTH] = {0};
//uint8_t g_apdu_command_response[HID_REPORT_LENGTH] = {0};
extern uint8_t g_apdu_command_buffer[APDU_COMMAND_QUEUE_SIZE][HID_REPORT_LENGTH];

int check_SE_program(void);
int loader_cmdhdlr_CosJump(void);
void init_default_value(void);
int apdu_command_parsing(uint8_t *apdu_command, uint8_t *response, uint16_t *response_size);
void response_handler(uint8_t *buffer, uint16_t response_length, 
	uint8_t response_state, uint8_t *response, uint16_t *response_size);

void spim_init(void);

/**
 * @brief CLI interface over UART
 */
#if 0
NRF_CLI_UART_DEF(m_cli_uart_transport, 0, 64, 16);
NRF_CLI_DEF(m_cli_uart,
            "uart_cli:~$ ",
            &m_cli_uart_transport.transport,
            '\r',
            4);


NRF_CLI_RTT_DEF(m_cli_rtt_transport);
NRF_CLI_DEF(m_cli_rtt,
            "rtt_cli:~$ ",
            &m_cli_rtt_transport.transport,
            '\n',
            4);
#endif
/**
 * @brief Enable USB power detection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

/**
 * @brief HID generic class interface number.
 * */
#define HID_GENERIC_INTERFACE  0

/**
 * @brief HID generic class endpoint number.
 * */
#define HID_GENERIC_EPIN       NRF_DRV_USBD_EPIN1

//secux
#define HID_GENERIC_EPOUT       NRF_DRV_USBD_EPOUT1
/**
 * @brief Mouse speed (value sent via HID when board button is pressed).
 * */
#define CONFIG_MOUSE_MOVE_SPEED (3)

/**
 * @brief Mouse move repeat time in milliseconds
 */
#define CONFIG_MOUSE_MOVE_TIME_MS (5)


/* GPIO used as LED & buttons in this example */
#define LED_USB_START    (BSP_BOARD_LED_0)
#define LED_HID_REP_IN   (BSP_BOARD_LED_2)

#define BTN_MOUSE_X_POS  0
#define BTN_MOUSE_Y_POS  1
#define BTN_MOUSE_LEFT   2
#define BTN_MOUSE_RIGHT  3

/**
 * @brief Left button mask in buttons report
 */
#define HID_BTN_LEFT_MASK  (1U << 0)

/**
 * @brief Right button mask in buttons report
 */
#define HID_BTN_RIGHT_MASK (1U << 1)

/* HID report layout */
#define HID_BTN_IDX   0 /**< Button bit mask position */
#define HID_X_IDX     1 /**< X offset position */
#define HID_Y_IDX     2 /**< Y offset position */
#define HID_W_IDX     3 /**< Wheel position  */
#define HID_REP_SIZE  4 /**< The size of the report */

/**
 * @brief Number of reports defined in report descriptor.
 */
#define REPORT_IN_QUEUE_SIZE    1 

/**
 * @brief Size of maximum output report. HID generic class will reserve
 *        this buffer size + 1 memory space. */
#define REPORT_OUT_MAXSIZE  63 //secux

/**
 * @brief HID generic class endpoints count.
 * */
#define HID_GENERIC_EP_COUNT  1 //secux

/**
 * @brief List of HID generic class endpoints.
 * */
 //secux
#define ENDPOINT_LIST()                                      \
(                                                            \
        HID_GENERIC_EPIN,                                    \
				HID_GENERIC_EPOUT																		 \
)

/**
 * @brief Additional key release events
 *
 * This example needs to process release events of used buttons
 */
enum {
    BSP_USER_EVENT_RELEASE_0 = BSP_EVENT_KEY_LAST + 1, /**< Button 0 released */
    BSP_USER_EVENT_RELEASE_1,                          /**< Button 1 released */
    BSP_USER_EVENT_RELEASE_2,                          /**< Button 2 released */
    BSP_USER_EVENT_RELEASE_3,                          /**< Button 3 released */
    BSP_USER_EVENT_RELEASE_4,                          /**< Button 4 released */
    BSP_USER_EVENT_RELEASE_5,                          /**< Button 5 released */
    BSP_USER_EVENT_RELEASE_6,                          /**< Button 6 released */
    BSP_USER_EVENT_RELEASE_7,                          /**< Button 7 released */
};

/**
 * @brief HID generic mouse action types
 */
typedef enum {
    HID_GENERIC_MOUSE_X,
    HID_GENERIC_MOUSE_Y,
    HID_GENERIC_MOUSE_BTN_LEFT,
    HID_GENERIC_MOUSE_BTN_RIGHT,
} hid_generic_mouse_action_t;

/**
 * @brief User event handler.
 * */
static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event);
//seucx
#define HID_ReportDesc() { \
	0x06, 0xA0, 0xFF,       /* Usage page (vendor defined)        */\
  0x09, 0x01,     /* Usage ID (vendor defined)                  */\
  0xA1, 0x01,     /* Collection (application)                   */\
  0x09, 0x03,             /* Usage ID - vendor defined          */\
  0x15, 0x00,             /* Logical Minimum (0)                */\
  0x26, 0xFF, 0x00,   /* Logical Maximum (255)                  */\
  0x75, 0x08,             /* Report Size (8 bits)               */\
  0x95, 0x40,             /* Report Count (64 fields)           */\
  0x81, 0x08,             /* Input (Data, Variable, Absolute)   */\
  0x09, 0x04,             /* Usage ID - vendor defined          */\
  0x15, 0x00,             /* Logical Minimum (0)                */\
  0x26, 0xFF, 0x00,   /* Logical Maximum (255)                  */\
  0x75, 0x08,             /* Report Size (8 bits)               */\
  0x95, 0x40,             /* Report Count (64 fields)           */\
  0x91, 0x08,             /* Output (Data, Variable, Absolute)  */\
  0xC0                                                          \
}

/**
 * @brief Reuse HID mouse report descriptor for HID generic class
 */
APP_USBD_HID_GENERIC_SUBCLASS_REPORT_DESC(mouse_desc, HID_ReportDesc());

static const app_usbd_hid_subclass_desc_t * reps[] = {&mouse_desc};

/*lint -save -e26 -e64 -e123 -e505 -e651*/

/**
 * @brief Global HID generic instance
 */
APP_USBD_HID_GENERIC_GLOBAL_DEF(m_app_hid_generic,
                                HID_GENERIC_INTERFACE,
                                hid_user_ev_handler,
                                ENDPOINT_LIST(),
                                reps,
                                REPORT_IN_QUEUE_SIZE,
                                REPORT_OUT_MAXSIZE,
                                APP_USBD_HID_SUBCLASS_NONE, //APP_USBD_HID_SUBCLASS_BOOT, //secux
                                APP_USBD_HID_PROTO_GENERIC); //APP_USBD_HID_PROTO_MOUSE); //secux

/*lint -restore*/


/**
 * @brief Mouse state
 *
 * Current mouse status
 */
struct
{
    int16_t acc_x;    /**< Accumulated x state */
    int16_t acc_y;    /**< Accumulated y state */
    uint8_t btn;      /**< Current btn state */
    uint8_t last_btn; /**< Last transfered button state */
}m_mouse_state;

/**
 * @brief Mark the ongoing transmission
 *
 * Marks that the report buffer is busy and cannot be used until transmission finishes
 * or invalidates (by USB reset or suspend event).
 */
static bool m_report_pending;

/**
 * @brief Timer to repeat mouse move
 */
//APP_TIMER_DEF(m_mouse_move_timer);

/**
 * @brief Get maximal allowed accumulated value
 *
 * Function gets maximal value from the accumulated input.
 * @sa m_mouse_state::acc_x, m_mouse_state::acc_y
 */
static int8_t hid_acc_for_report_get(int16_t acc)
{
    if(acc > INT8_MAX)
    {
        return INT8_MAX;
    }
    else if(acc < INT8_MIN)
    {
        return INT8_MIN;
    }
    else
    {
        return (int8_t)(acc);
    }
}
#if 0
/**
 * @brief Internal function that process mouse state
 *
 * This function checks current mouse state and tries to send
 * new report if required.
 * If report sending was successful it clears accumulated positions
 * and mark last button state that was transfered.
 */
static void hid_generic_mouse_process_state(void)
{
    if (m_report_pending)
        return;
    if ((m_mouse_state.acc_x != 0) ||
        (m_mouse_state.acc_y != 0) ||
        (m_mouse_state.btn != m_mouse_state.last_btn))
    {
        ret_code_t ret;
			
			static uint8_t report_secux[64];
			memset(report_secux, 2, sizeof(report_secux));
			
        static uint8_t report[HID_REP_SIZE];
        /* We have some status changed that we need to transfer */
        report[HID_BTN_IDX] = m_mouse_state.btn;
        report[HID_X_IDX]   = (uint8_t)hid_acc_for_report_get(m_mouse_state.acc_x);
        report[HID_Y_IDX]   = (uint8_t)hid_acc_for_report_get(m_mouse_state.acc_y);
        /* Start the transfer */
			
        ret = app_usbd_hid_generic_in_report_set(
            &m_app_hid_generic,
            report_secux,
            sizeof(report_secux));
			
        if (ret == NRF_SUCCESS)
        {
            m_report_pending = true;
            m_mouse_state.last_btn = report[HID_BTN_IDX];
            CRITICAL_REGION_ENTER();
            /* This part of the code can fail if interrupted by BSP keys processing.
             * Lock interrupts to be safe */
            m_mouse_state.acc_x   -= (int8_t)report[HID_X_IDX];
            m_mouse_state.acc_y   -= (int8_t)report[HID_Y_IDX];
            CRITICAL_REGION_EXIT();
        }
    }
}

/**
 * @brief HID generic IN report send handling
 * */
static void hid_generic_mouse_action(hid_generic_mouse_action_t action, int8_t param)
{
    CRITICAL_REGION_ENTER();
    /*
     * Update mouse state
     */
    switch (action)
    {
        case HID_GENERIC_MOUSE_X:
            m_mouse_state.acc_x += param;
            break;
        case HID_GENERIC_MOUSE_Y:
            m_mouse_state.acc_y += param;
            break;
        case HID_GENERIC_MOUSE_BTN_RIGHT:
            if(param == 1)
            {
                m_mouse_state.btn |= HID_BTN_RIGHT_MASK;
            }
            else
            {
                m_mouse_state.btn &= ~HID_BTN_RIGHT_MASK;
            }
            break;
        case HID_GENERIC_MOUSE_BTN_LEFT:
            if(param == 1)
            {
                m_mouse_state.btn |= HID_BTN_LEFT_MASK;
            }
            else
            {
                m_mouse_state.btn &= ~HID_BTN_LEFT_MASK;
            }
            break;
    }
    CRITICAL_REGION_EXIT();
}
#endif

#if (ENABLE_WEBUSB == 0)
typedef uint32_t (*response_callback_t)(void);
/**
 * @brief process response and set the IN report to send
 *
 */
void response_process(uint8_t *response, uint16_t response_size, response_callback_t *callback)
{
	ret_code_t ret;
	
	static uint8_t report_secux[HID_REPORT_LENGTH] = {0};
	NRF_LOG_INFO("response_size:%d", response_size);
	for (int i=0; i<CEIL_DIV(response_size, HID_REPORT_LENGTH); i++)
	{
		memset(report_secux, 0, HID_REPORT_LENGTH);
		memcpy(report_secux, response + (i*HID_REPORT_LENGTH), HID_REPORT_LENGTH);
		
		ret = app_usbd_hid_generic_in_report_set(
			&m_app_hid_generic,
			report_secux,
			HID_REPORT_LENGTH);
		
		nrf_delay_ms(100); //for test
	}
	
	if (ret != NRF_SUCCESS)
	{
		NRF_LOG_INFO("%s failed", __FUNCTION__);
	}
    
    if (NULL != *callback)
    {
        (*callback)();
    }
}
#endif
/**
 * @brief Class specific event handler.
 *
 * @param p_inst    Class instance.
 * @param event     Class specific event.
 * */
static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event)
{
    switch (event)
    {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
        {
					NRF_LOG_INFO("OUT_REPORT_READY, %d: %s", __LINE__, __FUNCTION__);
						uint8_t const * p_rep_buffer = NULL;
						size_t buffer_size = 0;
					  p_rep_buffer = app_usbd_hid_generic_out_report_get(&m_app_hid_generic, &buffer_size);
						/* 
						// print out buffer
						NRF_LOG_INFO("print buffer, size:%d", buffer_size);
						for (int i=0; i<buffer_size; i+=4)
						{
							NRF_LOG_INFO("buffer[%d]:%.02x %.02x %.02x %.02x", i, 
								p_rep_buffer[i], p_rep_buffer[i+1], p_rep_buffer[i+2], p_rep_buffer[i+3]);
						}
            */
						if (g_apdu_command_received == APDU_COMMAND_QUEUE_SIZE)
						{
							NRF_LOG_INFO("received command over buffer size");
							break;
						}
						else
						{
							//NRF_LOG_INFO("test123, %d" , __LINE__);
							memcpy(g_apdu_command_buffer[g_apdu_command_received++], p_rep_buffer, buffer_size);
							NRF_LOG_INFO("received command: index:%d", g_apdu_command_received-1);
							NRF_LOG_HEXDUMP_INFO(&g_apdu_command_buffer[g_apdu_command_received - 1], buffer_size);
						}
						
            break;
        }
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
        {
					NRF_LOG_INFO("IN_REPORT_DONE, %d: %s", __LINE__, __FUNCTION__);
            //m_report_pending = false;
            //hid_generic_mouse_process_state();
            //bsp_board_led_invert(LED_HID_REP_IN);
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
        {
            NRF_LOG_INFO("SET_BOOT_PROTO");
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
        {
            NRF_LOG_INFO("SET_REPORT_PROTO");
            break;
        }
        default:
            break;
    }
}

/**
 * @brief USBD library specific event handler.
 *
 * @param event     USBD library event.
 * */
static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SOF:
            break;
        case APP_USBD_EVT_DRV_RESET:
            m_report_pending = false;
            break;
        case APP_USBD_EVT_DRV_SUSPEND:
            m_report_pending = false;
            app_usbd_suspend_req(); // Allow the library to put the peripheral into sleep mode
            //bsp_board_leds_off();
            break;
        case APP_USBD_EVT_DRV_RESUME:
            m_report_pending = false;
            //bsp_board_led_on(LED_USB_START);
            break;
        case APP_USBD_EVT_STARTED:
            m_report_pending = false;
            //bsp_board_led_on(LED_USB_START);
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            //bsp_board_leds_off();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}
void usb_init()
{
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    app_usbd_class_inst_t const * class_inst_generic;
    class_inst_generic = app_usbd_hid_generic_class_inst_get(&m_app_hid_generic);
    ret = app_usbd_class_append(class_inst_generic);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");
		NRF_LOG_FLUSH();

        app_usbd_enable();
        app_usbd_start();
    }		
		NRF_LOG_FLUSH();
}

