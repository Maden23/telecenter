import pickle
import json
import os.path
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request

TOKEN_STORAGE = '../auth/gsuite_token.pickle'
CLIENT_SECRET = '../auth/gsuite_secret.json'

# If modifying these scopes, delete the file token.pickle.
SCOPES = ['https://www.googleapis.com/auth/admin.directory.resource.calendar']

def main():
    """ Fetches cameras from GSuite and writes them to JSON file grouping by room 
    """
    creds = None
    # The file token.pickle stores the user's access and refresh tokens, and is
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
        service = build('admin', 'directory_v1', credentials=creds)
    except socket.gaierror:
        print ("No connection to GSuite\n")
        exit(1)

    # Call the Admin SDK Directory API
    results = service.resources().calendars().list(customer='my_customer').execute()
    items = results.get('items', [])
    # Distribute ONVIF-cameras (names and adresses) to rooms
    rooms = {} # {"room1" : { "cameras" :[("cam1", "cam1 full", "rtsp://123"), ("cam2", "cam2 full", "rtsp://1234")], "audio": [("source1", "rtsp://..."), ("sourse2", "rtsp://")], "room2" : {...}}
    if not items:
        print('No cameras in the domain.')
        exit(-1)

    for item in items:
        if 'resourceType' in item and item['resourceType'] in ["ONVIF-camera", "Encoder", "Enc/Dec"]:
            if not item['floorSection'] in rooms:
                rooms[item['floorSection']] = {"cameras" : [], "audio" : []}
            
            if 'userVisibleDescription' in item.keys() and item['userVisibleDescription'] != "":
                name = item['userVisibleDescription'] 
            else: name = item['resourceName']

            fullname = item['resourceName']
            # Make RTSP address
            info = json.loads(item['resourceDescription'])
            rtsp = "rtsp://" 
            if "rtsp_mainstream" in info:
                rtsp = info["rtsp_mainstream"]
            else:
                if 'login' in info and 'password' in info:
                    rtsp += info['login'] + ":" + info['password'] + "@"
                else:
                    rtsp += "admin:Supervisor@"
                rtsp += info['ip']

            # If item has audio, place it in "audio" section
            # Checking for custom rtsp mics only
            if 'audio' in info and info["audio"] == "rtsp_mic":
                rooms[item['floorSection']]["audio"].append({'name' : name, 'full_name' : fullname, 'address' : rtsp})


            rooms[item['floorSection']]["cameras"].append({'name' : name, 'full_name' : fullname, 'address' : rtsp})

    # Convert to JSON-like structure
    # JSONrooms = {}
    # for room in rooms:
    #     JSONrooms[room] = []
    #     for camName, fullCamName, camAddress in rooms[room]:
    #         JSONrooms[room].append({'name' : camName, 'full_name' : fullCamName, 'address' : camAddress})

    with open("gsuite_rooms.json", "w", encoding='utf8') as file:
        # json.dump(JSONrooms, file, ensure_ascii=False)
        json.dump(rooms, file, ensure_ascii=False)


if __name__ == '__main__':
    main()
