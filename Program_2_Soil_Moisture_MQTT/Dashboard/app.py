from flask import Flask
from flask import render_template
from flask import jsonify
import csv
import os

app = Flask(__name__)

CSV_FILE = "soil_history.csv"

@app.route("/")
def home():
    return render_template("index.html")

@app.route("/api/data")
def api_data():

    values = []

    if os.path.exists(CSV_FILE):

        with open(CSV_FILE, newline="") as file:

            reader = csv.DictReader(file)

            for row in reader:

                try:
                    values.append(
                        float(row["soil"])
                    )
                except:
                    pass

    if not values:

        return jsonify({
            "current": 0,
            "min": 0,
            "max": 0,
            "avg": 0,
            "history": []
        })

    return jsonify({
        "current": values[-1],
        "min": min(values),
        "max": max(values),
        "avg": round(sum(values)/len(values), 1),
        "history": values[-50:]
    })

if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=True
    )