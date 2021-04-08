#include "singleStreamUI.h"

SingleStreamUI::SingleStreamUI()
{
    gtk_init (nullptr, nullptr);

    /* Init windows */
    playerBuilder = nullptr;
    playerWindow = windowInit(&playerBuilder, "ui/player.glade", "player");

    if (!playerBuilder || !playerBuilder)
    {
        cerr << "Failed to init window" << endl << endl;
        exit(1);
    }
    
    /* Set window size */
    gtk_widget_set_size_request(playerWindow, 1080, 720);
	
    /* Init styles */
    int res = initStyles();
    if(res == -1) {
        cerr << "Failed to get styles" << endl << endl;
    }

    /* Find player and window elements to control*/
    initPlayerWidgets();
    player = new Player(playerWidget, "other");
    player->setCam("307", "rtsp://172.18.191.74:554/Streaming/Channels/1");
    player->onClick = {.func_to_call = eventHandleFunction(playerClicked), .params = this};
    initOverlayWidgets();

    g_signal_connect(playerWindow, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    gtk_window_present(GTK_WINDOW(playerWindow));

    /* MQTT */
    // create queue for communicating between threads (redirecting mqtt messages)
    mqttQueue = g_async_queue_new();
    g_thread_new("run_mqtt", runMqtt, this); // thread for mqtt-client
    g_thread_new("mqtt_queue", parseQueue, this); // thread for parsing messages

    /* Conecting to camera controlling server */
    cameraClient = new CameraClient("127.0.0.1", 5555);
    if (!cameraClient->connectToCamera("172.18.191.74", 80, "admin", "Supervisor"))
    {
        cerr << "Could not connect to camera" << endl << endl;
    }

    // Pass control to GTK
    gtk_main();

    // Close app (all threads) after gtk_main_quit
    exit(0);

}

SingleStreamUI::~SingleStreamUI()
{
    delete player;

    // MQTT
    delete mqtt;
    g_async_queue_unref(mqttQueue);

}

gpointer SingleStreamUI::runMqtt(gpointer data)
{
    auto obj = (SingleStreamUI*)data; //SingleStreamUI object

    auto mqtt = new MqttClient("tcp://localhost:1883", "singlestream", {"operator/active_cam"});
    obj->mqtt = mqtt;

    // start receiving messages to queue
    mqtt->passMessagesToQueue(obj->mqttQueue);

}


gpointer SingleStreamUI::parseQueue(gpointer data)
{
    auto obj = (SingleStreamUI*)data; //SingleStreamUI object
    // Extract message from the queue and take actions
    while (auto pop = g_async_queue_pop(obj->mqttQueue))
    {
        auto *msg = (QueueMQTTMessage*) pop;
        cout << "Message in " << msg->topic << ": "
             << msg->message << endl << endl;

        // React to messages
        if (msg->topic == "operator/active_cam")
        {
            // Operator picked another camera, changing playing stream
            // message format: camName,uri
            string camName, uri;
            int split = msg->message.find(',');
            string camName = msg->message.substr(0, split);
            string uri = msg->message.substr(split+1, msg->message.length() - split);
            obj->player->setCam(camName, uri);

            // TODO: Changing camera to be controlled

        }
    }
}

int SingleStreamUI::initStyles() {
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

GtkWidget* SingleStreamUI::windowInit(GtkBuilder** builder, string gladeFile, string windowName) {
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

    gtk_builder_connect_signals(*builder,nullptr);

    g_signal_connect (G_OBJECT (window), "destroy", gtk_main_quit, nullptr);

    return window;
}

void SingleStreamUI::initPlayerWidgets()
{
    /* Find drawing area*/
    playerWidget = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerWidget");
    if (!playerWidget)
    {
        cerr << "Player widget not found." << endl;
    }
    // gtk_widget_add_events(playerWidget, GDK_BUTTON_PRESS_MASK); 
    // g_signal_connect(playerWindow, "button-press-event", G_CALLBACK(playerClicked), NULL);

    /* Find cam label */
    playerLabel = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerLabel");
    if (!playerLabel)
    {
        cerr << "Player label not found." << endl;
    }
}

gboolean SingleStreamUI::showOverlayWidgets(gpointer ui_ptr)
{
    SingleStreamUI* ui = (SingleStreamUI*)ui_ptr;
    for (auto widget : ui->hidableOverlayWidgets)
    {
        gtk_widget_show(widget);
    }
    return false;
}

gboolean SingleStreamUI::hideOverlayWidgets(gpointer ui_ptr)
{
    SingleStreamUI* ui = (SingleStreamUI*)ui_ptr;
    for (auto widget : ui->hidableOverlayWidgets)
    {
        gtk_widget_hide(widget);
    }
    return false;
}

void SingleStreamUI::initOverlayWidgets()
{
    vector<string> names = {"b_up", "b_down", "b_right", "b_left"};

    for (auto name : names)
    {
        GtkWidget* widget = (GtkWidget*)gtk_builder_get_object(playerBuilder, name.c_str());
        if (!widget)
        {
            cerr << name << " not found." << endl;
            exit(1);
        }
        g_signal_connect(widget, "clicked", G_CALLBACK (overlayWidgetClicked), this);
        hidableOverlayWidgets.push_back(widget);
        hideOverlayWidgets(this);
    }    
}


gboolean SingleStreamUI::overlayWidgetClicked( GtkWidget *widget, GdkEventButton *event, gpointer ui_ptr)
{
    SingleStreamUI* ui = (SingleStreamUI*)ui_ptr;
    string buttonName = gtk_buildable_get_name(GTK_BUILDABLE(widget));
    cout << "CLICKED " << buttonName << endl;
    cameraCommand_t command;
    if (buttonName == "b_up") command = UP;
    if (buttonName == "b_down") command = DOWN;
    if (buttonName == "b_left") command = LEFT;
    if (buttonName == "b_right") command = RIGHT;

    ui->cameraClient->commandToCamera(command);
 
    // Reschedule hiding widgets
    g_assert(ui->hideWidgetsGSource != 0);
    g_source_remove(ui->hideWidgetsGSource);
    ui->hideWidgetsGSource = g_timeout_add_seconds(3, hideOverlayWidgets, ui);
    return TRUE;
}

void SingleStreamUI::playerClicked(SingleStreamUI *ui)
{
    if (ui->hidableOverlayWidgets.empty())
        return;
    
    // Checking for visible widgets. If any one is visible, hide all of them. 
    if (gtk_widget_get_visible(ui->hidableOverlayWidgets[0]))
    {
        ui->hideOverlayWidgets(ui);
    }
    // Otherwise show all widgets and start job to hide them after some time 
    else
    {
        ui->showOverlayWidgets(ui);
        ui->hideWidgetsGSource = g_timeout_add_seconds(3, hideOverlayWidgets, ui);
    }
    

}