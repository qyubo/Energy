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
    while (1) {};
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
  UNUSED(cdc_acm);
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
        while (1) {};
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
        while (1) {};
      }
      break;
    }

    /* Set the the control line state */
    case UX_SLAVE_CLASS_CDC_ACM_SET_CONTROL_LINE_STATE :
    default :
      break;
  }
}

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
        if (actual_length >= sizeof(msg_visionrx))
        {
          memcpy(&msg_visionrx, UserRxBufferFS, sizeof(msg_visionrx));
          memcpy(&debug_rx_vision, &msg_visionrx, sizeof(msg_visionrx));
        }
      }
    }
    om_publish(visionrx_topic, &msg_visionrx, sizeof(msg_visionrx), true, false);
    tx_thread_sleep(2);
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
    /* Get the device */
    device = &_ux_system_slave->ux_system_slave_device;

    /* Get the data interface */
    data_interface = device->ux_slave_device_first_interface->ux_slave_interface_next_interface;

    /* Get the cdc Instance */
    cdc_acm = data_interface->ux_slave_interface_class_instance;

    cdc_acm -> ux_slave_class_cdc_acm_transmission_status = UX_FALSE;

    om_suber_export(ins_suber, &ins, false);
    msg_visiontx.header = 0x5A;
    msg_visiontx.detect_color = 0x00;
    msg_visiontx.reset_tracker = false;
    msg_visiontx.set_target = 0x00;
    msg_visiontx.q1 = ins.quaternion[0];
    msg_visiontx.q2 = ins.quaternion[1];
    msg_visiontx.q3 = ins.quaternion[2];
    msg_visiontx.q4 = ins.quaternion[3];
    msg_visiontx.gyro_yaw = ins.gyro_y;
    msg_visiontx.gyro_pitch = ins.gyro_p;
    Append_CRC16_Check_Sum((uint8_t *)&msg_visiontx, sizeof(msg_visiontx));
    memcpy(&debug_tx_vision, &msg_visiontx, sizeof(msg_visiontx));

    ux_device_class_cdc_acm_write(cdc_acm, (UCHAR *)&msg_visiontx, sizeof(msg_visiontx), &actual_length);

    tx_thread_sleep(5);
  }
}
/* USER CODE END 0 */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
