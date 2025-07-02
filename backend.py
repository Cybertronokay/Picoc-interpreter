import os
from flask import Flask, request, jsonify
from flask_cors import CORS
import subprocess

app = Flask(__name__)
CORS(app)  # Allow CORS for all routes

@app.route("/evaluate", methods=["POST"])
def evaluate():
    if "file" not in request.files:
        return jsonify({"output": "No file uploaded"}), 400

    file = request.files["file"]
    if file.filename == "":
        return jsonify({"output": "Empty filename"}), 400

    # Save uploaded file as input.txt
    file.save("input.txt")
    print("Received file and saved as input.txt")

    return compile_and_run()

@app.route("/evaluate_code", methods=["POST"])
def evaluate_code():
    data = request.get_json()
    code = data.get("code")

    if not code:
        return jsonify({"output": "No code provided"}), 400

    # Save raw code to input.txt
    with open("input.txt", "w") as f:
        f.write(code)

    print("Received code via editor and saved as input.txt")
    
    return compile_and_run()

def compile_and_run():
    # Compile the C interpreter
    try:
        subprocess.run(["gcc", "picoc_web.c", "-o", "picoc_web.exe"], check=True)
    except subprocess.CalledProcessError as e:
        return jsonify({"output": f"Compilation Error:\n{e}"}), 200

    # Run the executable
    try:
        result = subprocess.check_output(["picoc_web.exe"], stderr=subprocess.STDOUT)
        output = result.decode().strip()
    except subprocess.CalledProcessError as e:
        output = f"Runtime Error:\n{e.output.decode() if e.output else str(e)}"

    print("Sending back:", output)
    return jsonify({"output": output}), 200

if __name__ == "__main__":
    app.run(debug=True)




