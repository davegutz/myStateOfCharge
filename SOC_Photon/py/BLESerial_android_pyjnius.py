'''
Bluetooth/Pyjnius example
=========================

This was used to send some bytes to an arduino via bluetooth.
The app must have BLUETOOTH and BLUETOOTH_ADMIN permissions (well, i didn't
tested without BLUETOOTH_ADMIN, maybe it works.)

Connect your device to your phone, via the bluetooth menu. After the
pairing is done, you'll be able to use it in the app.

on windows 10 installed JDK 11.   A stack overflow article said >=12 not compatible
JAVA_HOME = C:\Program Files\Java\jdk-11.0.15

'''

from jnius import autoclass

BluetoothAdapter = autoclass('android.bluetooth.BluetoothAdapter')
BluetoothDevice = autoclass('android.bluetooth.BluetoothDevice')
BluetoothSocket = autoclass('android.bluetooth.BluetoothSocket')
UUID = autoclass('java.util.UUID')

def get_socket_stream(name):
    paired_devices = BluetoothAdapter.getDefaultAdapter().getBondedDevices().toArray()
    socket = None
    for device in paired_devices:
        if device.getName() == name:
            socket = device.createRfcommSocketToServiceRecord(
                UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"))
            recv_stream = socket.getInputStream()
            send_stream = socket.getOutputStream()
            break
    socket.connect()
    return recv_stream, send_stream

if __name__ == '__main__':
    recv_stream, send_stream = get_socket_stream('linvor')
    send_stream.write('hello\n')
    send_stream.flush()

