
import sys
import json
import os
import requests
from onvif import ONVIFCamera
ROOMS_INFO_STORAGE = 'rooms.json'

def load_cameras():
    with open(ROOMS_INFO_STORAGE, 'r') as json_file:
        data = json.load(json_file)
        for room in data:
            for info_cam in data[room]:
                try:
                    ping = os.system("ping -c 1 " + info_cam["ip"])
                    if ping == 0:
                        mycam = ONVIFCamera(info_cam["ip"], str(
                            info_cam["port"]), "admin", "Supervisor", '/usr/local/lib/python3.6/dist-packages/wsdl/')
                        resp_info = mycam.devicemgmt.GetDeviceInformation()
                        resp_network = mycam.devicemgmt.GetNetworkInterfaces()
                        nameCamera = "Camera_" + info_cam["ip"] 
                        if resp_info["Model"] is not None:
                             nameCamera = resp_info["Model"]
                        camera = {
                            "name": nameCamera,
                            "type": info_cam["type"],
                            "room_name": room,
                            "room_id": "string",
                            "ip": info_cam["ip"],
                            "port": info_cam["port"],
                            "mac": resp_network[0]["Info"]["HwAddress"]
                        }
                        media_service = mycam.create_media_service()
                        profiles = media_service.GetProfiles()
                        for num, profile in enumerate(profiles):
                            token = profile.token
                            mycam = media_service.create_type('GetStreamUri')
                            mycam.ProfileToken = token
                            mycam.StreamSetup = {
                                'Stream': 'RTP-Unicast', 'Transport': {'Protocol': 'RTSP'}}
                            rtsp = media_service.GetStreamUri(mycam)
                            camera["rtsp_" + str(num)] = rtsp["Uri"]
                        camera["rtsp_main"] = camera["rtsp_0"]
                        print(str(camera))
                        r = requests.post('https://nvr.miem.hse.ru/api/erudite/equipment', data=json.dumps(camera), headers={'key': os.environ.get('ERUDITE_KEY')})
                        print("STATUS",r.status_code)
                    else:
                        continue
                except:
                    print("error camera :(")
                    continue
        json_file.close()

load_cameras()
