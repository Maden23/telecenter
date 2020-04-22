from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField
from wtforms.validators import DataRequired

class AddCameraForm(FlaskForm):
    cameraName = StringField('Camera name', validators=[DataRequired()])
    rtspName = StringField('RTSP address', validators=[DataRequired()])
    submit = SubmitField('Add new camera')

class DeleteAllForm(FlaskForm):
    submit = SubmitField('Delete all')
