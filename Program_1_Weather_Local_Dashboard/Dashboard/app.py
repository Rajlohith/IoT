from flask import Flask, jsonify, request, send_from_directory
from flask import render_template
from collections import deque
from datetime import datetime
import csv
import os

import json
import threading
import paho.mqtt.client as mqtt

app = Flask(__name__, static_folder="static", static_url_path="/static")

DATA_FILE = "dashboard_history.csv"
FIELDNAMES = [
    "timestamp",
    "temperature",
    "humidity",
    "windSpeed",
    "windDirection",
    "rainfall",
    "rainDetected",
    "pulses",
    "tipCount",
]

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "weather/data"

history = deque(maxlen=5000)

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT")
    client.subscribe(MQTT_TOPIC)


def on_message(client, userdata, msg):

    try:

        payload = json.loads(
            msg.payload.decode()
        )

        record = normalize_record(payload)

        history.append(record)

        append_to_csv(record)

        print("MQTT Data:", record)

    except Exception as e:

        print("MQTT Error:", e)


def start_mqtt():

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)

    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(
        MQTT_BROKER,
        MQTT_PORT,
        60
    )

    client.loop_forever()

def to_float(value):
    try:
        if value is None or value == "" or value == "null":
            return None
        return float(value)
    except (TypeError, ValueError):
        return None

def to_int(value):
    try:
        if value is None or value == "" or value == "null":
            return None
        return int(float(value))
    except (TypeError, ValueError):
        return None

def to_bool(value):
    if isinstance(value, bool):
        return value
    if value in (1, "1", "true", "True", "TRUE"):
        return True
    return False

def normalize_record(data):
    ts = data.get("timestamp")
    if not ts:
        ts = datetime.now().isoformat(timespec="seconds")

    record = {
        "timestamp": ts,
        "temperature": to_float(data.get("temperature")),
        "humidity": to_float(data.get("humidity")),
        "windSpeed": to_float(data.get("windSpeed")),
        "windDirection": str(data.get("windDirection", "Unknown")),
        "rainfall": to_float(data.get("rainfall")),
        "rainDetected": to_bool(data.get("rainDetected")),
        "pulses": to_int(data.get("pulses")),
        "tipCount": to_int(data.get("tipCount")),
    }
    return record

def append_to_csv(record):
    file_exists = os.path.exists(DATA_FILE)
    with open(DATA_FILE, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        if not file_exists:
            writer.writeheader()
        writer.writerow(record)

def load_existing_history():
    if not os.path.exists(DATA_FILE):
        return
    with open(DATA_FILE, "r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            record = {
                "timestamp": row.get("timestamp", ""),
                "temperature": to_float(row.get("temperature")),
                "humidity": to_float(row.get("humidity")),
                "windSpeed": to_float(row.get("windSpeed")),
                "windDirection": row.get("windDirection", "Unknown"),
                "rainfall": to_float(row.get("rainfall")),
                "rainDetected": to_bool(row.get("rainDetected")),
                "pulses": to_int(row.get("pulses")),
                "tipCount": to_int(row.get("tipCount")),
            }
            history.append(record)

load_existing_history()

@app.route("/")
def home():
    return render_template("index.html")





@app.route("/api/latest", methods=["GET"])
def latest():
    if not history:
        return jsonify({})
    return jsonify(history[-1])


@app.route("/api/history", methods=["GET"])
def get_history():
    limit = request.args.get("limit", default=300, type=int)
    limit = max(1, min(limit, 5000))
    return jsonify(list(history)[-limit:])

mqtt_thread = threading.Thread(
    target=start_mqtt,
    daemon=True
)

mqtt_thread.start()


if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=True
    )