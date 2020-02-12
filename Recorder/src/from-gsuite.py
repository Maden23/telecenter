import pickle
import json
import os.path
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request

# If modifying these scopes, delete the file token.pickle.
SCOPES = ['https://www.googleapis.com/auth/admin.directory.resource.calendar']

def main():
    """ Fetches cameras from GSuite and writes them to JSON file grouping by room 
    """
    creds = None
    # The file token.pickle stores the user's access and refresh tokens, and is
    # created automatically when the authorization flow completes for the first
    # time.
    if os.path.exists('token.pickle'):
        with open('token.pickle', 'rb') as token:
            creds = pickle.load(token)
    # If there are no (valid) credentials available, let the user log in.
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(
                'gsuite_secret.json', SCOPES)
            creds = flow.run_local_server(port=0)
        # Save the credentials for the next run
        with open('token.pickle', 'wb') as token:
            pickle.dump(creds, token)

    service = build('admin', 'directory_v1', credentials=creds)

    # Call the Admin SDK Directory API
    results = service.resources().calendars().list(customer='my_customer').execute()
    items = results.get('items', [])

    # Distribute ONVIF-cameras (names and adresses) to rooms
    rooms = {} # {"room1" : [("cam1", "rtsp://123"), ("cam2", "rtsp://1234")], "room2" : {...}}
    if not items:
        print('No cameras in the domain.')
        exit(-1)

    for item in items:
        if item['resourceType'] == "ONVIF-camera":
            if not item['floorSection'] in rooms:
                rooms[item['floorSection']] = []
            name = item['resourceName']
            # Make RTSP address
            info = json.loads(item['resourceDescription'])
            rtsp = "rtsp://" 
            if 'login' in info and 'password' in info:
                rtsp += info['login'] + ":" + info['password'] + "@"
            rtsp += info['ip']

            rooms[item['floorSection']].append((name, rtsp))

    # Convert to JSON-like structure
    JSONrooms = {}
    for room in rooms:
        JSONrooms[room] = []
        for camName, camAddress in rooms[room]:
            JSONrooms[room].append({'name' : camName, 'address' : camAddress})

    with open("gsuite_rooms.json", "w", encoding='utf8') as file:
        json.dump(JSONrooms, file, ensure_ascii=False)


if __name__ == '__main__':
    main()