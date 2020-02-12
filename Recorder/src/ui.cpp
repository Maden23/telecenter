#include "ui.h"

UI::UI(Config *config)
{
    this->config = config;

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
    recorder = new Recorder(config);
    gtk_widget_add_events(menuWindow, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(menuWindow), "key_press_event", G_CALLBACK(keyPress), this);


    /* Find player and window elements to control*/
    initPlayerWidgets();
    player = new Player(playerWidget, config);
    initMenuWidgets();

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

UI::~UI()
{
    delete player;
    delete recorder;
}


int UI::initStyles() {
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

GtkWidget* UI::windowInit(GtkBuilder** builder, string gladeFile, string windowName) {
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

void UI::initPlayerWidgets()
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

void UI::initMenuWidgets()
{
    auto rooms = config->getRooms();

    /* Make custom cameras the tab #0 */
    auto custom = rooms.find("custom");
    if (custom != rooms.end())
    {
        initRoomTab(0, custom->first);
        initCamWidgets(0, custom->second);
        rooms.erase(custom);
    }
    /* Stip tab with custom cameras if there isn't any */
    int room_n = 1;

    for (auto room : rooms)
    {

        initRoomTab(room_n, room.first);
        initCamWidgets(room_n, room.second);
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

void UI::initCamWidgets(int room_n, map<string, string> cams)
{
    /* Find out how many cameras are known */
    if (cams.size() > 9)
    {
        cout << "Too many cameras to display in room " << room_n << endl << endl;
    }

    /*  Setting buttons for each camera. Remember not to run out of 9 buttons */
    int n = 0;  
    for (auto &cam : cams)
    {   
        if (n == 9) break;

        /* Create struct object for storing data and ui objects for the camera */
        struct Camera camData;
        camData.name = cam.first;
        camData.uri = cam.second;

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
        gtk_button_set_label(GTK_BUTTON(button), cam.first.c_str());

        /* Pass player and camName to click handler */
        struct display_player_data *data = new struct display_player_data;
        data->player = player;
        data->camName = cam.first;
        data->uri = cam.second;
        data->playerLabel = playerLabel;
        data->playingCamName = &playingCamName;

        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(displayPlayer), data);

        /* Add button to Camera struct */
        camData.button = button;

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
        camData.recImage = recImage;

        /* Add cemera data to vector */
        camDataV.push_back(camData);

        /* Find a switch for this camera */
        // if (switchGrid)
        // {
            name = "s" + to_string(n) + "_" + to_string(room_n);
            GtkWidget *sw = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
            if (!sw)
            {
                cerr << "Cannot get switch " << name << endl << endl;
            }
            else
            {
                // Make the switch visible and set it to ON state
                gtk_widget_show(sw);
                gtk_switch_set_active(GTK_SWITCH(sw), true);
            }
        // }

        n++;

    }   // for cams
}

void UI::initRoomTab(int room_n, string room_name)
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

void displayPlayer(GtkWidget* widget, gpointer *data)
{
    auto context = (struct display_player_data*) data;
    // gtk_widget_show(context->playerWidget);
    // gtk_widget_show(context->playerLabel);
    context->player->playStream(context->uri);
    *context->playingCamName = context->camName;
    gtk_label_set_text(GTK_LABEL(context->playerLabel), context->camName.c_str());

}

gboolean keyPress(GtkWidget* widget, GdkEventKey *event, UI *ui)
{
    struct Camera *cam = nullptr;
    /* Find data struct for playing camera */
    for (auto camData : ui->camDataV)
    {
        if (camData.name == ui->playingCamName)
        {
            cam = &camData;
            break;
        }
    }
    if (!cam) return false;

    if (event->keyval == GDK_KEY_R || event->keyval == GDK_KEY_r)
    {
        /* If no stream is playing, record all */
        // if (ui->playingCamName == "")
        // {
        //     for (auto &rec : ui->recImages)
        //     {
        //         ui->recorder->startRecording(ui->config->getCamUri(rec.first));
        //         gtk_widget_show(rec.second);
        //     }
        // }
        // /* or record the playing stream */   
        // else

        if (ui->playingCamName != "")
        {
            ui->recorder->startRecording(cam->uri);
            gtk_widget_show(cam->recImage);
        }
    }

    if (event->keyval == GDK_KEY_S || event->keyval == GDK_KEY_s)
    {
        /* If no stream is playing, stop recording all */
        cerr << "Stop key pressed" << endl << endl;
        // if (ui->playingCamName == "")
        // {
        //     for (auto &rec : ui->recImages)
        //     {
        //         ui->recorder->stopRecording(ui->config->getCamUri(rec.first));
        //         gtk_widget_hide(rec.second);
        //     }
        // }
        // /* or stop recording the playing stream */   
        // else

        if (ui->playingCamName != "")
        {
            if(ui->recorder->stopRecording(cam->uri))
                gtk_widget_hide(cam->recImage);
        }
        else
        {
            cerr << "No playing cam" << endl << endl;
            return false;
        }
    }
    return true;
}

void editButtonClicked(GtkWidget* widget, vector<GtkWidget*> *switchGridV)
{
    /* Turns on or off switches when edit button is clicked */
    // vector <GtkWidget*> *switchGridV = (vector <GtkWidget*>*) data;
    for (auto switchGrid : *switchGridV)
    {
        gtk_widget_set_visible(switchGrid, !gtk_widget_get_visible(switchGrid));
    }
}