from flask import Flask, render_template, flash, url_for, redirect
from forms import CameraListForm
import json
import os

app = Flask(__name__)

SECRET_KEY = os.urandom(16)
app.config['SECRET_KEY'] = SECRET_KEY

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/recorder', methods=['GET', 'POST'])
def recorder():
    form = CameraListForm()
    
    # adding new camera to json
    if form.validate_on_submit():
        # writing data to file
        with open('cams.json', 'r+') as json_file:
            data = json.load(json_file)
            temp = data['cams']
            new_cam = {
                        "name": form.cameraName.data,
                        "address": form.rtspName.data
                    }
            temp.append(new_cam)
            json_file.seek(0)
            json.dump(data, json_file, indent=4)
            json_file.close()

        # web-page success message
        flash(f'Camera {form.cameraName.data} has been added successfuly!', 'success')
        return redirect(url_for('recorder'))

    # reading cameras from json
    with open('cams.json') as json_file:
        data = json.load(json_file)

        cams = {}

        for i in range(len(data['cams'])):
            cams.update({data['cams'][i]['name']: data['cams'][i]['address']})

    return render_template('recorder.html', cams=cams, form=form)
