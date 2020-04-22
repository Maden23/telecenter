from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField
from wtforms.validators import DataRequired

class CameraListForm(FlaskForm):
    cameraName = StringField('Camera name', validators=[DataRequired()])
    rtspName = StringField('RTSP address', validators=[DataRequired()])
    submit = SubmitField('Add new camera')