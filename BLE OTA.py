import os
import asyncio
from bleak import BleakClient, BleakScanner
import struct
import sys
import time
from collections import deque

# Replace with your TinyS3's BLE address
BLE_ADDRESS = "F4:12:FA:9F:0C:7D"
SERVICE_UUID = "4e8cbb5e-bc0f-4aab-a6e8-55e662418bef"
CHARACTERISTIC_UUID = "513fcda9-f46d-4e41-ac4f-42b768495a85"

PART = 16000
# MTU = 1024
end = True
fileBytes = None
total = 0
time_deque = deque(maxlen=10)  # To store the last 10 elapsed times

def calculate_time_remaining(elapsed_times, bytes_remaining, chunk_size):
    """
    Calculate the estimated time remaining based on the average of the last 10 elapsed times.
    """
    if len(elapsed_times) == 0:
        return "Calculating..."

    average_time = sum(elapsed_times) / len(elapsed_times)
    estimated_time_remaining = (bytes_remaining / chunk_size) * average_time

    minutes, seconds = divmod(estimated_time_remaining, 60)
    return f"{int(minutes)} minutes and {int(seconds)} seconds remaining"

async def send_firmware(address, file_path):
    device = await BleakScanner.find_device_by_address(address, timeout=3.0)
    disconnected_event = asyncio.Event()

    if not device:
        print(f"Device with address {address} could not be found.")
        return

    def handle_disconnect(_: BleakClient):
        global end
        end = False
        print("Device disconnected")
        disconnected_event.set()

    async def send_data(client: BleakClient, data: bytearray, response: bool):
        start_time = time.time()
        await client.write_gatt_char(CHARACTERISTIC_UUID, data, response)
        end_time = time.time()
        elapsed_time = end_time - start_time
        time_deque.append(elapsed_time)  # Store the elapsed time
        return elapsed_time

    try:
        async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
            # await asyncio.sleep(0.01)
            
            # Send the size of the file first
            file_size = os.path.getsize(file_path)
            await client.write_gatt_char(CHARACTERISTIC_UUID, struct.pack("<I", file_size))  # Send as a 4-byte integer
            print(f"Sent file size: {file_size} bytes")

            # Calculate total packets
            chunk_size = 517 * 1  # Define the chunk size MTU 1024
            total_packets = (file_size + chunk_size - 1) // chunk_size
            packet_number = 0

            # Read the file and send in chunks
            with open(file_path, 'rb') as f:
                total_sent = 0
                while chunk := f.read(chunk_size):
                    packet_number += 1
                    elapsed_time = await send_data(client, chunk, response=True)
                    total_sent += len(chunk)
                    percentage = (total_sent / file_size) * 100
                    bytes_remaining = file_size - total_sent
                    time_remaining = calculate_time_remaining(time_deque, bytes_remaining, chunk_size)
                    print(f"Packet {packet_number}/{total_packets}: Sent {total_sent}/{file_size} bytes ({percentage:.2f}%) in {elapsed_time:.4f} seconds. {time_remaining}")

            print("All data sent, waiting for the device to disconnect...")
            # Wait for the disconnect event
            await disconnected_event.wait()

    except OSError as e:
        print(f"An error occurred: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")

async def scan_for_devices():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    if devices:
        print("Found the following devices:")
        for device in devices:
            print(f"Device: {device.name}, Address: {device.address}")
    else:
        print("No BLE devices found.")

def main():
    # if len(sys.argv) == 1:
    #     print("No arguments provided. Scanning for devices instead.")
    #     asyncio.run(scan_for_devices())
    #     return
    
    # if len(sys.argv) != 3:
    #     print("Usage: python script.py <BLE_ADDRESS> <FIRMWARE_PATH>")
    #     sys.exit(1)

    # address = sys.argv[1]
    # firmware_path = sys.argv[2]

    address = "F4:12:FA:9F:0C:7D"
    firmware_path = r".pio\build\um_nanos3\firmware.bin"

    if not os.path.exists(firmware_path):
        print(f"File not found: {firmware_path}")
        sys.exit(1)

    asyncio.run(send_firmware(address, firmware_path))

if __name__ == "__main__":
    main()
