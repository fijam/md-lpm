#!/usr/bin/python3

from pymcuprog.backend import SessionConfig
from pymcuprog.toolconnection import ToolSerialConnection
from pymcuprog.backend import Backend
import argparse


def read_user_row_byte():
    byte = backend.read_memory(memory_name='user_row', numbytes=1, offset_byte=0)
    return int.from_bytes(byte[0].data, byteorder="little")


def read_eeprom_offset(offset):
    byte = backend.read_memory(memory_name='eeprom', numbytes=2, offset_byte=offset)
    return int.from_bytes(byte[0].data, byteorder="little")

def write_eeprom_offset(offset, data):
    backend.write_memory(memory_name='eeprom', offset_byte=offset, data=data)


def value_to_readout(val, level):
    if val == 65535:
        return "Nothing saved yet"
    if level == 3:
        return f'{(val*0.0054)+0.0867:.2f} mW'  # not very precise
    return f'{val*0.0022:.2f} mW'


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--serial-port', dest='serial_port', action='store',
                        help='USB-serial adapter port (e.g. COM5)')
    parser.add_argument('--cal-factor', dest='cal_factor', action='store',
                        help='override device-specific calibration (e.g. 95)')
    parser.add_argument('--clear', dest='clear', action='store_true',
                        help='clear the memory after reading')
    return parser.parse_args()


parser = parse_arguments()
if parser.serial_port is None:
    parser.serial_port = input('USB-serial adapter port (e.g. COM5): ')

# Configure session
sessionconfig = SessionConfig('attiny212')
# Instantiate transport & backend
transport = ToolSerialConnection(serialport=parser.serial_port)
backend = Backend()
# Connect to tool using transport
print("Connecting...")
backend.connect_to_tool(transport)
backend.start_session(sessionconfig)

if parser.cal_factor is None:
    cal_factor = read_user_row_byte()
else:
    cal_factor = parser.cal_factor

print("Saved sensor readings:")

val_writ = read_eeprom_offset(0)
val_href = read_eeprom_offset(16)
val_lref = read_eeprom_offset(32)
val_batt = read_eeprom_offset(48)

batt_volts = (1.5 * 1023) / val_batt

print(f"  WritPw: {(value_to_readout(val_writ,3))}")
print(f"  HrefPw: {(value_to_readout(val_href,2))}")
print(f"  LrefPw: {(value_to_readout(val_lref,1))}")
print(f"Calibration factor: {cal_factor}%")
print(f"Battery level: {'good' if batt_volts>=2.84 else 'low'} ({batt_volts:.3f}V)")

if parser.clear:
    write_eeprom_offset(0, [0xFF, 0xFF])
    write_eeprom_offset(16, [0xFF, 0xFF])
    write_eeprom_offset(32, [0xFF, 0xFF])
    print()
    print("Memory cleared")

input('\n Press any key to exit.')

backend.end_session()
backend.disconnect_from_tool()
