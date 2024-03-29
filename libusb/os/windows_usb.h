/*
 * Windows backend for libusb 1.0
 * Copyright (C) 2009 Pete Batard <pbatard@gmail.com>
 * Parts of this code adapted from libusb-win32-v1 by Stephan Meyer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

// Windows API default is uppercase - ugh!
#if !defined(bool)
#define bool BOOLEAN
#endif
#if !defined(true)
#define true TRUE
#endif
#if !defined(false)
#define false FALSE
#endif

#if !defined(libusb_bus_t)
#define libusb_bus_t uint8_t
#define LIBUSB_BUS_MAX UINT8_MAX
#endif
#if !defined(libusb_devaddr_t)
#define libusb_devaddr_t uint8_t
#define LIBUSB_DEVADDR_MAX UINT8_MAX
#endif

#define safe_free(p) do {if (p != NULL) {free(p); p = NULL;}} while(0)
#define safe_closehandle(h) do {if (h != INVALID_HANDLE_VALUE) {CloseHandle(h); h = INVALID_HANDLE_VALUE;}} while(0)
#define safe_strncpy(dst, dst_max, src, count) strncpy(dst, src, min(count, dst_max - 1))
#define safe_strcpy(dst, dst_max, src) safe_strncpy(dst, dst_max, src, strlen(src)+1)
#define safe_strncat(dst, dst_max, src, count) strncat(dst, src, min(count, dst_max - strlen(dst) - 1))
#define safe_strcat(dst, dst_max, src) safe_strncat(dst, dst_max, src, strlen(src)+1)
#define safe_strcmp(str1, str2) strcmp(((str1==NULL)?"<NULL>":str1), ((str2==NULL)?"<NULL>":str2))
#define safe_strncmp(str1, str2, count) strncmp(((str1==NULL)?"<NULL>":str1), ((str2==NULL)?"<NULL>":str2), count)
#define safe_strlen(str) ((str==NULL)?0:strlen(str))
#define safe_strdup _strdup
#define safe_sprintf _snprintf
#define safe_unref_device(dev) do {if (dev != NULL) {libusb_unref_device(dev); dev = NULL;}} while(0)
void inline upperize(char* str) {
	size_t i;
	if (str == NULL) return;
	for (i=0; i<strlen(str); i++)
		str[i] = toupper(str[i]);
}

#define MAX_CTRL_BUFFER_LENGTH      4096
#define MAX_USB_DEVICES             256

#define MAX_PATH_LENGTH             128
#define MAX_KEY_LENGTH              256
#define ERR_BUFFER_SIZE             256
#define GUID_STRING_LENGTH          40

#define wchar_to_utf8_ms(wstr, str, strlen) WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, strlen, NULL, NULL)
#define ERRNO GetLastError()

// This is used to support multiple kernel drivers in Windows.
struct windows_driver_backend {
	const char *name;	// A human-readable name for your backend, e.g. "WinUSB"
	int (*open)(struct libusb_device_handle *dev_handle);
	int (*claim_interface)(struct libusb_device_handle *dev_handle, int iface);
	int (*set_interface_altsetting)(struct libusb_device_handle *dev_handle, int iface, int altsetting);
	int (*release_interface)(struct libusb_device_handle *dev_handle, int iface);
	int (*clear_halt)(struct libusb_device_handle *dev_handle, unsigned char endpoint);
	int (*reset_device)(struct libusb_device_handle *dev_handle);
	int (*submit_bulk_transfer)(struct usbi_transfer *itransfer);
	int (*submit_iso_transfer)(struct usbi_transfer *itransfer);
	int (*submit_control_transfer)(struct usbi_transfer *itransfer);
	int (*abort_control)(struct usbi_transfer *itransfer);
	int (*abort_transfers)(struct usbi_transfer *itransfer);
};
extern const struct windows_driver_backend windows_template_backend;
extern const struct windows_driver_backend windows_winusb_backend;

#define PRINT_UNSUPPORTED_API(fname)        \
	usbi_dbg("unsupported API call for '"   \
		#fname "'");                        \
	return LIBUSB_ERROR_NOT_SUPPORTED;

enum windows_version {
	WINDOWS_UNSUPPORTED,
	WINDOWS_XP,
	WINDOWS_VISTA_AND_LATER,
};

/*
 * private structures definition
 * with inline pseudo constructors/destructors
 */

// HCDs
struct windows_hcd_priv {
	char *path;
	struct windows_hcd_priv *next;
};

static inline void windows_hcd_priv_init(struct windows_hcd_priv* p) {
	p->path = NULL;
	p->next = NULL;
}

static inline void windows_hcd_priv_release(struct windows_hcd_priv* p) {
	safe_free(p->path);
}


// Nodes (Hubs & devices)
struct windows_device_priv {
	struct libusb_device *parent_dev;   // access to parent is required for usermode ops
	ULONG connection_index;             // also required for some usermode ops
	char *path;                         // path used by Windows to reference the USB node
	struct {
		char *path;                     // each interface has a path as well
		int8_t nb_endpoints;            // and a set of endpoint addresses (USB_MAXENDPOINTS)
		uint8_t *endpoint;	            
	} interface[USB_MAXINTERFACES];
	char *driver;                       // driver name (eg WinUSB, USBSTOR, HidUsb, etc)
	struct windows_driver_backend const *apib;
	uint8_t active_config;
	USB_DEVICE_DESCRIPTOR dev_descriptor;
	unsigned char **config_descriptor;  // list of pointers to the cached config descriptors
};

static inline void windows_device_priv_init(struct windows_device_priv* p) {
	int i;
	p->parent_dev = NULL;
	p->connection_index = 0;
	p->path = NULL;
	p->driver = NULL;
	p->apib = &windows_template_backend;
	p->active_config = 0;
	p->config_descriptor = NULL;
	memset(&(p->dev_descriptor), 0, sizeof(USB_DEVICE_DESCRIPTOR));
	for (i=0; i<USB_MAXINTERFACES; i++) {
		p->interface[i].path = NULL;
		p->interface[i].nb_endpoints = 0;
		p->interface[i].endpoint = NULL;
	}
}

static inline void windows_device_priv_release(struct windows_device_priv* p, int num_configurations) {
	int i;
	safe_free(p->path);
	safe_free(p->driver);
	if ((num_configurations > 0) && (p->config_descriptor != NULL)) {
		for (i=0; i < num_configurations; i++)
			safe_free(p->config_descriptor[i]);
	}
	safe_free(p->config_descriptor);
	for (i=0; i<USB_MAXINTERFACES; i++) {
		safe_free(p->interface[i].path);
		safe_free(p->interface[i].endpoint);
	}
}

static inline struct windows_device_priv *__device_priv(struct libusb_device *dev) {
	return (struct windows_device_priv *)dev->os_priv;
}

struct winusb_handles {
	HANDLE file;
	HANDLE winusb;
};

struct windows_device_handle_priv {
	int active_interface;
	struct winusb_handles interface_handle[USB_MAXINTERFACES];
};

static inline struct windows_device_handle_priv *__device_handle_priv(
	struct libusb_device_handle *handle)
{
	return (struct windows_device_handle_priv *) handle->os_priv;
}

// used for async polling functions
struct windows_transfer_priv {
	struct winfd pollable_fd;
};


/*
 * Windows API structures (redefined for convenience)
 */

// Fixed length version of USB_ROOT_HUB_NAME & USB_NODE_CONNECTION_NAME
typedef struct _USB_ROOT_HUB_NAME_FIXED {
	ULONG ActualLength;
	WCHAR RootHubName[MAX_PATH_LENGTH];
} USB_ROOT_HUB_NAME_FIXED;

typedef struct _USB_NODE_CONNECTION_NAME_FIXED {
	ULONG ConnectionIndex;
	ULONG ActualLength;
	WCHAR NodeName[MAX_PATH_LENGTH];
} USB_NODE_CONNECTION_NAME_FIXED;

typedef struct _USB_HUB_NAME_FIXED {
	union {
		USB_ROOT_HUB_NAME_FIXED root;
		USB_NODE_CONNECTION_NAME_FIXED node;
	} u;
} USB_HUB_NAME_FIXED;

// The following structure needs to be packed
// NB: can't reuse structs containing a zero element arrays
// (eg. struct with a 'Data[0]'), as MSVC6 can't handle it.
#pragma pack(push, 1)
typedef struct _USB_CONFIGURATION_DESCRIPTOR_SHORT {
	struct {
		ULONG ConnectionIndex;
		struct {
			UCHAR bmRequest;
			UCHAR bRequest;
			USHORT wValue;
			USHORT wIndex;
			USHORT wLength;
		} SetupPacket;
	} req;
	USB_CONFIGURATION_DESCRIPTOR data;
} USB_CONFIGURATION_DESCRIPTOR_SHORT;
#pragma pack(pop)

#if !defined(_MSC_VER)
typedef union _USB_HUB_CAP_FLAGS {
	ULONG ul;
	struct {
		ULONG HubIsHighSpeedCapable:1;
		ULONG HubIsHighSpeed:1;
		ULONG HubIsMultiTtCapable:1;
		ULONG HubIsMultiTt:1;
		ULONG HubIsRoot:1;
		ULONG HubIsArmedWakeOnConnect:1;
		ULONG ReservedMBZ:26;
	};
} USB_HUB_CAP_FLAGS, *PUSB_HUB_CAP_FLAGS;

typedef struct _USB_HUB_CAPABILITIES_EX {
	USB_HUB_CAP_FLAGS CapabilityFlags;
} USB_HUB_CAPABILITIES_EX, *PUSB_HUB_CAPABILITIES_EX;
#endif

#if !defined(USB_GET_HUB_CAPABILITIES_EX)
#define USB_GET_HUB_CAPABILITIES_EX 276
#endif

#if !defined(IOCTL_USB_GET_HUB_CAPABILITIES_EX)
#define IOCTL_USB_GET_HUB_CAPABILITIES_EX \
	CTL_CODE( FILE_DEVICE_USB, USB_GET_HUB_CAPABILITIES_EX, \
	METHOD_BUFFERED, FILE_ANY_ACCESS )
#endif

/*
 * WinUSB macros - from libusb-win32 1.x
 */
#pragma once

#define DLL_DECLARE(api, ret, name, args)                    \
  typedef ret (api * __dll_##name##_t)args; __dll_##name##_t name

#define DLL_LOAD(dll, name, ret_on_failure)                   \
  do {                                                        \
  HMODULE h = GetModuleHandle(#dll);                          \
  if(!h)                                                      \
    h = LoadLibrary(#dll);                                    \
  if(!h) {                                                    \
    if(ret_on_failure)                                        \
      return LIBUSB_ERROR_OTHER;                             \
    else break; }                                             \
  if((name = (__dll_##name##_t)GetProcAddress(h, #name)))     \
    break;                                                    \
  if((name = (__dll_##name##_t)GetProcAddress(h, #name "A"))) \
    break;                                                    \
  if((name = (__dll_##name##_t)GetProcAddress(h, #name "W"))) \
    break;                                                    \
  if(ret_on_failure)                                          \
    return LIBUSB_ERROR_OTHER;                               \
  } while(0)


/* winusb.dll interface */

#define SHORT_PACKET_TERMINATE  0x01
#define AUTO_CLEAR_STALL        0x02
#define PIPE_TRANSFER_TIMEOUT   0x03
#define IGNORE_SHORT_PACKETS    0x04
#define ALLOW_PARTIAL_READS     0x05
#define AUTO_FLUSH              0x06
#define RAW_IO                  0x07
#define MAXIMUM_TRANSFER_SIZE   0x08
#define AUTO_SUSPEND            0x81
#define SUSPEND_DELAY           0x83
#define DEVICE_SPEED            0x01
#define LowSpeed                0x01
#define FullSpeed               0x02
#define HighSpeed               0x03 

typedef enum _USBD_PIPE_TYPE {
	UsbdPipeTypeControl,
	UsbdPipeTypeIsochronous,
	UsbdPipeTypeBulk,
	UsbdPipeTypeInterrupt
} USBD_PIPE_TYPE;

typedef struct {
  USBD_PIPE_TYPE PipeType;
  UCHAR          PipeId;
  USHORT         MaximumPacketSize;
  UCHAR          Interval;
} WINUSB_PIPE_INFORMATION, *PWINUSB_PIPE_INFORMATION;

#pragma pack(1)
typedef struct {
  UCHAR  request_type;
  UCHAR  request;
  USHORT value;
  USHORT index;
  USHORT length;
} WINUSB_SETUP_PACKET, *PWINUSB_SETUP_PACKET;
#pragma pack()

typedef void *WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

DLL_DECLARE(WINAPI, BOOL, WinUsb_Initialize, 
            (HANDLE, PWINUSB_INTERFACE_HANDLE));
DLL_DECLARE(WINAPI, BOOL, WinUsb_Free, (WINUSB_INTERFACE_HANDLE));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetAssociatedInterface, 
            (WINUSB_INTERFACE_HANDLE, UCHAR, PWINUSB_INTERFACE_HANDLE));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetDescriptor, 
            (WINUSB_INTERFACE_HANDLE, UCHAR, UCHAR, USHORT, PUCHAR,
             ULONG, PULONG));
DLL_DECLARE(WINAPI, BOOL, WinUsb_QueryInterfaceSettings,
            (WINUSB_INTERFACE_HANDLE, UCHAR, PUSB_INTERFACE_DESCRIPTOR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_QueryDeviceInformation, 
            (WINUSB_INTERFACE_HANDLE, ULONG, PULONG, PVOID));
DLL_DECLARE(WINAPI, BOOL, WinUsb_SetCurrentAlternateSetting, 
            (WINUSB_INTERFACE_HANDLE, UCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetCurrentAlternateSetting, 
            (WINUSB_INTERFACE_HANDLE, PUCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_QueryPipe, 
            (WINUSB_INTERFACE_HANDLE, UCHAR, UCHAR,
             PWINUSB_PIPE_INFORMATION));
DLL_DECLARE(WINAPI, BOOL, WinUsb_SetPipePolicy, 
            (WINUSB_INTERFACE_HANDLE, UCHAR, ULONG, ULONG, PVOID));
DLL_DECLARE(WINAPI, BOOL, WinUsb_GetPipePolicy, 
            (WINUSB_INTERFACE_HANDLE, UCHAR, ULONG, PULONG, PVOID));
DLL_DECLARE(WINAPI, BOOL, WinUsb_ReadPipe, 
            (WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG,
             LPOVERLAPPED));
DLL_DECLARE(WINAPI, BOOL, WinUsb_WritePipe,
            (WINUSB_INTERFACE_HANDLE, UCHAR, PUCHAR, ULONG, PULONG, 
             LPOVERLAPPED));
DLL_DECLARE(WINAPI, BOOL, WinUsb_ControlTransfer,
            (WINUSB_INTERFACE_HANDLE, WINUSB_SETUP_PACKET, PUCHAR, ULONG, 
             PULONG, LPOVERLAPPED));
DLL_DECLARE(WINAPI, BOOL, WinUsb_ResetPipe, 
            (WINUSB_INTERFACE_HANDLE, UCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_AbortPipe, 
            (WINUSB_INTERFACE_HANDLE, UCHAR));
DLL_DECLARE(WINAPI, BOOL, WinUsb_FlushPipe, 
            (WINUSB_INTERFACE_HANDLE, UCHAR));
