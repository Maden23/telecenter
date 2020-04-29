from flask import Flask, render_template, flash, url_for, redirect, request
from operator import setitem
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
    deleteAllForm = cameralist_forms.DeleteAllForm()
    deleteCameraForm = cameralist_forms.DeleteCameraForm()
    
    # adding new camera to json
    if addForm.addSubmit.data and addForm.validate():
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
    

    # # delete all cameras
    if deleteAllForm.deleteAllSubmit.data and deleteAllForm.validate():
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
    

    # delete specified camera
    if deleteCameraForm.deleteCameraSubmit.data and deleteCameraForm.validate():
        # reading data from file
        with open('cams.json', 'r') as json_file:
            data = json.load(json_file)
            json_file.close()
        
        # writing data to json
        with open('cams.json', 'w') as json_file:
            temp = data['cams']
            formValues = request.values.to_dict()
            cameraID = formValues['cameraID']
            temp.pop(int(cameraID))
            data['cams'] = temp
            json.dump(data, json_file, indent=4)
            json_file.close()

        # web-page success message
        flash(f'Camera with ID={cameraID} has been removed successfuly', 'success')
        return redirect(url_for('recorder'))


    # reading cameras from json
    with open('cams.json') as json_file:
        data = json.load(json_file)
        # cams = {}
        # for i in range(len(data['cams'])):
        #     cams.update({data['cams'][i]['name']: data['cams'][i]['address']})
        
        cams = []
        id = 0
        for item in data['cams']:
            cams.append([id, item['name'], item['address']])
            id += 1

    return render_template('recorder.html', cams=cams, addForm=addForm, 
                            deleteAllForm=deleteAllForm, deleteCameraForm=deleteCameraForm)


@app.route('/recorder/edit/<cameraID>', methods=['GET', 'POST'])
def recorder_edit(cameraID):
    editForm = cameralist_forms.EditCameraForm()

    # searching camera info by id in json
    with open('cams.json', 'r') as json_file:
        data = json.load(json_file)
        cameraInfo = data['cams'][int(cameraID)]
        json_file.close()
    
    # if delete button was pressed
    if editForm.deleteSubmit.data and editForm.validate():
        with open('cams.json', 'r') as json_file:
            data = json.load(json_file)
            json_file.close()
        
        with open('cams.json', 'w') as json_file:
            temp = data['cams']
            temp.pop(int(cameraID))
            data['cams'] = temp
            json.dump(data, json_file, indent=4)
            json_file.close()
        
        # web-page success message
        flash(f'Camera "{editForm.cameraName.data}" has been removed successfuly', 'success')
        return redirect(url_for('recorder'))
    
    # if edit button was pressed
    if editForm.editSubmit.data and editForm.validate():
        with open('cams.json', 'r') as json_file:
            data = json.load(json_file)
            json_file.close()
        
        with open('cams.json', 'w') as json_file:
            temp = data['cams']
            temp[int(cameraID)] = {
                        "name": editForm.cameraName.data,
                        "address": editForm.rtspName.data
                    }
            json_file.seek(0)
            json.dump(data, json_file, indent=4)
            json_file.close()
        
        # web-page success message
        flash(f'Camera "{editForm.cameraName.data}" has been edited successfuly', 'success')
        return redirect(url_for('recorder'))
    
    return render_template('recorder_edit.html', camera=cameraInfo, editForm=editForm)