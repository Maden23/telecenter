# Что в этом репозитории
Набор программ для использования в модулях программно-аппаратного комплекса IP-телестудии. Исходный код каждой программы лежит в директории с соответствующим названием:

| Директория | Описание |
| ------    | ------ |
| [Recorder](https://git.miem.hse.ru/19102/telecenter/-/tree/master/Recorder)  | Выполняет запись видеопотоков, и их загрузку в Google Drive  |
| [Network](https://git.miem.hse.ru/19102/telecenter/-/tree/master/Network)    | Выполняет ping камер и отображает график изменения значений для выбранной камеры | 
| [Grid](https://git.miem.hse.ru/19102/telecenter/-/tree/master/Grid)          | Отображает сетку изображений с камер  | 
| [SingleStream](https://git.miem.hse.ru/19102/telecenter/-/tree/master/SingleStream)    | Отображает поток с одной камеры на полный экран | 
| [core](https://git.miem.hse.ru/19102/telecenter/-/tree/master/core)          | Содержит классы, которые используются в нескольких программах  | 
| [auth](https://git.miem.hse.ru/19102/auth)                                   | Это submodule (ссылка на репозиторий внутри репозитория). Содержит данные для Google-авторизации приложений. Доступен только членам команды. |

### Дополнительно
Назначение других файлов в репозитории:

| Название | Описание |
| ------   | ------ |
| [doxygen](https://git.miem.hse.ru/19102/telecenter/-/tree/master/doxygen)  | Директория содержит конфигурацию для автоматической генерации документации с помощью Doxygen и сгенерированные файлы |
| [.gitlab-ci.yml](https://git.miem.hse.ru/19102/telecenter/-/blob/master/.gitlab-ci.yml)| Файл для выполнения CI пайплайна в другом репозитории (настроен Mirroring), где деплоятся Pages. С помощью него автоматически обновляется [документация исходного кода](https://maden23.gitlab.io/telecenter) после каждого коммита. |

**Содержание**

[[_TOC_]]

# Документация
- [Wiki](https://git.miem.hse.ru/19102/telecenter/-/wikis/home)
- [Документация исходного кода (Doxygen)](https://maden23.gitlab.io/telecenter)
- [Описание проекта](https://docs.google.com/document/d/1h5Fe9YLLMEI-pzpfbAH7BXZpYWnlAfymhN2KjJObhCY/edit?usp=sharing)

**Формальности**
- [Программные документы](https://drive.google.com/drive/folders/1Ovr58dabMcnGOkfGxf8s12jKU7rTYFOc?usp=sharing)
- [Конструкторские документы](https://drive.google.com/drive/folders/1MhgQvr0wkbufuU-ss9-4LadWp8AWsSrF?usp=sharing)
- [Пользовательские документы](https://drive.google.com/drive/folders/1bpvONr3PEYKrJsVolynj8CtA4zE5wT8k?usp=sharing)


# Зависимости
### Зависимости для Recorder, Grid и SingleStream
##### GStreamer
Фреймворк для работы с видео
````
sudo apt-get install libgstreamer1.0-0 libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
````

##### GTK+
Фреймворк для графического интерфейса
```
sudo apt-get install libgtk-3-dev
```

##### Google API
Библиотека python для доступа к GSuite и Google Drive
```
sudo apt-get install python3-pip
pip3 install google-api-python-client google-auth-httplib2 google-auth-oauthlib
```

### Еще для Grid и SingleStream
##### [Eclipse Paho MQTT](https://github.com/eclipse/paho.mqtt.cpp)
Библиотека для обмена данными по протоколу MQTT для C++
```bash
cd
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
git checkout v1.3.1

cmake -Bbuild -H. -DPAHO_WITH_SSL=OFF -DPAHO_ENABLE_TESTING=OFF
sudo cmake --build build/ --target install
sudo ldconfig
cd ..
sudo rm -r paho.mqtt.c

git clone https://github.com/eclipse/paho.mqtt.cpp
cd paho.mqtt.cpp
cmake -Bbuild -H. -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=FALSE -DPAHO_WITH_SSL=OFF
sudo cmake --build build/ --target install
sudo ldconfig
cd ..
sudo rm -r paho.mqtt.cpp 
```

### Зависимости для Network
##### PyQt5
Фреймворк Qt для python. Используется для графического интерфейса
```
pip3 install pyqt5
```

##### fping
Библиотека для выполнения ping из кода на python
```
pip3 install fping
```

# Порядок установки
####  1. [Установить зависимости](#зависимости)
####  2. Клонировать этот репозиторий и получить файлы для авторизации

    git clone https://git.miem.hse.ru/19102/telecenter.git
    cd telecenter/auth
    git submodule init
    git submodule update
    cd ..
####  3. Выполнить компиляцию (кроме NetworkMonitor)
###### Создать директории для бинарных файлов
``` bash
mkdir core/obj Recorder/obj Grid/obj SingleStream/obj
```
###### Перейти в директорию с нужной программой
Для Recorder:
    
    cd Recorder

Для Grid:

    cd Grid

Для SingleStream:

    cd SingleStream

###### Компиляция
    make clean
    make

####  4. Настроить платформу
В файле .conf изменить параметр `platform`, в зависимости от аппаратной платформы, на которой планируется запускать программу:

| Платформа | Значение |
| ------    | ------ |
| Nvidia Jetson Nano | `platform = jetson` |
| Любая другая | `platform = other` |

# Запуск
##### Запуск Recorder после [компиляции](#3-выполнить-компиляцию-кроме-network)
    ./recorder
##### Запуск Grid после [компиляции](#3-выполнить-компиляцию-кроме-network)
    ./grid
##### Запуск SingleStream после [компиляции](#3-выполнить-компиляцию-кроме-network)
    ./singlestream 
##### Запуск Network
    cd ~/telecenter/Network/src
    python3 network_monitor.py

