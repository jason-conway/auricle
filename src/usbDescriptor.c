/**
 * @file usbDescriptor.c
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Configure USB descriptors
 * @version 0.1
 * @date 2021-08-10
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "usb_names.h"

#define AURICLE_MANUFACTURER_NAME {'J', 'A', 'S', 'O', 'N', ' ', 'C', 'O', 'N', 'W', 'A', 'Y'}
#define AURICLE_MANUFACTURER_NAME_LEN 12

#define AURICLE_PRODUCT_NAME {'a', 'u', 'r', 'i', 'c', 'l', 'e'}
#define AURICLE_PRODUCT_NAME_LEN 7

struct usb_string_descriptor_struct usb_string_product_name =
	{
		2 + AURICLE_PRODUCT_NAME_LEN * 2,
		3,
		AURICLE_PRODUCT_NAME
	};

struct usb_string_descriptor_struct usb_string_manufacturer_name =
	{
		2 + AURICLE_MANUFACTURER_NAME_LEN * 2,
		3,
		AURICLE_MANUFACTURER_NAME
	};
