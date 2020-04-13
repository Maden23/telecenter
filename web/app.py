from flask import Flask
from flask import render_template
import json

app = Flask(__name__)

@app.route('/')
def index():
    return 'Index page'

@app.route('/hello')
def hello():
    return 'Hello, world!'

@app.route('/recorder')
def recorder():
    with open('cams.json') as json_file:
        data = json.load(json_file)

        cams = {}

        for i in range(len(data['cams'])):
            cams.update({data['cams'][i]['name']: data['cams'][i]['address']})

    return render_template('index.html', cams=cams)