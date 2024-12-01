
import json


with open("pytohn_scripts/devices.json") as f:
    JS = json.load(f)
maxLen = 0
devices = "const I2C_DISCOVERY_DEV i2c_discovery_devs[] = {"
devices_names = list()


def address_is_skip_2(addresses: list):
    broke = True
    for i, a in enumerate(addresses):
        if (a != addresses[0]+i*2):
            broke = False
            break
    return broke


def addDevice(addresses: list, part_number: list, addessMode: int):
    global devices, devices_names
    for p in part_number:
        devices += "\n{" + \
            f"\"{p}\", {addresses[0]}, {addresses[-1]}, {addessMode}"+"},"
        if (str(p).upper() in devices_names):
            print("AAAAAAAAAAAA", part_number)
        devices_names.append(str(p).upper())


JS.sort(key=lambda obj: (-len(obj['drivers']), not (
    'adafruit' in obj), not ('sparkfun' in obj)))
for dev in JS:
    if (not "is_3v" in dev or not "addresses" in dev):
        continue

    addresses = sorted(list(dev["addresses"]))
    part_numbers = str(dev["part_number"]).strip().replace(
        " ", "_").replace("-", "_").split("/")
    maxLen = max(max([len(i) for i in part_numbers]), maxLen)

    if (len(addresses) == 0):
        continue

    addessMode = 0
    if (addresses[-1]-addresses[0] == len(addresses)-1):
        pass
    elif (len(addresses) == 2):
        pass
    elif (address_is_skip_2(addresses)):
        addessMode = 1
    elif (addresses == [16, 17, 18, 19, 20, 21, 22, 23, 64, 65, 66, 67, 68, 69, 70, 71, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90]):
        addessMode = 2
    elif (addresses == [45, 83, 87]):
        addessMode = 3
    else:
        print("WRONG ADDRESS", part_numbers)

    addDevice(addresses, part_numbers, addessMode)

devices += "};"

print(devices)
print("class i2c_discovery_ENUM { enum{", ",".join(devices_names), "\n};};")
