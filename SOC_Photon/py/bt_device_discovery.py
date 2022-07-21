from bluetooth import *

print("performing inquiry...")

nearby_devices = discover_devices(lookup_names=True)

print("found ", len(nearby_devices), " devices")

for name, addr in nearby_devices:
    print(addr, name)
