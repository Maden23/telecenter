import requests
import json
import os
ROOMS_FILE = "db-rooms.json"

with open(ROOMS_FILE, 'w') as file:
    response = requests.get('https://nvr.miem.hse.ru/api/erudite/equipment',
                            headers={'key': os.environ.get('ERUDITE_KEY')})

    response = response.content.decode('utf8').replace("'", '"')
    data = json.loads(response)
    info = {}

    for room in data:
        if room['room_name'] in info:
            info[room["room_name"]]['cameras'].append(room)
        else:
            info[str(room["room_name"])] = {
                "cameras": [room],
                "audio": []
            }

    info = json.dumps(info)
    info = json.loads(info)
    json.dump(info, file, ensure_ascii=False, indent=4)
    file.close()

