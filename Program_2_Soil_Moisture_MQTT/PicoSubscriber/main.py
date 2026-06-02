from machine import Pin, I2C
import network
import time
import ssd1306
from umqtt.simple import MQTTClient

SSID = "YOUR_WIFI"
PASSWORD = "YOUR_PASSWORD"

BROKER = "192.168.1.100"
TOPIC = b"plant/soil"

# OLED
i2c = I2C(
    0,
    scl=Pin(5),
    sda=Pin(4)
)

oled = ssd1306.SSD1306_I2C(
    128,
    64,
    i2c
)

current = 0
minimum = 100
maximum = 0

total = 0
count = 0

try:
    open("soil.csv","r").close()
except:
    with open("soil.csv","w") as f:
        f.write("timestamp,soil\n")

try:
    open("dashboard.csv","r").close()
except:
    with open("dashboard.csv","w") as f:
        f.write("timestamp,soil\n")

wlan = network.WLAN(network.STA_IF)
wlan.active(True)

wlan.connect(SSID,PASSWORD)

while not wlan.isconnected():
    time.sleep(1)

print("WiFi Connected")

def updateOLED():

    avg = total/count if count>0 else 0

    oled.fill(0)

    oled.text("Soil Moisture",0,0)

    oled.text(
        "Cur:{}%".format(current),
        0,
        15)

    oled.text(
        "Min:{}".format(minimum),
        0,
        30)

    oled.text(
        "Max:{}".format(maximum),
        65,
        30)

    oled.text(
        "Avg:{:.1f}".format(avg),
        0,
        50)

    oled.show()

def saveCSV(value):

    t = time.time()

    with open("soil.csv","a") as f:
        f.write(
            "{},{}\n".format(
                t,
                value))

    with open("dashboard.csv","a") as f:
        f.write(
            "{},{}\n".format(
                t,
                value))

def callback(topic,msg):

    global current
    global minimum
    global maximum
    global total
    global count

    current = int(msg.decode())

    if current < minimum:
        minimum = current

    if current > maximum:
        maximum = current

    total += current
    count += 1

    saveCSV(current)

    updateOLED()

    print("----------------")
    print("Current:",current)
    print("Min:",minimum)
    print("Max:",maximum)
    print("Average:",total/count)

client = MQTTClient(
    "PicoSubscriber",
    BROKER)

client.set_callback(callback)

client.connect()

client.subscribe(TOPIC)

updateOLED()

while True:

    try:
        client.check_msg()
    except:
        pass

    time.sleep(0.1)