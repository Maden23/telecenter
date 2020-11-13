import os
import sys  # for arguments

import pickle
import os.path
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request

TOKEN_STORAGE = '../auth/gdrive.pickle'
CLIENT_SECRET = '../auth/gdrive_secret.json'

try:
    import argparse
    # parser = argparse.ArgumentParser("Videofile name and location")
    # parser.add_argument("filename", help = "The name of a videofile.", type = str)  # videofile name argument
    # parser.add_argument("location", help = "The location of a videofile", type = str)  # videofile location argument
    parser = argparse.ArgumentParser("Path to file")
    parser.add_argument("filename", help = "Path to a videofile.", type = str)  # videofile name argument

    args = parser.parse_args()
    # print("Trying to open", args.filename, "from", args.location, "...")
    print("Trying to open", args.filename, "...")
except:
    print("Failed to parse arguments")

#--------------------------------- OAuth -----------------------------------------
# If modifying these scopes, delete the file gdrive.pickle
SCOPES = ['https://www.googleapis.com/auth/drive.file']

creds = None
# The file gdrive.pickle stores the user's access and refresh tokens, and is
# created automatically when the authorization flow completes for the first
# time.
if os.path.exists(TOKEN_STORAGE):
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

# filename = args.location + args.filename
filename = args.filename
print("Uploading file to Google Drive ...")

metadata = {'name': args.filename.split('/')[-1]}
metadata['mimeType'] = None

try:
    res = DRIVE.files().create(body = metadata, media_body = filename).execute()
    if res:
        print('File upload success')
    else:
        print('File upload failed')
except:
    print("Upload failed. Please check filename and location to the file.")

