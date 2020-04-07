Набор программ для использования в модулях программно-аппаратного комплекса IP-телестудии. Исходный код каждой программы лежит в директории с соответствующим названием:

| Программа | Описание |
| ------    | ------ |
| Recorder  | Выполняет запись видеопотоков, и их загрузку в Google Drive  |
| Grid      | Отображает сетку изображений с камер  | 
| Network    | Выполняет ping камер и отображает график изменения значений для выбранной камеры | 

**Содержание**

[[_TOC_]]

# Документация
[Документация исходного кода (Doxygen)](https://maden23.gitlab.io/telecenter)

# Зависимости
### Зависимости для Recorder и Grid
##### GStreamer
Фреймворк для работы с видео
````
apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
````

##### GTK+
Фреймворк для графического интерфейса
```
sudo apt-get install libgtk-3-dev
```

#### Google API
Библиотека python для доступа к GSuite и Google Drive
```
pip3 install google-api-python-client google-auth-httplib2 google-auth-oauthlib
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
####  2. Клонировать этот репозиторий

    git clone https://git.miem.hse.ru/19102/telecenter.git

####  3. Получить токены для сервисов Google

####  4. Выполнить компиляцию (для Recorder и Grid)
###### Перейти в директорию с нужной программой
Для Recorder:
    
    cd telecenter/Recorder

Для Grid:

    cd telecenter/Grid

###### Компиляция
    make clean
    make

####  5. Настроить платформу (только для Recorder)
В файле Recorder/recorder.conf изменить параметр `platform`, в зависимости от аппаратной платформы, на которой планируется запускать программу:

| Платформа | Значение |
| ------    | ------ |
| Nvidia Jetson Nano | `platform = jetson` |
| Любая другая | `platform = other` |

# Запуск
##### Запуск Recorder после [компиляции](#4-выполнить-компиляцию-для-recorder-и-grid)
    cd Recorder
    ./recorder
##### Запуск Grid после [компиляции](#4-выполнить-компиляцию-для-recorder-и-grid)
    cd Grid
    ./grid
##### Запуск Network
    cd Network/src
    python3 network_monitor.py

