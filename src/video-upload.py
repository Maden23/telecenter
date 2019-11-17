import os
import sys  # for arguments

from apiclient.discovery import build
from httplib2 import Http
from oauth2client import file, client, tools

try:
	import argparse
	parser = argparse.ArgumentParser("Videofile name and location")
	parser.add_argument("filename", help = "The name of a videofile.", type = str)  # videofile name argument
	parser.add_argument("location", help = "The location of a videofile", type = str)  # videofile location argument
	args = parser.parse_args()
	print("Trying to open", args.filename, "from", args.location, "...")
except:
	print("Failed to parse arguments")

SCOPES = 'https://www.googleapis.com/auth/drive.file'
store = file.Storage('storage.json')  # config file
creds = store.get()
if not creds or creds.invalid:
    flow = client.flow_from_clientsecrets('client_secret.json', scope = SCOPES)
    creds = tools.run_flw(flow, store, flags) \
            if flags else tools.run(flow, store)

try:
	DRIVE = build('drive', 'v3', http = creds.authorize(Http()))
	print("HTTP authorization success")
except:
	print("HTTP error occured")

filename = args.location + args.filename
print("Uploading file to Google Drive ...")

metadata = {'name': args.filename}
metadata['mimeType'] = None

try:
	res = DRIVE.files().create(body = metadata, media_body = filename).execute()
	if res:
		print('File upload success')
	else:
		print('File upload failed')
except:
	print("Upload failed. Please check filename and location to the file.")

