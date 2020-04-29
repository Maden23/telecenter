from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField, HiddenField
from wtforms.validators import DataRequired

class AddCameraForm(FlaskForm):
    cameraName = StringField('Camera name', validators=[DataRequired()])
    rtspName = StringField('RTSP address', validators=[DataRequired()])
    addSubmit = SubmitField('Add new camera')

class DeleteAllForm(FlaskForm):
    deleteAllSubmit = SubmitField('Delete all')

class DeleteCameraForm(FlaskForm):
    cameraID = HiddenField('Camera ID')
    deleteCameraSubmit = SubmitField('Delete camera')

class EditCameraForm(FlaskForm):
    cameraID = HiddenField('Camera ID')
    cameraName = StringField('Camera name')
    rtspName = StringField('RTSP address')
    editSubmit = SubmitField('Save data')
    deleteSubmit = SubmitField('Delete camera')
