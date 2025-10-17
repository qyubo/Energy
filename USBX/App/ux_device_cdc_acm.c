/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_cdc_acm.c
  * @author  MCD Application Team
  * @brief   USBX Device applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ux_device_cdc_acm.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "app_usbx_device.h"

// user headers
#include "om.h"
#include "crc.hpp"
#include "magicmsgs.hpp"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_RX_DATA_SIZE                          2048
#define APP_TX_DATA_SIZE                          2048

/* Rx/TX flag */
#define RX_NEW_RECEIVED_DATA                      0x01
#define TX_NEW_TRANSMITTED_DATA                   0x02

/* Data length for vcp */
#define VCP_WORDLENGTH8                           8
#define VCP_WORDLENGTH9                           9

/* the minimum baudrate */
#define MIN_BAUDRATE                              9600
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* Data received over uart are stored in this buffer */
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/* Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/* Increment this pointer or roll it back to
start address when data are received over USART */
uint32_t UserTxBufPtrIn;

/* Increment this pointer or roll it back to
start address when data are sent over USB */
uint32_t UserTxBufPtrOut;
UX_SLAVE_CLASS_CDC_ACM  *cdc_acm;
UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARAMETER CDC_VCP_LineCoding =
{
  115200, /* baud rate */
  0x00,   /* stop bits-1 */
  0x00,   /* parity - none */
  0x08    /* nb. of bits 8 */
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @param  cdc Instance
  * @retval none
  */
void CDC_Init_FS(void *cdc_acm)
{
  /* Status */
  UINT ux_status = UX_SUCCESS;

  /* USER CODE BEGIN 3 */

  /* Set device_class_cdc_acm with default parameters */
  ux_status = ux_device_class_cdc_acm_ioctl(cdc_acm,
                                            UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
                                            &CDC_VCP_LineCoding);
  /* Check Status */
  if (ux_status != UX_SUCCESS)
  {
    while(1){}
  }

  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
void CDC_DeInit_FS(void *cdc_acm)
{
  /* USER CODE BEGIN 4 */
  // HAL_UART_DeInit(&huart3);
  /* USER CODE END 4 */
}

/**
  * @brief  Manage the CDC class requests
  * @param  cdc Instance
  * @retval none
  */
VOID ux_app_parameters_change(VOID *cdc_acm)
{
  UX_SLAVE_TRANSFER *transfer_request;
  UX_SLAVE_DEVICE   *device;
  ULONG             request;
  UINT ux_status = UX_SUCCESS;

  /* Get the pointer to the device.  */
  device = &_ux_system_slave -> ux_system_slave_device;

  /* Get the pointer to the transfer request associated with the control endpoint. */
  transfer_request = &device -> ux_slave_device_control_endpoint.
                     ux_slave_endpoint_transfer_request;

  /* Extract all necessary fields of the request. */
  request = *(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_REQUEST);

  /* Here we proceed only the standard request we know of at the device level.  */
  switch (request)
  {
    /* Set Line Coding Command */
    case UX_SLAVE_CLASS_CDC_ACM_SET_LINE_CODING :
    {
      /* Get the Line Coding parameters */
      ux_status = ux_device_class_cdc_acm_ioctl(cdc_acm,
                                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING,
                                                &CDC_VCP_LineCoding);
      /* Check Status */
      if (ux_status != UX_SUCCESS)
      {
        while(1){}
      }

      /* Check if baudrate < 9600) then set it to 9600 */
      if (CDC_VCP_LineCoding.ux_slave_class_cdc_acm_parameter_baudrate < MIN_BAUDRATE)
      {
        CDC_VCP_LineCoding.ux_slave_class_cdc_acm_parameter_baudrate = MIN_BAUDRATE;
      }
      break;
    }

    /* Get Line Coding Command */
    case UX_SLAVE_CLASS_CDC_ACM_GET_LINE_CODING :
    {
      ux_status = ux_device_class_cdc_acm_ioctl(cdc_acm,
                                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
                                                &CDC_VCP_LineCoding);

      /* Check Status */
      if (ux_status != UX_SUCCESS)
      {
        while(1){}
      }
      break;
    }

    /* Set the the control line state */
    case UX_SLAVE_CLASS_CDC_ACM_SET_CONTROL_LINE_STATE :
    default :
      break;
  }
}

uint32_t new_data_ = 0;
uint32_t debug_length = 0;
struct msg_visionrx_t debug_rx_vision;
struct msg_visiontx_t debug_tx_vision;
/**
  * @brief  Function implementing usbx_cdc_acm_thread_entry.
  * @param arg: Not used
  * @retval None
  */
void usbx_cdc_acm_read_thread_entry(ULONG arg)
{
  UX_SLAVE_DEVICE *device;
  UX_SLAVE_INTERFACE *data_interface;
  UX_SLAVE_CLASS_CDC_ACM *cdc_acm;
  ULONG actual_length;
  ULONG ux_status = UX_SUCCESS;
  ULONG senddataflag = 0;

  /* Get device */
  device = &_ux_system_slave->ux_system_slave_device;

  om_init();
  struct msg_visionrx_t msg_visionrx;
  om_topic_t *visionrx_topic = om_config_topic(NULL, "ca", "visionrx", sizeof(msg_visionrx));

  while (1)
  {

    /* Check if device is configured */
    if (device->ux_slave_device_state == UX_DEVICE_CONFIGURED)
    {
      /* Get Data interface */
      data_interface = device->ux_slave_device_first_interface->ux_slave_interface_next_interface;

      /* Compares two memory blocks ux_slave_class_name and _ux_system_slave_class_cdc_acm_name */
      ux_status = ux_utility_memory_compare(data_interface->ux_slave_interface_class->ux_slave_class_name,
                                            _ux_system_slave_class_cdc_acm_name,
                                            ux_utility_string_length_get(_ux_system_slave_class_cdc_acm_name));

      /* Check Compares success */
      if (ux_status == UX_SUCCESS)
      {
        cdc_acm =  data_interface->ux_slave_interface_class_instance;

        /* Set transmission_status to UX_FALSE for the first time */
        cdc_acm -> ux_slave_class_cdc_acm_transmission_status = UX_FALSE;

        /* Read the received data in blocking mode */
        ux_device_class_cdc_acm_read(cdc_acm, (UCHAR *)UserRxBufferFS, sizeof(msg_visionrx),
                                     &actual_length);
        new_data_ = actual_length;
        if (actual_length >= sizeof(msg_visionrx)) {
          memcpy(&msg_visionrx, UserRxBufferFS, sizeof(msg_visionrx));
          memcpy(&debug_rx_vision, &msg_visionrx, sizeof(msg_visionrx));
        }
        // tx_thread_sleep(1);
        // if (actual_length != 0)
        // {
        //   /* Wait until the requested flag TX_NEW_TRANSMITTED_DATA is received */
        //   if (tx_event_flags_get(&EventFlag, TX_NEW_TRANSMITTED_DATA, TX_OR_CLEAR,
        //                          &senddataflag, TX_WAIT_FOREVER) != TX_SUCCESS)
        //   {
        //     while(1){}
        //   }
        // }
      }
    }
    om_publish(visionrx_topic, &msg_visionrx, sizeof(msg_visionrx), true, false);
    tx_thread_sleep(1);
  }
}

/**
  * @brief  Function implementing usbx_cdc_acm_write_thread_entry.
  * @param arg: Not used
  * @retval None
  */
void usbx_cdc_acm_write_thread_entry(ULONG arg)
{
  UX_SLAVE_DEVICE    *device;
  UX_SLAVE_INTERFACE *data_interface;
  UX_SLAVE_CLASS_CDC_ACM *cdc_acm;
  ULONG actual_length;
  ULONG receivedataflag = 0;
  ULONG buffptr;
  ULONG buffsize;
  UINT ux_status = UX_SUCCESS;

  om_suber_t *ins_suber = om_subscribe(om_find_topic("ins", UINT32_MAX));
  struct msg_ins_t ins;
  struct msg_visiontx_t msg_visiontx;

  while (1)
  {
    // /* Wait until the requested flag RX_NEW_RECEIVED_DATA is received */
    // if (tx_event_flags_get(&EventFlag, RX_NEW_RECEIVED_DATA, TX_OR_CLEAR,
    //                        &receivedataflag, TX_WAIT_FOREVER) != TX_SUCCESS)
    // {
    //   while(1){}
    // }
    //
    // /* Get the device */
    device = &_ux_system_slave->ux_system_slave_device;

    /* Get the data interface */
    data_interface = device->ux_slave_device_first_interface->ux_slave_interface_next_interface;

    om_suber_export(ins_suber, &ins, false);
    msg_visiontx.header = 0x5A;
    msg_visiontx.detect_color = 0x00;
    msg_visiontx.reset_tracker = false;
    msg_visiontx.set_target = 0x00;
    msg_visiontx.q1 = ins.quaternion[0];
    msg_visiontx.q2 = ins.quaternion[1];
    msg_visiontx.q3 = ins.quaternion[2];
    msg_visiontx.q4 = ins.quaternion[3];
    Append_CRC16_Check_Sum((uint8_t *)&msg_visiontx, 20);
    memcpy(&debug_tx_vision, &msg_visiontx, sizeof(msg_visiontx));

    // /* Compares two memory blocks ux_slave_class_name and _ux_system_slave_class_cdc_acm_name */
    ux_status = ux_utility_memory_compare(data_interface->ux_slave_interface_class->ux_slave_class_name,
                                          _ux_system_slave_class_cdc_acm_name,
                                          ux_utility_string_length_get(_ux_system_slave_class_cdc_acm_name));
    if (ux_status == UX_SUCCESS)
    {
      cdc_acm =  data_interface->ux_slave_interface_class_instance;

      /* Set transmission_status to UX_FALSE for the first time */
      cdc_acm -> ux_slave_class_cdc_acm_transmission_status = UX_FALSE;
      if (new_data_) {
        // ux_device_class_cdc_acm_write(cdc_acm, UserRxBufferFS , new_data_ , &actual_length);
        debug_length = new_data_;
        new_data_ = 0;
        tx_thread_sleep(10);
      }
      else
      {
        debug_length = 0;
        uint8_t test_data_ = 0xA5;
        // ux_device_class_cdc_acm_write(cdc_acm, UserRxBufferFS , test_data_ , &actual_length);
        ux_device_class_cdc_acm_write(cdc_acm, (UCHAR *)&msg_visiontx, sizeof(msg_visiontx), &actual_length);
        debug_length = actual_length;
      }
    }

    tx_thread_sleep(1);

    /* Check if there is a new data to send */
    // if (UserTxBufPtrOut != UserTxBufPtrIn)
    // {
    //   /* Check buffer overflow and Rollback */
    //   if (UserTxBufPtrOut > UserTxBufPtrIn)
    //   {
    //     buffsize = APP_RX_DATA_SIZE - UserTxBufPtrOut;
    //   }
    //   else
    //   {
    //     /* Calculate data size */
    //     buffsize = UserTxBufPtrIn - UserTxBufPtrOut;
    //   }
    //
    //   /* Copy UserTxBufPtrOut in buffptr */
    //   buffptr = UserTxBufPtrOut;
    //
    //   /* Send data over the class cdc_acm_write */
    //   ux_status = ux_device_class_cdc_acm_write(cdc_acm,
    //                                             (UCHAR *)(&UserTxBufferFS[buffptr]),
    //                                             buffsize, &actual_length);
    //
    //   /* Check if dataset is correctly transmitted */
    //   if (ux_status == UX_SUCCESS)
    //   {
    //     /* Increment the UserTxBufPtrOut pointer */
    //     UserTxBufPtrOut += buffsize;
    //
    //     /* Rollback UserTxBufPtrOut if it equal to APP_TX_DATA_SIZE */
    //     if (UserTxBufPtrOut == APP_TX_DATA_SIZE)
    //     {
    //       UserTxBufPtrOut = 0;
    //     }
    //   }
    // }
  }
}
/* USER CODE END 0 */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
