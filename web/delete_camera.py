import json

with open('cams.json', 'r') as json_file:
    data = json.load(json_file)
    json_file.close()
        

with open('cams.json', 'w') as json_file:
    temp = data['cams']

    print("data: ", data)
    print("temp: ", temp)

    temp.pop(2)
    data['cams'] = temp
    json.dump(data, json_file, indent=4)
    json_file.close()