from flask import Flask, render_template, flash, url_for, redirect
import cameralist_forms
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
    addForm = cameralist_forms.AddCameraForm()
    deleteForm = cameralist_forms.DeleteAllForm()
    
    # adding new camera to json
    if addForm.validate_on_submit():
        # writing data to file
        with open('cams.json', 'r+') as json_file:
            data = json.load(json_file)
            temp = data['cams']
            new_cam = {
                        "name": addForm.cameraName.data,
                        "address": addForm.rtspName.data
                    }
            temp.append(new_cam)
            json_file.seek(0)
            json.dump(data, json_file, indent=4)
            json_file.close()

        # web-page success message
        flash(f'Camera {addForm.cameraName.data} has been added successfuly!', 'success')
        return redirect(url_for('recorder'))
    

    # delete all cameras
    if deleteForm.validate_on_submit():
        # reading data from file
        with open('cams.json', 'r') as json_file:
            data = json.load(json_file)
            json_file.close()
        
        # writing empty data to json
        with open('cams.json', 'w') as json_file:
            temp = data['cams']
            temp.clear()
            data['cams'] = temp
            json.dump(data, json_file, indent=4)
            json_file.close()

        # web-page success message
        flash(f'All cameras have been removed successfuly', 'success')
        return redirect(url_for('recorder'))


    # reading cameras from json
    with open('cams.json') as json_file:
        data = json.load(json_file)

        cams = {}

        for i in range(len(data['cams'])):
            cams.update({data['cams'][i]['name']: data['cams'][i]['address']})

    return render_template('recorder.html', cams=cams, addForm=addForm, deleteForm=deleteForm)
