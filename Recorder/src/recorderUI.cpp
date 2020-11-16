#include "recorderUI.h"

RecorderUI::RecorderUI(Config *config)
{
    this->config = config;
    this->rooms = config->getRooms();

    gtk_init (nullptr, nullptr);
    
    /* Init windows */
    playerBuilder = nullptr;
    playerWindow = windowInit(&playerBuilder, "ui/player.glade", "player");
    menuBuilder = nullptr;
    menuWindow = windowInit(&menuBuilder, "ui/menu.glade", "menu");
    
    if(menuWindow == nullptr || menuBuilder == nullptr || playerWindow == nullptr || playerBuilder == nullptr) {
        cerr << "Failed to init builder" << endl;
        exit(0);
    }

    /* Set window size */
    gtk_widget_set_size_request(menuWindow, stoi(config->getParam("windowWidth")),
            stoi(config->getParam("windowHeight")));
    gtk_widget_set_size_request(playerWindow, stoi(config->getParam("windowWidth")),
            stoi(config->getParam("windowHeight")));

    /* Start/stop recording on key press */
    recManager = new RecManager(config);
    gtk_widget_add_events(menuWindow, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(menuWindow), "key_press_event", G_CALLBACK(keyPressHandle), this);


    /* Find player and window elements to control*/
    initPlayerWidgets();
    player = new Player(playerWidget, config->getParam("platform"));
    initMenuWidgets();

    /* Add information to status bar */
    // IP address
    string ip = findIP();
    GtkWidget* ipLabel = GTK_WIDGET (gtk_builder_get_object(menuBuilder, "ipLabel"));
    gtk_label_set_text(GTK_LABEL(ipLabel), ip.c_str());
    // Disk space
    const unsigned int GB = (1024 * 1024) * 1024;
    struct statvfs buffer;
    int ret = statvfs("/", &buffer);
    if (!ret) {
        double available = (double)(buffer.f_bfree * buffer.f_frsize) / GB;
        ostringstream streamObj;
        streamObj << fixed << setprecision(2) << available;
        string space = streamObj.str() + " Gb";
        GtkWidget* spaceLabel = GTK_WIDGET (gtk_builder_get_object(menuBuilder, "spaceLabel"));
        gtk_label_set_text(GTK_LABEL(spaceLabel), space.c_str());
    }
    // GDrive upload status
    struct gdrive_status_data data;
    data.recManager = recManager;
    GtkWidget* GDriveIcon = (GtkWidget*) gtk_builder_get_object(menuBuilder, "GDriveIcon");
    data.GDriveIcon = GDriveIcon;
    g_timeout_add(500, updateGDriveStatus, &data);

	/* Init styles */
    int res = initStyles();
    if(res == -1) {
        cerr << "Failed to get styles" << endl << endl;
    }

    g_signal_connect(menuWindow, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    /* Place windows */
    if (config->getParamInt("screens") == 2)
    {
        cout << "Showing GUI on 2 screens" << endl << endl;
        GdkScreen *screen = gdk_screen_get_default();
        gtk_window_fullscreen_on_monitor(GTK_WINDOW(menuWindow), screen, 0);
        gtk_widget_show(menuWindow);
        gtk_window_fullscreen_on_monitor(GTK_WINDOW(playerWindow), screen, 1);
        gtk_widget_show(playerWindow);
    }
    else
    {
        /* For one screen */
        cout << "Showing GUI on 1 screen" << endl << endl;
        gtk_window_move(GTK_WINDOW(menuWindow), 0, 100);
        gtk_window_move(GTK_WINDOW(playerWindow), config->getParamInt("windowWidth") + 70, 100);
        gtk_window_present(GTK_WINDOW(playerWindow));
        gtk_window_present(GTK_WINDOW(menuWindow));
    }
    

    gtk_main();
}

RecorderUI::~RecorderUI()
{
    delete player;
    delete recManager;
}


int RecorderUI::initStyles() {
    GFile* css = g_file_new_for_path("ui/styles.css");
    if(css == nullptr) {
      std::cout << "File not found" << std::endl;
      return -1;
    }
    GError* err = nullptr;
    GtkCssProvider *css_provider =  gtk_css_provider_new();
    gtk_css_provider_load_from_file(css_provider, css, &err);
    if(err != nullptr) {
      cerr << "Error occured while loading styles: " <<  err->message;
      g_error_free(err);
      return -1;
    }

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), (GtkStyleProvider*) css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
    return 0;
}

GtkWidget* RecorderUI::windowInit(GtkBuilder** builder, string gladeFile, string windowName) {
    ifstream f(gladeFile.c_str());
    if (!f.good())
    {
        cerr << "Could not find " << gladeFile << endl;
        return nullptr;
    }

    *builder = gtk_builder_new();

    if (builder == nullptr) {
        cerr << "No builder for " << gladeFile << endl;
        return nullptr;
    }

    if(gtk_builder_add_from_file(*builder, gladeFile.c_str(), nullptr) == 0) {
        printf("Build failed\n");
        return nullptr;
    }

    GtkWidget* window = GTK_WIDGET (gtk_builder_get_object(*builder, windowName.c_str()));

    if(window == nullptr) {
        cerr << "No window for " << gladeFile << endl;
        return nullptr;
    }

    gtk_widget_set_size_request (GTK_WIDGET(window), 1920, 550);

    gtk_builder_connect_signals(*builder,nullptr);

    g_signal_connect (G_OBJECT (window), "destroy", gtk_main_quit, nullptr);

    return window;
}

void RecorderUI::initPlayerWidgets()
{
    /* Find drawing area*/
    playerWidget = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerWidget");
    if (!playerWidget)
    {
        cerr << "Player widget not found." << endl;
    }

    /* Find cam label */
    playerLabel = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerLabel");
    if (!playerLabel)
    {
        cerr << "Player label not found." << endl;
    }
}

void RecorderUI::initMenuWidgets()
{
    /* Make custom cameras the tab #0 */
    for (auto room : *rooms)
    {
        if (room->type == CUSTOM)
        {
            initRoomTab(0, room->getName());
            initCamWidgets(0, room->getCameras());
            break;
        }
    }

    /* Stip tab with custom cameras if there isn't any */
    int room_n = 1;

    for (auto room : *rooms)
    {
        // Skip all custom rooms
        if (room->type == CUSTOM) continue;

        initRoomTab(room_n, room->getName());
        initCamWidgets(room_n, room->getCameras());
        room_n++;
    }  // for rooms

    /* Edit mode */
    // Find edit button
    editButton = (GtkWidget*) gtk_builder_get_object(menuBuilder, "editButton");
    if (!editButton)
    {
        cerr << "Cannot get editButton" << endl << endl;
    }
    else
    {
        // Assign handler for click event and pass switch pages to it
        g_signal_connect(G_OBJECT(editButton), "clicked", G_CALLBACK(editButtonClicked), &switchGridV);
    }
}

void RecorderUI::initCamWidgets(int room_n, vector<Camera*> *cams)
{

    /* Find out how many cameras are known */
    if (cams->size() > 9)
    {
        cout << "Too many cameras to display in room " << room_n << endl << endl;
    }

    /*  Setting buttons for each camera. Remember not to run out of 9 buttons */
    int n = 0;  
    for (auto cam : *cams)
    {   
        if (n == 9) break;
        /* Create struct object for storing data and ui objects for the camera */
        cam->record = false;

        /* Find the camera button object */
        string name = "b" + to_string(n) + "_" + to_string(room_n);
        GtkWidget *button = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
        
        if (button == nullptr)
        {
            cerr << "Cannot get button " << name << endl << endl;
            n++;
            continue;
        }

        /* Show button and assign cam label */
        gtk_widget_show(button);
        gtk_button_set_label(GTK_BUTTON(button), cam->name.c_str());

        /* Pass player and camName to click handler */
        struct display_player_data *data = new struct display_player_data;
        *data = {cam, playerLabel, player, &playingCamName};
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(displayPlayer), data);

        /* Add button to Camera struct */
        cam->button = button;

        /* Find the rec image object */
        name = "i" + to_string(n) + "_" + to_string(room_n);
        GtkWidget *recImage = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
        if (recImage == nullptr)
        {
            cerr << "Cannot get rec image " << name << endl << endl;
            n++;
            continue;
        }

        /* Add image to Camera struct */
        cam->recImage = recImage;

        /* Find a switch for this camera */
        name = "s" + to_string(n) + "_" + to_string(room_n);
        GtkWidget *sw = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
        switch_state_changed_data *sw_data = new switch_state_changed_data;
        *sw_data = {cam, recManager};
        if (!sw)
        {
            cerr << "Cannot get switch " << name << endl << endl;
        }
        else
        {
            // Make the switch visible and set it to OFF state
            gtk_widget_show(sw);
            gtk_switch_set_active(GTK_SWITCH(sw), false);
            g_signal_connect(G_OBJECT(sw), "state-set", G_CALLBACK(switchStateChanged), sw_data);
        }

        n++;

    }   // for cams
}

void RecorderUI::initRoomTab(int room_n, string room_name)
{
    /* Show page */
    string name = "page" + to_string(room_n);
    GtkWidget *pageGrid = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
    if (!pageGrid)
    {
        cerr << "Cannot get " << name << endl << endl;
        room_n++;
        return;
    }
    gtk_widget_show(pageGrid);

    /* Assign tab title as the name of the room */
    GtkWidget *stack = (GtkWidget*) gtk_builder_get_object(menuBuilder, "stack1");
    if (!stack)
    {
        cerr << "Cannot get stack1" << endl << endl;
        room_n++;
        return;
    }
    GValue a = G_VALUE_INIT;
    g_value_init (&a, G_TYPE_STRING);
    g_value_set_string (&a, room_name.c_str());
    gtk_container_child_set_property (GTK_CONTAINER(stack), pageGrid, "title", &a);
    g_value_unset (&a);


    /* Find grid with switches for edit mode */
    name = "switchGrid" + to_string(room_n);
    GtkWidget* switchGrid = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
    if (!pageGrid)
    {
        cerr << "Cannot get " << name << endl << endl;
    }
    else
    {
        // Store switchGrid in a vector to turn it on/off on edit button click
        switchGridV.push_back(switchGrid);
    }

}

void RecorderUI::displayPlayer(GtkWidget* widget, gpointer data)
{
    auto *context = (display_player_data*) data;
    // gtk_widget_show(context->playerWidget);
    // gtk_widget_show(context->playerLabel);
    context->player->setCam(context->cam->fullName, context->cam->uri);
    context->player->playStream();
    *context->playingCamName = context->cam->fullName;
    gtk_label_set_text(GTK_LABEL(context->playerLabel), context->cam->fullName.c_str());

}

gboolean RecorderUI::keyPressHandle(GtkWidget* widget, GdkEventKey *event, RecorderUI *ui)
{
    if (!ui->rooms) return false;

    if (event->keyval == GDK_KEY_R || event->keyval == GDK_KEY_r)
    {
        cerr << "Start key pressed" << endl << endl;
        // Check if any cameras are picked for recording
        bool picked;
        for (auto room : *(ui->rooms))
        {
            for (auto cam : *(room->getCameras()))
            {
                if (cam->record)
                {
                    picked = true;
                    break;
                }
            }
            if (picked) break;
        }

        if (!picked) return false;

        // Record all cameras that were picked
        // !important: iterate with & if you want to pass cam pointer somewhere
        for (auto &room : *(ui->rooms))
        {
            for (auto &cam : *(room->getCameras()))
            {
                if(cam->record)
                {
                    ui->recManager->startRecording(cam);
                    auto audio = room->getAudioSource();
                    if (audio->uri != "")
                        ui->recManager->startRecording(audio);
                    gtk_widget_show(cam->recImage);
                }
            } // for cams
        } // for rooms
    }

    if (event->keyval == GDK_KEY_S || event->keyval == GDK_KEY_s)
    {
        cerr << "Stop key pressed" << endl << endl;
        ui->recManager->stopAll();
    }
    return true;
}

void RecorderUI::editButtonClicked(GtkWidget* widget, vector<GtkWidget*> *switchGridV)
{
    /* Turns on or off switches when edit button is clicked */
    for (auto switchGrid : *switchGridV)
    {
        gtk_widget_set_visible(switchGrid, !gtk_widget_get_visible(switchGrid));
    }
}

string RecorderUI::findIP()
{
    char host[256];
    char *IP;
    struct hostent *host_entry;
    int hostname;
    hostname = gethostname(host, sizeof(host)); //find the host name
    if (hostname == -1) return "";
    host_entry = gethostbyname(host); //find host information
    if (!host_entry) return "";
    IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); //Convert into IP string
    return string(IP);
}


gboolean RecorderUI::updateGDriveStatus(gpointer user_data)
{
    gdrive_status_data *data = (gdrive_status_data*)user_data;

    if (data->recManager->isGDriveUploadActive())
    {
        gtk_image_set_from_icon_name(GTK_IMAGE(data->GDriveIcon), "gnome-netstatus-tx", (GtkIconSize)35);
    }
    else
    {
        gtk_image_set_from_icon_name(GTK_IMAGE(data->GDriveIcon), "account-logged-in", (GtkIconSize)35);
    }
    return true;
}

void RecorderUI::switchStateChanged(GtkWidget* widget, gboolean state, gpointer user_data)
{
    auto *data = (switch_state_changed_data*)user_data;
    /* If we are in recording state add this camera to recordings or stop, depending on the switch state */
    if (!data->recManager->getRunningRecordings().empty())
    {
        if (state)
        {
            gtk_widget_show(GTK_WIDGET(data->cam->recImage));
            data->recManager->startRecording(data->cam);
        }
        else
        {
            data->recManager->stopRecording(data->cam);
        }
    }
    data->cam->record = state;
}
