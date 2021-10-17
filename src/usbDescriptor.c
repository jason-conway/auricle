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

struct usb_string_descriptor_struct usb_string_product_name = {16, 3, {'a', 'u', 'r', 'i', 'c', 'l', 'e'}};
struct usb_string_descriptor_struct usb_string_manufacturer_name = {26, 3, {'J', 'A', 'S', 'O', 'N', ' ', 'C', 'O', 'N', 'W', 'A', 'Y'}};
