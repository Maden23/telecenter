import os
import sys  # for arguments

import pickle
from os import path, remove
from googleapiclient.discovery import build
from googleapiclient.http import  MediaFileUpload
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request

TOKEN_STORAGE = '../auth/gdrive.pickle'
CLIENT_SECRET = '../auth/gdrive_secret.json'

try:
    import argparse
    parser = argparse.ArgumentParser("Path to file")
    parser.add_argument("pathToFile", help = "Path to a videofile.", type = str)  # videofile name argument

    args = parser.parse_args()
    print("Trying to open", args.pathToFile, "...")
except:
    print("Failed to parse arguments")

#--------------------------------- OAuth -----------------------------------------
# If modifying these scopes, delete the file gdrive.pickle
SCOPES = ['https://www.googleapis.com/auth/drive.file']

creds = None
# The file gdrive.pickle stores the user's access and refresh tokens, and is
# created automatically when the authorization flow completes for the first
# time.
if path.exists(TOKEN_STORAGE):
    with open(TOKEN_STORAGE, 'rb') as token:
        creds = pickle.load(token)
# If there are no (valid) credentials available, let the user log in.
if not creds or not creds.valid:
    if creds and creds.expired and creds.refresh_token:
        creds.refresh(Request())
    else:
        flow = InstalledAppFlow.from_client_secrets_file(
            CLIENT_SECRET, SCOPES)
        creds = flow.run_local_server(port=0)
    # Save the credentials for the next run
    with open(TOKEN_STORAGE, 'wb') as token:
        pickle.dump(creds, token)


try:
    DRIVE = build('drive', 'v3', credentials=creds)
    print("HTTP authorization success")
except:
    print("HTTP error occured")


print("Uploading file to Google Drive ...")

fileName = args.pathToFile.split('/')[-1]
metadata = {'name': fileName}

ext = fileName.split('.')[-1]
if ext == "mp4":
    mimetype = 'video/mp4'
if ext == "aac":
    mimetype = 'audio/aac'

try:
    media = MediaFileUpload(args.pathToFile, mimetype=mimetype, resumable=True)
    res = DRIVE.files().create(body = metadata, media_body = media).execute()
except Exception as e:
    print("Upload failed.", e.__class__)
    exit(1)

# Upload was successfull, removing files
os.remove(args.pathToFile)

