import win32com.client


def get_usb_device():
    try:
        usb_list = []
        wmi = win32com.client.GetObject("winmgmts:")
        for usb in wmi.InstancesOf("Win32_USBHub"):
            print(usb.DeviceID)
            print(usb.description)
            usb_list.append(usb.description)

        print(usb_list)
        return usb_list
    except Exception as error:
        print('error', error)


if __name__ == '__main__':
    def main():
        get_usb_device()

    main()
