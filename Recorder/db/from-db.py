import requests
import json
ROOMS_FILE = "db-rooms.json"

with open(ROOMS_FILE, 'w') as file:
    response = requests.get('https://nvr.miem.hse.ru/api/erudite/equipment',
                            headers={'key': '564e99c0d9ce4003a3b7ae4ff90fad89'})

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

