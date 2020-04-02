#include "ui.h"

UI::UI(Config *config)
{
    this->config = config;
    this->rooms = config->getRooms();

    gtk_init (nullptr, nullptr);
    
    /* Init windows */
    menuBuilder = nullptr;
    menuWindow = windowInit(&menuBuilder, "ui/menu.glade", "menu");
    
    if(menuWindow == nullptr || menuBuilder == nullptr) {
        cerr << "Failed to init builder" << endl;
        exit(0);
    }

    /* Set window size */
    gtk_widget_set_size_request(menuWindow, stoi(config->getParam("windowWidth")),
            stoi(config->getParam("windowHeight")));


    /* Start/stop recording on key press */
    gtk_widget_add_events(menuWindow, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(menuWindow), "key_press_event", G_CALLBACK(keyPress), this);

    /* Find player and window elements to control*/
    initMenuWidgets();

    /* Add information to status bar */
    // IP address
    string ip = findIP();
    GtkWidget* ipLabel = GTK_WIDGET (gtk_builder_get_object(menuBuilder, "ipLabel"));
    gtk_label_set_text(GTK_LABEL(ipLabel), ip.c_str());

    /* Init styles */
    int res = initStyles();
    if(res == -1) {
        cerr << "Failed to get styles" << endl << endl;
    }

    /* Place windows */
    if (config->getParamInt("screens") == 2)
    {
        cout << "Showing GUI on full screen" << endl << endl;
        GdkScreen *screen = gdk_screen_get_default();
        gtk_window_fullscreen_on_monitor(GTK_WINDOW(menuWindow), screen, 0);
        gtk_widget_show(menuWindow);
    }
    else
    {
        /* For one screen */
        cout << "Showing GUI in window mode" << endl << endl;
        gtk_window_move(GTK_WINDOW(menuWindow), 0, 100);
        gtk_window_present(GTK_WINDOW(menuWindow));
    }

    g_signal_connect(menuWindow, "destroy", G_CALLBACK (gtk_main_quit), NULL);

//    g_idle_add(on_show, this);

    

    gtk_main();
}

UI::~UI()
{
}

int UI::on_show(gpointer ui_ptr)
{
    UI *ui = (UI*)ui_ptr;
        /* Find window elements to control*/
        ui->initMenuWidgets();

        /* Add information to status bar */
        // IP address
        string ip = ui->findIP();
        GtkWidget* ipLabel = GTK_WIDGET (gtk_builder_get_object(ui->menuBuilder, "ipLabel"));
        gtk_label_set_text(GTK_LABEL(ipLabel), ip.c_str());

        /* Init styles */
        int res = ui->initStyles();
        if(res == -1) {
            cerr << "Failed to get styles" << endl << endl;
        }
    return 0;
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


void UI::initMenuWidgets()
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

    /* Asign check for detecting page switching */
    GtkWidget *stack = (GtkWidget*) gtk_builder_get_object(menuBuilder, "stack1");
    if (!stack)
    {
        cerr << "Cannot get stack1" << endl << endl;
        room_n++;
        return;
    }
    g_signal_connect(G_OBJECT (stack), "notify::visible-child", G_CALLBACK(pageSwitched), rooms);
    // Run for currently visible page
    pageSwitched(GTK_WIDGET(stack), nullptr, rooms);
}

void UI::pageSwitched(GtkWidget *widget, GParamSpec *property, vector<Room*>* rooms)
{
//    auto rooms = (vector<Room*>*) data;
    string page = gtk_stack_get_visible_child_name (GTK_STACK(widget));
    // Page name is like "page2", getting number by removing "page" substring
    page.replace(page.begin(), page.begin()+4, "");
    int pageNum = stoi(page);

    /* Stop all playing players */
    bool foundPlayingPage = false;
    for (auto room : *rooms)
    {
        for (auto cam : *(room->getCameras()))
        {
            if (cam.player->isPlaying())
            {
                cam.player->stopStream();
                foundPlayingPage = true;
            }
        }
        if (foundPlayingPage) break;
    }

    /* Starting players on active page */
    for (Camera cam : *(rooms->at(pageNum - 1)->getCameras()))
    {
        cam.player->playStream();
    }
    cerr << "Switched to " << rooms->at(pageNum - 1)->getName() << endl << endl;
}

void UI::initCamWidgets(int room_n, vector<Camera> *cams)
{

    /* Find out how many cameras are known */
    if (cams->size() > 9)
    {
        cout << "Too many cameras to display in room " << room_n << endl << endl;
    }

    /*  Setting buttons for each camera. Remember not to run out of 9 buttons */
    int n = 0;  
    for (Camera &cam : *cams)
    {   
        if (n == 9) break;
        /* Create struct object for storing data and ui objects for the camera */

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
//        gtk_button_set_label(GTK_BUTTON(button), cam.name.c_str());

        /* Pass player and camName to click handler */
        struct display_player_data *data = new struct display_player_data;
        *data = {cam, &playingCamName};
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(displayPlayer), data);

        /* Add button to Camera struct */
        cam.button = button;

        /* Find the drawing area object */
        name = "da" + to_string(n) + "_" + to_string(room_n);
        GtkWidget *drawingArea = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
        if (drawingArea == nullptr)
        {
            cerr << "Cannot get drawingArea " << name << endl << endl;
            n++;
            continue;
        }

        /* Add drawingArea to Camera struct */
        cam.drawingArea = drawingArea;
        gtk_widget_show(drawingArea);
        /* Create player for camera and assign drawing area and stream url */
        Player *player = new Player(drawingArea, config->getParam("platform"), cam.uri, cam.name);
        cam.player = player;

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

}

void UI::displayPlayer(GtkWidget* widget, gpointer data)
{
    auto *context = (display_player_data*) data;
    cout << "Pressed on " << context->cam.fullName << endl << endl;
}

gboolean UI::keyPress(GtkWidget* widget, GdkEventKey *event, UI *ui)
{
    if (!ui->rooms) return false;

    if (event->keyval == GDK_KEY_R || event->keyval == GDK_KEY_r)
    {
        cerr << "Start key pressed" << endl << endl;
    }

    if (event->keyval == GDK_KEY_S || event->keyval == GDK_KEY_s)
    {
        cerr << "Stop key pressed" << endl << endl;
    }
    return true;
}

string UI::findIP()
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